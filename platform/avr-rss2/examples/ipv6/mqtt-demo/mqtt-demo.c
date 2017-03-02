/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/** \addtogroup cc2538-examples
 * @{
 *
 * \defgroup cc2538-mqtt-demo CC2538DK MQTT Demo Project
 *
 * Demonstrates MQTT functionality. Works with IBM Quickstart as well as
 * mosquitto.
 * @{
 *
 * \file
 * An MQTT example for the cc2538dk platform
 */
/*---------------------------------------------------------------------------*/
#include "contiki-conf.h"
#include "rpl/rpl-private.h"
#include "mqtt.h"
#include "net/rpl/rpl.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ipv6/sicslowpan.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "lib/sensors.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/temp-sensor.h"
#include "dev/battery-sensor.h"
#include <string.h>
#ifdef CO2
#include "dev/co2_sa_kxx-sensor.h"
#endif
#include "adc.h"
#include "dev/pulse-sensor.h"
#include "dev/pms5003/pms5003.h"
#include "dev/pms5003/pms5003-sensor.h"
#include "i2c.h"
#include "dev/bme280/bme280-sensor.h"
#include "dev/serial-line.h"

extern void handle_serial_input(const char *line);

/*---------------------------------------------------------------------------*/
/*
 * IBM server: messaging.quickstart.internetofthings.ibmcloud.com
 * (184.172.124.189) mapped in an NAT64 (prefix 64:ff9b::/96) IPv6 address
 * Note: If not able to connect; lookup the IP address again as it may change.
 *
 * Alternatively, publish to a local MQTT broker (e.g. mosquitto) running on
 * the node that hosts your border router
 */
#ifdef MQTT_DEMO_BROKER_IP_ADDR
static const char *broker_ip = MQTT_DEMO_BROKER_IP_ADDR;
#define DEFAULT_ORG_ID              "mqtt-demo"
#else
static const char *broker_ip = "0064:ff9b:0000:0000:0000:0000:b8ac:7cbd";
#define DEFAULT_ORG_ID              "quickstart"
#endif
/*---------------------------------------------------------------------------*/
/*
 * A timeout used when waiting for something to happen (e.g. to connect or to
 * disconnect)
 */
#define STATE_MACHINE_PERIODIC     (CLOCK_SECOND >> 1)
/*---------------------------------------------------------------------------*/
/* Provide visible feedback via LEDS during various states */
/* When connecting to broker */
#define CONNECTING_LED_DURATION    (CLOCK_SECOND >> 2)

/* Each time we try to publish */
#define PUBLISH_LED_ON_DURATION    (CLOCK_SECOND)
/*---------------------------------------------------------------------------*/
/* Connections and reconnections */
#define RETRY_FOREVER              0xFF
#define RECONNECT_INTERVAL         (CLOCK_SECOND * 2)

/*
 * Number of times to try reconnecting to the broker.
 * Can be a limited number (e.g. 3, 10 etc) or can be set to RETRY_FOREVER
 */
#define RECONNECT_ATTEMPTS         RETRY_FOREVER
#define CONNECTION_STABLE_TIME     (CLOCK_SECOND * 5)
static struct timer connection_life;
static uint8_t connect_attempt;
/*---------------------------------------------------------------------------*/
/* Various states */
static uint8_t state;
#define STATE_INIT            0
#define STATE_REGISTERED      1
#define STATE_CONNECTING      2
#define STATE_CONNECTED       3
#define STATE_PUBLISHING      4
#define STATE_DISCONNECTED    5
#define STATE_NEWCONFIG       6
#define STATE_CONFIG_ERROR 0xFE
#define STATE_ERROR        0xFF
/*---------------------------------------------------------------------------*/
#define CONFIG_ORG_ID_LEN        32
#define CONFIG_TYPE_ID_LEN       32
#define CONFIG_AUTH_TOKEN_LEN    32
#define CONFIG_EVENT_TYPE_ID_LEN 32
#define CONFIG_CMD_TYPE_LEN       8
#define CONFIG_IP_ADDR_STR_LEN   64
/*---------------------------------------------------------------------------*/
#define RSSI_MEASURE_INTERVAL_MAX 86400 /* secs: 1 day */
#define RSSI_MEASURE_INTERVAL_MIN     5 /* secs */
#define PUBLISH_INTERVAL_MAX      86400 /* secs: 1 day */
#define PUBLISH_INTERVAL_MIN          5 /* secs */
/*---------------------------------------------------------------------------*/
/* A timeout used when waiting to connect to a network */
#define NET_CONNECT_PERIODIC        (CLOCK_SECOND >> 2)
#define NO_NET_LED_DURATION         (NET_CONNECT_PERIODIC >> 1)
/*---------------------------------------------------------------------------*/
/* Default configuration values */
#define DEFAULT_TYPE_ID             "avr-rss2"
#define DEFAULT_AUTH_TOKEN          "AUTHZ"
#define DEFAULT_EVENT_TYPE_ID       "status"
#define DEFAULT_SUBSCRIBE_CMD_TYPE  "+"
#define DEFAULT_BROKER_PORT         1883
#define DEFAULT_PUBLISH_INTERVAL    (30 * CLOCK_SECOND)
#define DEFAULT_KEEP_ALIVE_TIMER    60
#define DEFAULT_RSSI_MEAS_INTERVAL  (CLOCK_SECOND * 30)
/*---------------------------------------------------------------------------*/
/* Take a sensor reading on button press */
#define PUBLISH_TRIGGER &button_sensor

/* Payload length of ICMPv6 echo requests used to measure RSSI with def rt */
#define ECHO_REQ_PAYLOAD_LEN   20
/*---------------------------------------------------------------------------*/
PROCESS_NAME(mqtt_demo_process);
#define CHECKER
#ifdef CHECKER
PROCESS(mqtt_checker_process, "MQTT state checker for debug");

AUTOSTART_PROCESSES(&mqtt_demo_process, &sensors_process, &mqtt_checker_process);
#else
AUTOSTART_PROCESSES(&mqtt_demo_process, &sensors_process);
#endif
SENSORS(&pms5003_sensor);

/*---------------------------------------------------------------------------*/
/**
 * \brief Data structure declaration for the MQTT client configuration
 */
typedef struct mqtt_client_config {
  char org_id[CONFIG_ORG_ID_LEN];
  char type_id[CONFIG_TYPE_ID_LEN];
  char auth_token[CONFIG_AUTH_TOKEN_LEN];
  char event_type_id[CONFIG_EVENT_TYPE_ID_LEN];
  char broker_ip[CONFIG_IP_ADDR_STR_LEN];
  char cmd_type[CONFIG_CMD_TYPE_LEN];
  clock_time_t pub_interval;
  int def_rt_ping_interval;
  uint16_t broker_port;
} mqtt_client_config_t;
/*---------------------------------------------------------------------------*/
/* Maximum TCP segment size for outgoing segments of our socket */
#define MAX_TCP_SEGMENT_SIZE  32  
/*---------------------------------------------------------------------------*/
#define STATUS_LED LEDS_YELLOW
/*---------------------------------------------------------------------------*/
/*
 * Buffers for Client ID and Topic.
 * Make sure they are large enough to hold the entire respective string
 *
 * d:quickstart:status:EUI64 is 32 bytes long
 * iot-2/evt/status/fmt/json is 25 bytes
 * We also need space for the null termination
 */
#define BUFFER_SIZE 64
static char client_id[BUFFER_SIZE];
static char pub_topic[BUFFER_SIZE];
static char sub_topic[BUFFER_SIZE];
/*---------------------------------------------------------------------------*/
/*
 * The main MQTT buffers.
 * We will need to increase if we start publishing more data.
 */
#define APP_BUFFER_SIZE 1024
static struct mqtt_connection conn;
static char app_buffer[APP_BUFFER_SIZE];
/*---------------------------------------------------------------------------*/
#define QUICKSTART "quickstart"
/*---------------------------------------------------------------------------*/
static struct mqtt_message *msg_ptr = 0;
static struct etimer publish_periodic_timer;
static struct ctimer ct;
static char *buf_ptr;
static uint16_t seq_nr_value = 0;
/*---------------------------------------------------------------------------*/
/* Parent RSSI functionality */
static struct uip_icmp6_echo_reply_notification echo_reply_notification;
static struct etimer echo_request_timer;
static unsigned long def_rt_rssi = 0;
/*---------------------------------------------------------------------------*/
static mqtt_client_config_t conf;
/*---------------------------------------------------------------------------*/
PROCESS(mqtt_demo_process, "MQTT Demo");
/*---------------------------------------------------------------------------*/
int
ipaddr_sprintf(char *buf, uint8_t buf_len, const uip_ipaddr_t *addr)
{
  uint16_t a;
  uint8_t len = 0;
  int i, f;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) {
        len += snprintf(&buf[len], buf_len - len, "::");
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
        len += snprintf(&buf[len], buf_len - len, ":");
      }
      len += snprintf(&buf[len], buf_len - len, "%x", a);
    }
  }

  return len;
}
/*---------------------------------------------------------------------------*/
static void
echo_reply_handler(uip_ipaddr_t *source, uint8_t ttl, uint8_t *data,
                   uint16_t datalen)
{
  if(uip_ip6addr_cmp(source, uip_ds6_defrt_choose())) {
    def_rt_rssi = sicslowpan_get_last_rssi();
  }
}
/*---------------------------------------------------------------------------*/
static void
publish_led_off(void *d)
{
  leds_off(STATUS_LED);
}
/*---------------------------------------------------------------------------*/
static void
pub_handler(const char *topic, uint16_t topic_len, const uint8_t *chunk,
            uint16_t chunk_len)
{
  DBG("Pub Handler: topic='%s' (len=%u), chunk_len=%u\n", topic, topic_len,
      chunk_len);

  /* If we don't like the length, ignore */
  if(topic_len != 23 || chunk_len != 1) {
    printf("Incorrect topic or chunk len. Ignored\n");
    return;
  }

  /* If the format != json, ignore */
  if(strncmp(&topic[topic_len - 4], "json", 4) != 0) {
    printf("Incorrect format\n");
  }

  if(strncmp(&topic[10], "leds", 4) == 0) {
    if(chunk[0] == '1') {
      leds_on(LEDS_RED);
    } else if(chunk[0] == '0') {
      leds_off(LEDS_RED);
    }
    return;
  }
}
static struct mqtt_app_statistics {
  unsigned int connected;
  unsigned int disconnected;
  unsigned int published;
  unsigned int pubacked;
} mqtt_stats = {0, 0, 0, 0};

/*---------------------------------------------------------------------------*/
static void
mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data)
{
  switch(event) {
  case MQTT_EVENT_CONNECTED: {
    DBG("APP - Application has a MQTT connection\n");
    timer_set(&connection_life, CONNECTION_STABLE_TIME);
    state = STATE_CONNECTED;
    mqtt_stats.connected++;
    break;
  }
  case MQTT_EVENT_DISCONNECTED: {
    DBG("APP - MQTT Disconnect. Reason %u\n", *((mqtt_event_t *)data));

    state = STATE_DISCONNECTED;
    process_poll(&mqtt_demo_process);
    mqtt_stats.disconnected++;
    break;
  }
  case MQTT_EVENT_PUBLISH: {
    msg_ptr = data;

    /* Implement first_flag in publish message? */
    if(msg_ptr->first_chunk) {
      msg_ptr->first_chunk = 0;
      DBG("APP - Application received a publish on topic '%s'. Payload "
          "size is %i bytes. Content:\n\n",
          msg_ptr->topic, msg_ptr->payload_length);
    }

    pub_handler(msg_ptr->topic, strlen(msg_ptr->topic), msg_ptr->payload_chunk,
                msg_ptr->payload_length);
    break;
  }
  case MQTT_EVENT_SUBACK: {
    DBG("APP - Application is subscribed to topic successfully\n");
    break;
  }
  case MQTT_EVENT_UNSUBACK: {
    DBG("APP - Application is unsubscribed to topic successfully\n");
    break;
  }
  case MQTT_EVENT_PUBACK: {
    DBG("APP - Publishing complete.\n");
    mqtt_stats.pubacked++;
    break;
  }
  default:
    DBG("APP - Application got a unhandled MQTT event: %i\n", event);
    break;
  }
}
/*---------------------------------------------------------------------------*/
static int
construct_pub_topic(void)
{
  int len = snprintf(pub_topic, BUFFER_SIZE, "iot-2/evt/%s/fmt/json",
                     conf.event_type_id);

  /* len < 0: Error. Len >= BUFFER_SIZE: Buffer too small */
  if(len < 0 || len >= BUFFER_SIZE) {
    printf("Pub Topic: %d, Buffer %d\n", len, BUFFER_SIZE);
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
construct_sub_topic(void)
{
  int len = snprintf(sub_topic, BUFFER_SIZE, "iot-2/cmd/%s/fmt/json",
                     conf.cmd_type);

  /* len < 0: Error. Len >= BUFFER_SIZE: Buffer too small */
  if(len < 0 || len >= BUFFER_SIZE) {
    printf("Sub Topic: %d, Buffer %d\n", len, BUFFER_SIZE);
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
construct_client_id(void)
{
  int len = snprintf(client_id, BUFFER_SIZE, "d:%s:%s:%02x%02x%02x%02x%02x%02x",
                     conf.org_id, conf.type_id,
                     linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
                     linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],
                     linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);

  /* len < 0: Error. Len >= BUFFER_SIZE: Buffer too small */
  if(len < 0 || len >= BUFFER_SIZE) {
    printf("Client ID: %d, Buffer %d\n", len, BUFFER_SIZE);
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
update_config(void)
{
  if(construct_client_id() == 0) {
    /* Fatal error. Client ID larger than the buffer */
    state = STATE_CONFIG_ERROR;
    return;
  }

  if(construct_sub_topic() == 0) {
    /* Fatal error. Topic larger than the buffer */
    state = STATE_CONFIG_ERROR;
    return;
  }

  if(construct_pub_topic() == 0) {
    /* Fatal error. Topic larger than the buffer */
    state = STATE_CONFIG_ERROR;
    return;
  }

  /* Reset the counter */
  seq_nr_value = 0;

  state = STATE_INIT;

  /*
   * Schedule next timer event ASAP
   *
   * If we entered an error state then we won't do anything when it fires.
   *
   * Since the error at this stage is a config error, we will only exit this
   * error state if we get a new config.
   */
  etimer_set(&publish_periodic_timer, 0);

  return;
}
/*---------------------------------------------------------------------------*/
static int
init_config()
{
  /* Populate configuration with default values */
  memset(&conf, 0, sizeof(mqtt_client_config_t));

  memcpy(conf.org_id, DEFAULT_ORG_ID, strlen(DEFAULT_ORG_ID));
  memcpy(conf.type_id, DEFAULT_TYPE_ID, strlen(DEFAULT_TYPE_ID));
  memcpy(conf.auth_token, DEFAULT_AUTH_TOKEN, strlen(DEFAULT_AUTH_TOKEN));
  memcpy(conf.event_type_id, DEFAULT_EVENT_TYPE_ID,
         strlen(DEFAULT_EVENT_TYPE_ID));
  memcpy(conf.broker_ip, broker_ip, strlen(broker_ip));
  memcpy(conf.cmd_type, DEFAULT_SUBSCRIBE_CMD_TYPE, 1);

  conf.broker_port = DEFAULT_BROKER_PORT;
  conf.pub_interval = DEFAULT_PUBLISH_INTERVAL;
  conf.def_rt_ping_interval = DEFAULT_RSSI_MEAS_INTERVAL;

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
subscribe(void)
{
  /* Publish MQTT topic in IBM quickstart format */
  mqtt_status_t status;

  status = mqtt_subscribe(&conn, NULL, sub_topic, MQTT_QOS_LEVEL_0);

  DBG("APP - Subscribing!\n");
  if(status == MQTT_STATUS_OUT_QUEUE_FULL) {
    DBG("APP - Tried to subscribe but command queue was full!\n");
  }
}
/*---------------------------------------------------------------------------*/
#define MQTT_PUBLISH_SENML
#ifdef MQTT_PUBLISH_IBMQS
static void
publish(void)
{
  /* Publish MQTT topic in IBM quickstart format */
  int len;
  int remaining = APP_BUFFER_SIZE;

  seq_nr_value++;

  buf_ptr = app_buffer;

#if 1

  uip_ipaddr_t loc_fipaddr; /* Link local address - use 8 last bytes for ID */
  uint8_t *ll; 
  uip_create_linklocal_prefix(&loc_fipaddr);
  uip_ds6_set_addr_iid(&loc_fipaddr, &uip_lladdr);
  //ll = (uint8_t *) linkaddr_node_addr.u8;
  ll = (uint8_t *) &loc_fipaddr;
  len = snprintf(buf_ptr, remaining,
                 "{"
                 "\"d\":{"
                 "\"Name\":\"%s\","
		 "\"ID\": \"%02x%02x%02x%02x%02x%02x%02x%02x\","
                 "\"Seq #\":%d,"
                 "\"Uptime (sec)\":%lu",
                 BOARD_STRING,
		 ll[8], ll[9], ll[10], ll[11], ll[12], ll[13], ll[14], ll[15],
		 //ll[0], ll[1], ll[2], ll[3], ll[4], ll[5], ll[6], ll[7],
		 seq_nr_value, clock_seconds());
#endif

  if(len < 0 || len >= remaining) {
    printf("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
    return;
  }

  remaining -= len;
  buf_ptr += len;

  len = snprintf(buf_ptr, remaining, ",\"V_MCU (V)\":%-5.2f",
		 ((double) battery_sensor.value(0)/1000.));

  if(len < 0 || len >= remaining) {
    printf("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
    return;
  }

  remaining -= len;
  buf_ptr += len;


  /* Put our Default route's string representation in a buffer */
  char def_rt_str[64];
  memset(def_rt_str, 0, sizeof(def_rt_str));
  ipaddr_sprintf(def_rt_str, sizeof(def_rt_str), uip_ds6_defrt_choose());

  len = snprintf(buf_ptr, remaining, ",\"Def Route\":\"%s\",\"RSSI (dBm)\":%d",
                 def_rt_str, def_rt_rssi);

  if(len < 0 || len >= remaining) {
    printf("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
    return;
  }
  remaining -= len;
  buf_ptr += len;

#ifdef CO2
  len = snprintf(buf_ptr, remaining, ",\"SA_CO2 (ppm)\":%d",
                 co2_sa_kxx_sensor.value(CO2_SA_KXX_CO2));

  if(len < 0 || len >= remaining) {
    printf("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
    return;
  }

  remaining -= len;
  buf_ptr += len;
#endif

  len = snprintf(buf_ptr, remaining, ",\"Particles TSI standard, CF=1 (ug/m3)\": {\"PM1\":%d, \"PM2.5\":%d, \"PM10\":%d}",
                 pms5003_sensor.value(PMS5003_SENSOR_PM1),
                 pms5003_sensor.value(PMS5003_SENSOR_PM2_5), 
		 pms5003_sensor.value(PMS5003_SENSOR_PM10));

  if(len < 0 || len >= remaining) {
    printf("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
    return;
  }

  remaining -= len;
  buf_ptr += len;

  len = snprintf(buf_ptr, remaining, ",\"Particles std. atmosphere (ug/m3)\": {\"PM1\":%d, \"PM2.5\":%d, \"PM10\":%d}",
                 pms5003_sensor.value(PMS5003_SENSOR_PM1_ATM),
                 pms5003_sensor.value(PMS5003_SENSOR_PM2_5_ATM), 
		 pms5003_sensor.value(PMS5003_SENSOR_PM10_ATM));

  if(len < 0 || len >= remaining) {
    printf("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
    return;
  }

  remaining -= len;
  buf_ptr += len;
  extern uint32_t pms5003_valid_frames();
  extern uint32_t pms5003_invalid_frames();

  len = snprintf(buf_ptr, remaining, ",\"PMS5003\": {\"valid frames\":%lu, \"invalid frames\":%lu}",
                 pms5003_valid_frames(),
                 pms5003_invalid_frames());

  if(len < 0 || len >= remaining) {
    printf("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
    return;
  }

  remaining -= len;
  buf_ptr += len;

  if( i2c_probed & I2C_BME280 ) {
    len = snprintf(buf_ptr, remaining, ",\"BME280\":{\"Temperature (C)\":%d, \"Humidity (%%)\":%d, \"Pressure (hPa)\":%d.%d}",
		   bme280_sensor.value(BME280_SENSOR_TEMP),
		   bme280_sensor.value(BME280_SENSOR_HUMIDITY), 
		   bme280_sensor.value(BME280_SENSOR_PRESSURE)/10,
		   bme280_sensor.value(BME280_SENSOR_PRESSURE) % 10);

    if(len < 0 || len >= remaining) {
      printf("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
      return;
    }
    remaining -= len;
    buf_ptr += len;
  }

  len = snprintf(buf_ptr, remaining, "}}");

  remaining -= len;
  buf_ptr += len;

  if(len < 0 || len >= remaining) {
    printf("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
    return;
  }

  printf("MQTT publish %d:\n", seq_nr_value);
  mqtt_publish(&conn, NULL, pub_topic, (uint8_t *)app_buffer,
               strlen(app_buffer), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);
  mqtt_stats.published++;
  DBG("APP - Publish!\n");
}

#elif defined(MQTT_PUBLISH_SENML)

#define PUTFMT(...) { \
		len = snprintf(buf_ptr, remaining, __VA_ARGS__);	\
		if (len < 0 || len >= remaining) { \
			printf("Line %d: Buffer too short. Have %d, need %d + \\0", __LINE__, remaining, len); \
			return; \
		} \
		remaining -= len; \
		buf_ptr += len; \
	}

static void
publish(void)
{
  /* Publish MQTT topic in SenML format */
  /* Use device URN as base name -- draft-arkko-core-dev-urn-03 */

  int len;
  int remaining = APP_BUFFER_SIZE;

  seq_nr_value++;

  buf_ptr = app_buffer;

  uip_ipaddr_t loc_fipaddr; /* Link local address - use 8 last bytes for ID */
  uint8_t *ll; 
  uip_create_linklocal_prefix(&loc_fipaddr);
  uip_ds6_set_addr_iid(&loc_fipaddr, &uip_lladdr);
  ll = (uint8_t *) &loc_fipaddr;
  ll = (uint8_t *) &linkaddr_node_addr.u8;

  PUTFMT("[{\"bn\":\"urn:dev:mac:%02x%02x%02x%02x%02x%02x%02x%02x;\"", ll[0], ll[1], ll[2], ll[3], ll[4], ll[5], ll[6], ll[7]);
  PUTFMT(",\"bt\":%lu}", clock_seconds());
  PUTFMT(",{\"n\":\"seq_no\", \"v\":%d}", seq_nr_value);
  PUTFMT(",{\"n\":\"battery\", \"u\":\"V\",\"v\":%-5.2f}", ((double) battery_sensor.value(0)/1000.));

  /* Put our Default route's string representation in a buffer */
  char def_rt_str[64];
  memset(def_rt_str, 0, sizeof(def_rt_str));
  ipaddr_sprintf(def_rt_str, sizeof(def_rt_str), uip_ds6_defrt_choose());

  PUTFMT(",{\"n\":\"def_route\",\"v\":\"%s\"}", def_rt_str);
  PUTFMT(",{\"n\":\"rssi\",\"u\":\"dBm\",\"v\":%lu}", def_rt_rssi);

#ifdef CO2
  PUTFMT(",{\"n\":\"co2\",\"u\":\"ppm\",\"v\":%d}", co2_sa_kxx_sensor.value(CO2_SA_KXX_CO2));
#endif

  PUTFMT(",{\"n\":\"pms5003;tsi;pm1\",\"u\":\"ug/m3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_PM1));
  PUTFMT(",{\"n\":\"pms5003;tsi;pm2_5\",\"u\":\"ug/m3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_PM2_5));
  PUTFMT(",{\"n\":\"pms5003;tsi;pm10\",\"u\":\"ug/m3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_PM10));

  PUTFMT(",{\"n\":\"pms5003;atm;pm1\",\"u\":\"ug/m3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_PM1_ATM));
  PUTFMT(",{\"n\":\"pms5003;atm;pm2_5\",\"u\":\"ug/m3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_PM2_5_ATM));
  PUTFMT(",{\"n\":\"pms5003;atm;pm10\",\"u\":\"ug/m3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_PM10_ATM));  

  extern uint32_t pms5003_valid_frames();
  extern uint32_t pms5003_invalid_frames();

  PUTFMT(",{\"n\":\"pms5003;valid_frames\",\"v\":%lu}", pms5003_valid_frames());
  PUTFMT(",{\"n\":\"pms5003;invalid_frames\",\"v\":%lu}", pms5003_invalid_frames());

  if( i2c_probed & I2C_BME280 ) {
    PUTFMT(",{\"n\":\"bme280;temp\",\"u\":\"Cel\",\"v\":%d}", bme280_sensor.value(BME280_SENSOR_TEMP));
    PUTFMT(",{\"n\":\"bme280;humidity\",\"u\":\"%%RH\",\"v\":%d}", bme280_sensor.value(BME280_SENSOR_HUMIDITY));
    PUTFMT(",{\"n\":\"bme280;pressure\",\"u\":\"hPa\",\"v\":%d.%d}", bme280_sensor.value(BME280_SENSOR_PRESSURE)/10,
	   bme280_sensor.value(BME280_SENSOR_PRESSURE) % 10);
  }

  PUTFMT("]");

  printf("MQTT publish %d:\n", seq_nr_value);
  printf("%s\n", app_buffer);
  mqtt_publish(&conn, NULL, pub_topic, (uint8_t *)app_buffer,
               strlen(app_buffer), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);

  DBG("APP - Publish!\n");
}
#endif

/*---------------------------------------------------------------------------*/
static void
connect_to_broker(void)
{
  /* Connect to MQTT server */
  mqtt_connect(&conn, conf.broker_ip, conf.broker_port,
               conf.pub_interval * 3);

  state = STATE_CONNECTING;
}
/*---------------------------------------------------------------------------*/
static void
ping_parent(void)
{
  if(uip_ds6_get_global(ADDR_PREFERRED) == NULL) {
    return;
  }

  uip_icmp6_send(uip_ds6_defrt_choose(), ICMP6_ECHO_REQUEST, 0,
                 ECHO_REQ_PAYLOAD_LEN);
}
/*---------------------------------------------------------------------------*/
static void
state_machine(void)
{
  switch(state) {
  case STATE_INIT:
    /* If we have just been configured register MQTT connection */
    mqtt_register(&conn, &mqtt_demo_process, client_id, mqtt_event,
                  MAX_TCP_SEGMENT_SIZE);

    /*
     * If we are not using the quickstart service (thus we are an IBM
     * registered device), we need to provide user name and password
     */
    if(strncasecmp(conf.org_id, QUICKSTART, strlen(conf.org_id)) != 0) {
      if(strlen(conf.auth_token) == 0) {
        printf("User name set, but empty auth token\n");
        state = STATE_ERROR;
        break;
      } else {
        mqtt_set_username_password(&conn, "use-token-auth",
                                   conf.auth_token);
      }
    }

    /* _register() will set auto_reconnect. We don't want that. */
    conn.auto_reconnect = 0;
    connect_attempt = 1;

    state = STATE_REGISTERED;
    DBG("Init\n");
    /* Continue */
  case STATE_REGISTERED:
    if(uip_ds6_get_global(ADDR_PREFERRED) != NULL) {
      /* Registered and with a public IP. Connect */
      DBG("Registered. Connect attempt %u\n", connect_attempt);
      ping_parent();
      connect_to_broker();
    } else {
      leds_on(STATUS_LED);
      ctimer_set(&ct, NO_NET_LED_DURATION, publish_led_off, NULL);
    }
    etimer_set(&publish_periodic_timer, NET_CONNECT_PERIODIC);
    return;
    break;
  case STATE_CONNECTING:
    leds_on(STATUS_LED);
    ctimer_set(&ct, CONNECTING_LED_DURATION, publish_led_off, NULL);
    /* Not connected yet. Wait */
    DBG("Connecting (%u)\n", connect_attempt);
    break;
  case STATE_CONNECTED:
    /* Don't subscribe unless we are a registered device */
    if(strncasecmp(conf.org_id, QUICKSTART, strlen(conf.org_id)) == 0) {
      DBG("Using 'quickstart': Skipping subscribe\n");
      state = STATE_PUBLISHING;
    }
    /* Continue */
  case STATE_PUBLISHING:
    /* If the timer expired, the connection is stable. */
    if(timer_expired(&connection_life)) {
      /*
       * Intentionally using 0 here instead of 1: We want RECONNECT_ATTEMPTS
       * attempts if we disconnect after a successful connect
       */
      connect_attempt = 0;
    }

    if(mqtt_ready(&conn) && conn.out_buffer_sent) {
      /* Connected. Publish */
      if(state == STATE_CONNECTED) {
        subscribe();
        state = STATE_PUBLISHING;
      } else {
        leds_on(STATUS_LED);
        ctimer_set(&ct, PUBLISH_LED_ON_DURATION, publish_led_off, NULL);
        publish();
      }
      etimer_set(&publish_periodic_timer, conf.pub_interval);

      DBG("Publishing\n");
      /* Return here so we don't end up rescheduling the timer */
      return;
    } else {
      /*
       * Our publish timer fired, but some MQTT packet is already in flight
       * (either not sent at all, or sent but not fully ACKd).
       *
       * This can mean that we have lost connectivity to our broker or that
       * simply there is some network delay. In both cases, we refuse to
       * trigger a new message and we wait for TCP to either ACK the entire
       * packet after retries, or to timeout and notify us.
       */
#if 0
      DBG("Publishing... (MQTT state=%d, q=%u)\n", conn.state,
          conn.out_queue_full);
#else
      DBG("Publishing... (MQTT state=%d, q=%u) mqtt_ready %d out_buffer_sent %d\n", conn.state,
          conn.out_queue_full, mqtt_ready(&conn), conn.out_buffer_sent);
#endif      
    }
    break;
  case STATE_DISCONNECTED:
    DBG("Disconnected\n");
    if(connect_attempt < RECONNECT_ATTEMPTS ||
       RECONNECT_ATTEMPTS == RETRY_FOREVER) {
      /* Disconnect and backoff */
      clock_time_t interval;
      mqtt_disconnect(&conn);
      connect_attempt++;

      interval = connect_attempt < 3 ? RECONNECT_INTERVAL << connect_attempt :
        RECONNECT_INTERVAL << 3;

      DBG("Disconnected. Attempt %u in %lu ticks\n", connect_attempt, interval);

      etimer_set(&publish_periodic_timer, interval);

      state = STATE_REGISTERED;
      return;
    } else {
      /* Max reconnect attempts reached. Enter error state */
      state = STATE_ERROR;
      DBG("Aborting connection after %u attempts\n", connect_attempt - 1);
    }
    break;
  case STATE_CONFIG_ERROR:
    /* Idle away. The only way out is a new config */
    printf("Bad configuration.\n");
    return;
  case STATE_ERROR:
  default:
    leds_on(STATUS_LED);
    /*
     * 'default' should never happen.
     *
     * If we enter here it's because of some error. Stop timers. The only thing
     * that can bring us out is a new config event
     */
    printf("Default case: State=0x%02x\n", state);
    return;
  }

  /* If we didn't return so far, reschedule ourselves */
  etimer_set(&publish_periodic_timer, STATE_MACHINE_PERIODIC);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mqtt_demo_process, ev, data)
{

  PROCESS_BEGIN();

  SENSORS_ACTIVATE(temp_sensor);
  SENSORS_ACTIVATE(battery_sensor);
#ifdef CO2
  SENSORS_ACTIVATE(co2_sa_kxx_sensor);
#endif
  leds_init(); 
  SENSORS_ACTIVATE(pulse_sensor);
  SENSORS_ACTIVATE(pms5003_sensor);
  if( i2c_probed & I2C_BME280 ) {
    SENSORS_ACTIVATE(bme280_sensor);
  }
  
  /* The data sink runs with a 100% duty cycle in order to ensure high 
     packet reception rates. */
  //NETSTACK_MAC.off(1);

  printf("MQTT Demo Process\n");

  if(init_config() != 1) {
    PROCESS_EXIT();
  }

  update_config();

  def_rt_rssi = 0x8000000;
  uip_icmp6_echo_reply_callback_add(&echo_reply_notification,
                                    echo_reply_handler);
  etimer_set(&echo_request_timer, conf.def_rt_ping_interval);

  /* Main loop */
  while(1) {

    PROCESS_YIELD();

    if(ev == sensors_event && data == PUBLISH_TRIGGER) {
      if(state == STATE_ERROR) {
        connect_attempt = 1;
        state = STATE_REGISTERED;
      }
    }

    if((ev == PROCESS_EVENT_TIMER && data == &publish_periodic_timer) ||
       ev == PROCESS_EVENT_POLL ||
       (ev == sensors_event && data == PUBLISH_TRIGGER)) {
      state_machine();
    }

    if(ev == PROCESS_EVENT_TIMER && data == &echo_request_timer) {
      ping_parent();
      etimer_set(&echo_request_timer, conf.def_rt_ping_interval);
    }

    if (ev == serial_line_event_message && data != NULL) {
      handle_serial_input((const char *) data);
    }
  }
  PROCESS_END();
}

#ifdef CHECKER
static struct etimer checktimer;

#define WATCHDOG_INTERVAL (CLOCK_SECOND*30)
/* Watchdogs in WATCHDOG_INTERVAL units: */
#define STALE_PUBLISHING_WATCHDOG 10
#define STALE_CONNECTING_WATCHDOG 20

static struct {
  unsigned int stale_publishing; 
  unsigned int stale_connecting;
  unsigned int closed_connection;
} watchdog_stats = {0, 0, 0};

PROCESS_THREAD(mqtt_checker_process, ev, data)
{
  static uint16_t stale_publishing = 0, stale_connecting = 0;
  static uint16_t seen_seq_nr_value = 0;

  PROCESS_BEGIN();
  etimer_set(&checktimer, WATCHDOG_INTERVAL);

  /* Main loop */
  while(1) {

    PROCESS_YIELD();

    if((ev == PROCESS_EVENT_TIMER) && (data == &checktimer)) {
      printf("MQTT: state %d conn.state %d", state, conn.state);
      if (state == STATE_PUBLISHING) { 
       stale_connecting = 0;
       if (seq_nr_value > seen_seq_nr_value) {
         stale_publishing = 0;
       }
       else {
         stale_publishing++;
         if (stale_publishing > STALE_PUBLISHING_WATCHDOG) {
           /* In publishing state, but nothing published for a while. 
            * Milder reset -- call mqtt_disconnect() to trigger mqtt to restart the session
            */
           mqtt_disconnect(&conn);
           stale_publishing = 0;
	   watchdog_stats.stale_publishing++;
         }
       }
       seen_seq_nr_value = seq_nr_value;
      }
      else {
	stale_connecting++;
       if (stale_connecting > STALE_CONNECTING_WATCHDOG) {
         if(conn.state > MQTT_CONN_STATE_NOT_CONNECTED) {
           /* Waiting for mqtt connection, but nothing happened for a while.
            * Trigger communication error by closing TCP socket 
            */
           tcp_socket_close(&conn.socket);
	   watchdog_stats.stale_connecting++;
         }       
         else {
	   watchdog_stats.closed_connection++;	   
	 }
         stale_connecting = 0;
       }
      }
      etimer_reset(&checktimer);
    }
  }
  PROCESS_END();
}

#endif /* CHECKER */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

