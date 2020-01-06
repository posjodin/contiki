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
#ifdef CONTIKI_TARGET_AVR_RSS2
#include <avr/pgmspace.h>
#endif /* CONTIKI_TARGET_AVR_RSS2 */

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
#include "dev/sht2x/sht2x.h"
#include "dev/serial-line.h"
#include "watchdog.h"
#ifndef RF230_DEBUG
#define RF230_DEBUG 0
#else
#if RF230_DEBUG
#include "radio/rf230bb/rf230bb.h"
#endif /* #if RF230_DEBUG */
#endif /* #ifndef RF230_DEBUG */


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
/* MQTT QoS Level */
#ifndef MQTT_QOS_LEVEL
#define MQTT_QOS_LEVEL MQTT_QOS_LEVEL_0
#endif /* MQTT_QOS_LEVEL */
/*---------------------------------------------------------------------------*/
/* Various states */
static uint8_t state;
#define STATE_INIT            0
#define STATE_REGISTERED      1
#define STATE_CONNECTING      2
#define STATE_CONNECTED       3
#define STATE_PUBLISHING      4
#define STATE_DISCONNECTED    5
#define STATE_NEWCONFIG       6 /* Not used? */
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
#ifdef MQTT_WATCHDOG
static struct etimer checktimer;

#define WATCHDOG_INTERVAL (CLOCK_SECOND*30)
/* Watchdogs in WATCHDOG_INTERVAL units: */
#define STALE_PUBLISHING_WATCHDOG 10
#define STALE_TCP_CONNECTING_WATCHDOG 20
#define STALE_CONNECTING_WATCHDOG 10
static struct {
  unsigned int stale_publishing; 
  unsigned int stale_tcp_connecting;
  unsigned int stale_connecting;
} watchdog_stats = {0, 0};
#endif /* MQTT_WATCHDOG */
/* Publish statistics every N publication */
#define PUBLISH_STATS_INTERVAL 8

extern int sim7020_rssi_to_dbm(int);

/* Connection management: publish then disconnect */
/* #define MQTT_PUBLISH_DISCONNECT */
/*---------------------------------------------------------------------------*/
extern int
mqtt_rpl_pub(char *buf, int bufsize);
extern int
mqtt_i2c_pub(char *buf, int bufsize);
/*---------------------------------------------------------------------------*/

PROCESS_NAME(mqtt_client_process);
PROCESS(mqtt_checker_process, "MQTT state checker for debug");

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
  clock_time_t pub_interval; /* in ticks */
  uint16_t keep_alive_timer; /* in secs */
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
 * Buffers for Client ID
 * Make sure they are large enough to hold the entire respective string
 *
 * d:quickstart:status:EUI64 is 32 bytes long
 * iot-2/evt/status/fmt/json is 25 bytes
 * We also need space for the null termination
 */
#define BUFFER_SIZE 64
static char client_id[BUFFER_SIZE];

/*
 * Node id string -- 8 bytes in hex plus null
 */
#define NODEID_SIZE 17
static char node_id[NODEID_SIZE];

/* Message for immediate publishing, without waiting for timer */
static char *pub_now_message = NULL;
static char *pub_now_topic;
/*---------------------------------------------------------------------------*/
/*
 * The main MQTT buffers.
 * We will need to increase if we start publishing more data.
 */
#if RF230_DEBUG || RPL_CONF_STATS 
/* increase buffer size when debug/statistics is enabled */
#define APP_BUFFER_SIZE 2048
#else
#define APP_BUFFER_SIZE 1024
#endif
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
#ifdef MQTT_CLI
extern char *mqtt_cli_input(const char *);
#endif /* MQTT_CLI */
/*---------------------------------------------------------------------------*/
/* Parent RSSI functionality */
static struct uip_icmp6_echo_reply_notification echo_reply_notification;
static struct etimer echo_request_timer;
static unsigned long def_rt_rssi = 0;
/*---------------------------------------------------------------------------*/
static char *construct_topic(char *suffix);
/*---------------------------------------------------------------------------*/
static mqtt_client_config_t conf;
/*---------------------------------------------------------------------------*/
PROCESS(mqtt_client_process, "MQTT client");
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

#define CCA_TRY 100
uint8_t cca[16];
static radio_value_t
get_chan(void)
{
  radio_value_t chan;
  if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan) ==
     RADIO_RESULT_OK) {
    return chan;
  }
  return 0;
}
static void
set_chan(uint8_t chan)
{
  if(NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, chan) ==
     RADIO_RESULT_OK) {
  }
}

extern bool rf230_blackhole_rx;

void 
do_all_chan_cca(uint8_t *cca)
{
  int i, j;
  uint8_t old_chan;
  uint8_t sreg = SREG;
  cli();
  old_chan = get_chan();
  rf230_blackhole_rx = 1;
  for(j = 0; j < 16; j++) {
    set_chan(j+11);
    cca[j] = 0;
#ifdef CONTIKI_TARGET_AVR_RSS2
    watchdog_periodic();
#endif
    for(i = 0; i < CCA_TRY; i++) {
      cca[j] += NETSTACK_RADIO.channel_clear();
    }
  }
  set_chan(old_chan);
  rf230_blackhole_rx = 0;
  SREG = sreg;
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
  
#ifdef MQTT_CLI
  char *cmd_topic, *reply_topic;
#endif /* MQTT_CLI */
  
  DBG("Pub Handler: topic='%s' (len=%u), chunk_len=%u\n", topic, topic_len,
      chunk_len);

  /* If we don't like the length, ignore */
  if(topic_len != strlen(topic)) {
    printf("Incorrect topic length (%d). Ignored\n", topic_len);

    return;
  }

  if (chunk[chunk_len-1] != 0 && chunk[chunk_len] != 0) {
    printf("MQTT chunk not null terminated\n");  
    return;
  }

#ifdef MQTT_CLI
  cmd_topic = construct_topic("cli/cmd");
  if (strcmp(cmd_topic, topic) == 0) {
    reply_topic = construct_topic("cli/reply");
    if (reply_topic) {
      /* This will oerwrite any pending reply with current --
       * last command gets priority. I think this is what
       * we want?
       */
      pub_now_message = mqtt_cli_input((char *) chunk);
      pub_now_topic = reply_topic;
    }
  }
  else {
    printf("Ignoring pub. topic %s\n", topic);
  }
#endif /* MQTT_CLI */
} 

static struct mqtt_app_statistics {
  unsigned int connected;
  unsigned int disconnected;
  unsigned int published;
  unsigned int pubacked;
  unsigned int subscribed;
} mqtt_stats = {0, 0, 0, 0, 0};

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
    process_poll(&mqtt_client_process);
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
    mqtt_stats.subscribed++;
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
#ifdef MQTT_PUBLISH_DISCONNECT
  mqtt_disconnect(&conn);
#endif /* MQTT_PUBLISH_DISCONNECT */
    break;
  }
  default:
    DBG("APP - Application got a unhandled MQTT event: %i\n", event);
    break;
  }
}
/*---------------------------------------------------------------------------*/
static char *
construct_topic(char *suffix)
{
  static char buf[BUFFER_SIZE];
  
  int len = snprintf(buf, sizeof(buf), "%s/%s/%s", MQTT_DEMO_TOPIC_BASE, node_id, suffix);

  /* len < 0: Error. Len >= BUFFER_SIZE: Buffer too small */
  if(len < 0 || len >= sizeof(buf)) {
    printf("Topic buffer too small: %d -- %d\n", len, BUFFER_SIZE);
    return NULL;
  }
  return buf;
}
/*---------------------------------------------------------------------------*/
static int
construct_client_id(void)
{
  int len = snprintf(client_id, BUFFER_SIZE, "%s:%s", conf.type_id, node_id);

  /* len < 0: Error. Len >= BUFFER_SIZE: Buffer too small */
  if(len < 0 || len >= BUFFER_SIZE) {
    printf("Client id: %d -- %d\n", len, BUFFER_SIZE);
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
construct_node_id(void)
{
int len = snprintf(node_id, NODEID_SIZE, 
			   "%02x%02x%02x%02x%02x%02x%02x%02x",
			   linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
			   linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[3],
			   linkaddr_node_addr.u8[4], linkaddr_node_addr.u8[5],
			   linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);

  /* len < 0: Error. Len >= BUFFER_SIZE: Buffer too small */
if(len < 0 || len >= BUFFER_SIZE) {
    printf("Node id: %d -- %d\n", len, BUFFER_SIZE);
    return 0;
  }
  return 1;
}

/*---------------------------------------------------------------------------*/
static void
update_config(void)
{
  if(construct_node_id() == 0) {
    /* Fatal error. Client ID larger than the buffer */
    state = STATE_CONFIG_ERROR;
    return;
  }

  if(construct_client_id() == 0) {
    /* Fatal error. Client ID larger than the buffer */
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

struct {
  uint8_t dustbin;
  uint8_t cca_test;
} lc;

/*---------------------------------------------------------------------------*/
static void
init_node_local_config()
{
  unsigned char node_mac[8];
  unsigned char n06aa[8] = { 0xfc, 0xc2, 0x3d, 0x00, 0x00, 0x01, 0x06, 0xaa }; /* Stadhus north side - has NO2 sensor */
  unsigned char n050f[8] = { 0xfc, 0xc2, 0x3d, 0x00, 0x00, 0x00, 0x05, 0x0f }; /* Stadhus south side - no NO2 sensor */
  unsigned char n63a7[8] = { 0xfc, 0xc2, 0x3d, 0x00, 0x00, 0x01, 0x63, 0xa7 }; /* SLB station - has NO2 sensor */
  unsigned char n8554[8] = { 0xfc, 0xc2, 0x3d, 0x00, 0x00, 0x01, 0x85, 0x54 }; /* SLB station - has NO2 sensor */
  unsigned char n837e[8] = { 0xfc, 0xc2, 0x3d, 0x00, 0x00, 0x01, 0x83, 0x7e }; /* RO test */
  unsigned char n1242[8] = { 0xfc, 0xc2, 0x3d, 0x00, 0x00, 0x00, 0x12, 0x42 }; /* lab node */

  memcpy(node_mac, &uip_lladdr.addr, sizeof(linkaddr_t));

  if(memcmp(node_mac, n06aa, 8) == 0) {
    lc.dustbin = 1;
    lc.cca_test = 1;
  }
  else if(memcmp(node_mac, n050f, 8) == 0) {
    lc.dustbin = 1;
    lc.cca_test = 1;
  }
  else if(memcmp(node_mac, n63a7, 8) == 0) {
    lc.dustbin = 1; /* 63a7 is at SLB station with dustbin enabled */
    lc.cca_test = 1;
  }
  else if(memcmp(node_mac, n8554, 8) == 0) {
    lc.dustbin = 1; /* 63a7 is at SLB station with dustbin enabled */
    lc.cca_test = 1;
  }
  else if(memcmp(node_mac, n837e, 8) == 0) {
    lc.dustbin = 0; /*  */
    lc.cca_test = 0;
  }
  else if(memcmp(node_mac, n1242, 8) == 0) {
    lc.dustbin = 1; /*  */
    lc.cca_test = 0;
  }
  else {
    lc.dustbin = 0;
    lc.cca_test = 0;
  }
  printf("Local node settings: Dustbin=%d, CCA_TEST=%d\n", lc.dustbin, lc.cca_test);
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
  printf("MQTT: Use broker_ip %s\n", broker_ip);
  memcpy(conf.cmd_type, DEFAULT_SUBSCRIBE_CMD_TYPE, 1);

  conf.broker_port = DEFAULT_BROKER_PORT;
#ifdef MQTT_CONF_PUBLISH_INTERVAL
  conf.pub_interval = MQTT_CONF_PUBLISH_INTERVAL;
#else
  conf.pub_interval = DEFAULT_PUBLISH_INTERVAL;
#endif /* MQTT_CONF_PUBLISH_INTERVAL */
  conf.pub_interval = 10*CLOCK_SECOND;

#ifdef MQTT_CONF_KEEP_ALIVE_TIMER
  conf.keep_alive_timer = MQTT_CONF_KEEP_ALIVE_TIMER;
#else
  conf.keep_alive_timer = DEFAULT_KEEP_ALIVE_TIMER;
#endif /* MQTT_CONF_KEEP_ALIVE_TIMER */ 
  conf.def_rt_ping_interval = DEFAULT_RSSI_MEAS_INTERVAL;

  init_node_local_config();
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
subscribe(void)
{
#ifdef MQTT_CLI
  /* Publish MQTT topic in IBM quickstart format */
  mqtt_status_t status;
  char *topic;
  
  topic = construct_topic("cli/cmd");
  if (topic) {
    status = mqtt_subscribe(&conn, NULL, topic, MQTT_QOS_LEVEL_0);
    DBG("APP - Subscribing to %s!\n", topic);
    if(status == MQTT_STATUS_OUT_QUEUE_FULL) {
      DBG("APP - Tried to subscribe but command queue was full!\n");
    }
  }
  else {
        state = STATE_CONFIG_ERROR;
  }
#endif /* MQTT_CLI */
}
/*---------------------------------------------------------------------------*/
#define PUTFMT(FORMAT, ...) {                                               \
    len = snprintf_P(buf_ptr, remaining, PSTR(FORMAT), ##__VA_ARGS__);        \
    if (len < 0 || len >= remaining) { \
        printf_P(PSTR("Line %d: Buffer too small. Have %d, need %d + \\0"), __LINE__, remaining, len); \
        return; \
    } \
    remaining -= len; \
    buf_ptr += len; \
}

static void
publish_sensors(void)
{
  /* Publish MQTT topic in SenML format */

  int len;
  int remaining = APP_BUFFER_SIZE;
  char *topic;
  buf_ptr = app_buffer;

  seq_nr_value++;

  /* Use device URN as base name -- draft-arkko-core-dev-urn-03 */
  PUTFMT("[{\"bn\":\"urn:dev:mac:%s;\"", node_id);
  PUTFMT(",\"bt\":%lu}", clock_seconds());

#ifdef CO2
  PUTFMT(",{\"n\":\"co2\",\"u\":\"ppm\",\"v\":%d}", co2_sa_kxx_sensor.value(CO2_SA_KXX_CO2));
#endif

  if (pms5003_sensor.value(PMS5003_SENSOR_TIMESTAMP) != 0) {
    PUTFMT(",{\"n\":\"pms5003;tsi;pm1\",\"u\":\"ug/m3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_PM1));
    PUTFMT(",{\"n\":\"pms5003;tsi;pm2_5\",\"u\":\"ug/m3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_PM2_5));
    PUTFMT(",{\"n\":\"pms5003;tsi;pm10\",\"u\":\"ug/m3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_PM10));
    PUTFMT(",{\"n\":\"pms5003;atm;pm1\",\"u\":\"ug/m3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_PM1_ATM));
    PUTFMT(",{\"n\":\"pms5003;atm;pm2_5\",\"u\":\"ug/m3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_PM2_5_ATM));
    PUTFMT(",{\"n\":\"pms5003;atm;pm10\",\"u\":\"ug/m3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_PM10_ATM));

    if(lc.dustbin) {
      PUTFMT(",{\"n\":\"pms5003;db;0_3\",\"u\":\"cnt/dm3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_DB0_3));
      PUTFMT(",{\"n\":\"pms5003;db;0_5\",\"u\":\"cnt/dm3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_DB0_5));
      PUTFMT(",{\"n\":\"pms5003;db;1\",\"u\":\"cnt/dm3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_DB1));
      PUTFMT(",{\"n\":\"pms5003;db;2_5\",\"u\":\"cnt/dm3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_DB2_5));
      PUTFMT(",{\"n\":\"pms5003;db;5\",\"u\":\"cnt/dm3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_DB5));
      PUTFMT(",{\"n\":\"pms5003;db;10\",\"u\":\"cnt/dm3\",\"v\":%d}", pms5003_sensor.value(PMS5003_SENSOR_DB10));
    }

  }

  if( i2c_probed & I2C_BME280 ) {
    bme280_sensor.value(BME280_SENSOR_TEMP);
    PUTFMT(",{\"n\":\"bme280;temp\",\"u\":\"Cel\",\"v\":%4.2f}", (double)bme280_mea.t_overscale100/100.0);
    PUTFMT(",{\"n\":\"bme280;humidity\",\"u\":\"%%RH\",\"v\":%4.2f}", (double)bme280_mea.h_overscale1024 / 1024.0);
#ifdef BME280_64BIT
    PUTFMT(",{\"n\":\"bme280;pressure\",\"u\":\"hPa\",\"v\":%4.2f}", (double)bme280_mea.p_overscale256/ (256.0*100));
#else
    PUTFMT(",{\"n\":\"bme280;pressure\",\"u\":\"hPa\",\"v\":%4.2f}", (double)bme280_mea.p);
#endif
  }

  if( i2c_probed & I2C_SHT2X ) {
    PUTFMT(",{\"n\":\"sht2x;temp\",\"u\":\"Cel\",\"v\":%d}", sht2x_sensor.value(0));
    PUTFMT(",{\"n\":\"sht2x;humidity\",\"u\":\"%%RH\",\"v\":%d}", sht2x_sensor.value(1));
  }

#ifdef MQTT_AT_RADIO
  {
    struct at_radio_status *status;
    int lb = -113;
    status = at_radio_status();
    if((status->rssi) > 0 )
      lb += (status->rssi)*2;  /* In dBm acording to A6/A7 datasheet */
    PUTFMT(",{\"n\":\"at_radio;linkbudget\",\"v\":%d}", lb);
  }
  {
    PUTFMT(",{\"n\":\"at_radio;linkbudget\",\"u\":\"dBm\",\"v\":%d}", sim7020_rssi_to_dbm(status.rssi));
  }

#endif

  
  PUTFMT("]");

  DBG("MQTT publish sensors %d: %d bytes\n", seq_nr_value, strlen(app_buffer));
  //printf("%s\n", app_buffer);
  topic = construct_topic("sensors");
  mqtt_publish(&conn, NULL, topic, (uint8_t *)app_buffer,
               strlen(app_buffer), MQTT_QOS_LEVEL, MQTT_RETAIN_OFF);
/*
  if(lc.cca_test) {
    int i; 
    do_all_chan_cca(cca);
    printf(" CCA: ");
    for(i = 0; i < 16; i++) {
      printf(" %3d", 100-cca[i]);
    }
    printf("\n");
  }
*/
}

static void
publish_stats(void)
{
  /* Publish MQTT topic in SenML format */
  /* Circle through different statistics -- one for each publish */
  enum {
    STATS_DEVICE,
#if AT_RADIO_CONF_STATS
    STATS_AT_RADIO,
#endif /* AT_RADIO_CONF_STATS */
    STATS_RPL,
  };
#define STARTSTATS STATS_DEVICE
#define ENDSTATS STATS_RPL

  static int stats = STARTSTATS;
  int len;
  int remaining = APP_BUFFER_SIZE;
  char *topic;
  
  buf_ptr = app_buffer;

  seq_nr_value++;

  /* Use device URN as base name -- draft-arkko-core-dev-urn-03 */
  PUTFMT("[{\"bn\":\"urn:dev:mac:%s;\"", node_id);
  PUTFMT(",\"bu\":\"count\"");
  PUTFMT(",\"bt\":%lu}", clock_seconds());

  PUTFMT(",{\"n\":\"seq_no\",\"u\":\"count\",\"v\":%d}", seq_nr_value);
  switch (stats) {
  case STATS_DEVICE:

    PUTFMT(",{\"n\":\"battery\", \"u\":\"V\",\"v\":%-5.2f}", ((double) battery_sensor.value(0)/1000.));

    /* Put our Default route's string representation in a buffer */
    char def_rt_str[64];
    memset(def_rt_str, 0, sizeof(def_rt_str));
    ipaddr_sprintf(def_rt_str, sizeof(def_rt_str), uip_ds6_defrt_choose());

#ifdef AT_RADIO_SOCKETS
    PUTFMT(",{\"n\":\"def_route\",\"vs\":\"%s\"}", at_radio_status()->ipaddr);
#else
    PUTFMT(",{\"n\":\"def_route\",\"vs\":\"%s\"}", def_rt_str);    
#endif /* AT_RADIO_SOCKETS */
    PUTFMT(",{\"n\":\"rssi\",\"u\":\"dBm\",\"v\":%lu}", def_rt_rssi);

    extern uint32_t pms5003_valid_frames();
    extern uint32_t pms5003_invalid_frames();

    PUTFMT(",{\"n\":\"pms5003;valid\",\"v\":%lu}", pms5003_valid_frames());
    PUTFMT(",{\"n\":\"pms5003;invalid\",\"v\":%lu}", pms5003_invalid_frames());

#if RF230_DEBUG
    PUTFMT(",{\"n\":\"rf230;no_ack\",\"v\":%u}", count_no_ack);
    PUTFMT(",{\"n\":\"rf230;cca_fail\",\"v\":%u}", count_cca_fail);
#endif
    
     /* case STATS_MQTT:*/
     
    PUTFMT(",{\"n\":\"mqtt;conn\",\"v\":%u}", mqtt_stats.connected);
    PUTFMT(",{\"n\":\"mqtt;disc\",\"v\":%u}", mqtt_stats.disconnected);
    PUTFMT(",{\"n\":\"mqtt;pub\",\"v\":%u}", mqtt_stats.published);
    PUTFMT(",{\"n\":\"mqtt;puback\",\"v\":%u}", mqtt_stats.pubacked);

#ifdef MQTT_WATCHDOG
    PUTFMT(",{\"n\":\"mqtt;wd;stale_pub\",\"v\":%u}", watchdog_stats.stale_publishing);
    PUTFMT(",{\"n\":\"mqtt;wd;stale_tcpconn\",\"v\":%u}", watchdog_stats.stale_tcp_connecting);
    PUTFMT(",{\"n\":\"mqtt;wd;stale_conn\",\"v\":%u}", watchdog_stats.stale_connecting);    
#endif

#ifdef CONTIKI_TARGET_AVR_RSS2
    /* Send bootcause 3 times after reboot (in the first 20 min after reboot) */
    if (seq_nr_value < 40) {
      PUTFMT(",{\"n\":\"bootcause\",\"v\":\"%02x\"}", GPIOR0);
    }
#endif

    PUTFMT(",");
    len = mqtt_i2c_pub(buf_ptr, remaining);
    if (len < 0 || len >= remaining) { 
      printf("Line %d: Buffer too short. Have %d, need %d + \\0", __LINE__, remaining, len); 
      return;
    }
    remaining -= len;
    buf_ptr += len;
    break;
  case STATS_RPL:
#if RPL_CONF_STATS
    PUTFMT(",{\"n\":\"rpl;mem_overflows\",\"v\":%u}", rpl_stats.mem_overflows);
    PUTFMT(",{\"n\":\"rpl;local_repairs\",\"v\":%u}", rpl_stats.local_repairs);
    PUTFMT(",{\"n\":\"rpl;global_repairs\",\"v\":%u}", rpl_stats.global_repairs);
    PUTFMT(",{\"n\":\"rpl;malformed_msgs\",\"v\":%u}", rpl_stats.malformed_msgs);
    PUTFMT(",{\"n\":\"rpl;resets\",\"v\":%u}", rpl_stats.resets);
    PUTFMT(",{\"n\":\"rpl;parent_switch\",\"v\":%u}", rpl_stats.parent_switch);
    PUTFMT(",{\"n\":\"rpl;forward_errors\",\"v\":%u}", rpl_stats.forward_errors);
    PUTFMT(",{\"n\":\"rpl;loop_errors\",\"v\":%u}", rpl_stats.loop_errors);
    PUTFMT(",{\"n\":\"rpl;loop_warnings\",\"v\":%u}", rpl_stats.loop_warnings);
    PUTFMT(",{\"n\":\"rpl;root_repairs\",\"v\":%u}", rpl_stats.root_repairs);
#endif
    PUTFMT(",");
    len = mqtt_rpl_pub(buf_ptr, remaining);
    if (len < 0 || len >= remaining) { 
      printf("Line %d: Buffer too short. Have %d, need %d + \\0", __LINE__, remaining, len); 
      return;
    }
    remaining -= len;
    buf_ptr += len;
    break;
#if AT_RADIO_CONF_STATS
  case STATS_AT_RADIO:
    PUTFMT(",{\"n\":\"at_radio;at_timeouts\",\"v\":%u}", at_radio_statistics.at_timeouts);
    PUTFMT(",{\"n\":\"at_radio;at_errors\",\"v\":%u}", at_radio_statistics.at_errors);
    PUTFMT(",{\"n\":\"at_radio;at_retries\",\"v\":%u}", at_radio_statistics.at_retries);
    PUTFMT(",{\"n\":\"at_radio;resets\",\"v\":%u}", at_radio_statistics.resets);
    PUTFMT(",{\"n\":\"at_radio;connections\",\"v\":%u}", at_radio_statistics.connections);
    PUTFMT(",{\"n\":\"at_radio;connfailed\",\"v\":%u}", at_radio_statistics.connfailed);
    {
      struct at_radio_status *status;
      status = at_radio_status();
      PUTFMT(",{\"n\":\"at_radio;module\",\"v\":%u}", status->module);
      PUTFMT(",{\"n\":\"at_radio;status\",\"vs\":"); 
      switch (status->state) {
      case AT_RADIO_STATE_NONE:
        PUTFMT("\"none\"}");
        break;
      case AT_RADIO_STATE_IDLE:
        PUTFMT("\"idle\"}");
        break;
      case AT_RADIO_STATE_REGISTERED:
        PUTFMT("\"registered\"}");
        break;
      case AT_RADIO_STATE_ACTIVE:
        PUTFMT("\"active\"}");
        break;
      default:
        PUTFMT("\"unknown (%d)\"}", status->state);
        break;
      }
      if (status->state == AT_RADIO_STATE_ACTIVE) {
        PUTFMT(",{\"n\":\"at_radio;local_ip\",\"vs\":\"%s\"}", status->ipaddr); 
      }
      PUTFMT(",{\"n\":\"at_radio;rssi\",\"v\":%u}", status->rssi);
      PUTFMT(",{\"n\":\"at_radio;gps_longi\",\"v\":%f}", status->longi);
      PUTFMT(",{\"n\":\"at_radio;gps_lat\",\"v\":%f}", status->lat);
      PUTFMT(",{\"n\":\"at_radio;gps_speed\",\"v\":%f}", status->speed);
      PUTFMT(",{\"n\":\"at_radio;gps_course\",\"v\":%f}", status->course);
    }
    break;
#endif /* AT_RADIO_CONF_STATS */
  }
  PUTFMT("]");

  DBG("MQTT publish stats part %d, seq %d, %d bytes:\n", stats, seq_nr_value, strlen(app_buffer));
  //printf("%s\n", app_buffer);
  topic = construct_topic("status");
  mqtt_publish(&conn, NULL, topic, (uint8_t *)app_buffer,
               strlen(app_buffer), MQTT_QOS_LEVEL, MQTT_RETAIN_OFF);
  if (++stats > ENDSTATS)
    stats = STARTSTATS;
  
}

static void
publish_now(void)
{
  /* Publish MQTT topic in SenML format */


  printf("-----Publish now to %s: \n%s\n", pub_now_topic, pub_now_message);
  if (pub_now_message) {
    mqtt_publish(&conn, NULL, pub_now_topic, (uint8_t *)pub_now_message,
                 strlen(pub_now_message), MQTT_QOS_LEVEL, MQTT_RETAIN_OFF);
    pub_now_message = NULL;
    pub_now_topic = NULL;
  }
}

static void
publish_cca_test(void)
{

  int len;
  int i;
  int remaining = APP_BUFFER_SIZE;
  char *topic;
  buf_ptr = app_buffer;

  seq_nr_value++;

  /* Publish MQTT topic not in SenML format */

    PUTFMT("%d", 100-cca[0]);
  for(i = 1; i < 16; i++) {
    PUTFMT(" %d", 100-cca[i]);
  }

  //DBG("MQTT publish CCA test, seq %d: %d bytes\n", seq_nr_value, strlen(app_buffer));
  printf("MQTT publish CCA test, seq %d: %d bytes\n", seq_nr_value, strlen(app_buffer));
  topic = construct_topic("cca_test");
  printf("TOPIC: %s\n", topic);
  printf("PAYLOAD: %s\n", app_buffer);
  mqtt_publish(&conn, NULL, topic, (uint8_t *)app_buffer,
               strlen(app_buffer), MQTT_QOS_LEVEL, MQTT_RETAIN_OFF);
}

#include "buildstring.h"
extern uint16_t bootcount;
static void
publish_build(void)
{

  int len;
  int remaining = APP_BUFFER_SIZE;
  char *topic;
  
  buf_ptr = app_buffer;

  seq_nr_value++;

  topic = construct_topic("build");
  /* Use device URN as base name -- draft-arkko-core-dev-urn-03 */
  PUTFMT("[{\"bn\":\"urn:dev:mac:%s;\"", node_id);
  PUTFMT(",\"bu\":\"count\"");
  PUTFMT(",\"bt\":%lu}", clock_seconds());
  PUTFMT(",{\"n\":\"build\",\"vs\":\"%s\"}", BUILDSTRING);
  PUTFMT(",{\"n\":\"bootcount\",\"v\":%u}", bootcount);
  PUTFMT(",{\"n\":\"bootcause\",\"v\":\"%02x\"}", GPIOR0);
  PUTFMT("]");
  printf("Publish builid\n");
  printf("%s\n", app_buffer);
  mqtt_publish(&conn, NULL, topic, (uint8_t *)app_buffer,
               strlen(app_buffer), MQTT_QOS_LEVEL, MQTT_RETAIN_OFF);
}

static void
publish(void)
{
  static uint8_t did_once = 0;
  
  if (pub_now_message)
    publish_now();
  else if (did_once == 0) {
    publish_build();
    did_once = 1;
  }
  else if ((seq_nr_value % PUBLISH_STATS_INTERVAL) == 2)
    publish_stats();
  else if (((seq_nr_value % PUBLISH_STATS_INTERVAL) == 6) && (lc.cca_test)) {
    do_all_chan_cca(cca);
    publish_cca_test();
  }
  else
    //publish_stats();
    publish_sensors();
  mqtt_stats.published++;
  if (conf.pub_interval < 600L*CLOCK_SECOND) {
    conf.pub_interval = conf.pub_interval << 1;
  }
}
/*---------------------------------------------------------------------------*/
static void
connect_to_broker(void)
{
  /* Connect to MQTT server */
  mqtt_connect(&conn, conf.broker_ip, conf.broker_port,
               conf.keep_alive_timer);

  state = STATE_CONNECTING;
}
/*---------------------------------------------------------------------------*/
static void
ping_parent(void)
{
#ifndef AT_RADIO_SOCKETS
  if(uip_ds6_get_global(ADDR_PREFERRED) == NULL) {
    return;
  }
#endif /* AT_RADIO_SOCKETS */
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
    mqtt_register(&conn, &mqtt_client_process, client_id, mqtt_event,
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
#ifdef MQTT_PUBLISH_DISCONNECT
    /* Wait for publish timer before connecting */
    etimer_set(&publish_periodic_timer, conf.pub_interval);
    return;
#endif /* MQTT_PUBLISH_DISCONNECT */
    /* Continue */
  case STATE_REGISTERED:
#ifdef AT_RADIO_SOCKETS
    if (at_radio_status()->state == AT_RADIO_STATE_ACTIVE) {
#else
    if(uip_ds6_get_global(ADDR_PREFERRED) != NULL) {
#endif /* AT_RADIO_SOCKETS */
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
    {
      static int connect_reported = 0;
      if (connect_attempt > connect_reported) {
        DBG("Connecting (%u)\n", connect_attempt);
        connect_reported += 1;
      }
    }
    break;
  case STATE_CONNECTED:
    /* Don't subscribe unless we are a registered device */
    if(strncasecmp(conf.org_id, QUICKSTART, strlen(conf.org_id)) == 0) {
      DBG("Using 'quickstart': Skipping subscribe\n");
      state = STATE_PUBLISHING;
    }
#ifdef MQTT_PUBLISH_DISCONNECT
    /* Skip subscribing and go directly to publishing state */
    state = STATE_PUBLISHING;
#endif /* MQTT_PUBLISH_DISCONNECT */    
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
      DBG("Publishing... (MQTT state %d conn.state=%d, q=%u) mqtt_ready %d out_buffer_sent %d\n", state, conn.state,
          conn.out_queue_full, mqtt_ready(&conn), conn.out_buffer_sent);
#endif      
    }
    break;
  case STATE_DISCONNECTED:
    DBG("Disconnected\n");
#ifdef MQTT_PUBLISH_DISCONNECT
    /* Done publish and disconnect -- restart publication timer */
    etimer_set(&publish_periodic_timer, conf.pub_interval);
    state = STATE_REGISTERED;
    return;
#endif /* MQTT_PUBLISH_DISCONNECT */
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
PROCESS_THREAD(mqtt_client_process, ev, data)
{

  PROCESS_BEGIN();

  printf("MQTT Client Process\n");

  if(init_config() != 1) {
    PROCESS_EXIT();
  }
  update_config();

  process_start(&mqtt_checker_process, NULL);

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
       (ev == sensors_event && data == PUBLISH_TRIGGER) ||
       pub_now_message != NULL) {
      state_machine();
    }

    if(ev == PROCESS_EVENT_TIMER && data == &echo_request_timer) {
      ping_parent();
      etimer_set(&echo_request_timer, conf.def_rt_ping_interval);
    }
  }

  PROCESS_END();
}

#ifdef MQTT_WATCHDOG

static char *statestr(uint8_t state) {
  switch (state) {
  case STATE_INIT: return("INIT");
  case STATE_REGISTERED: return("REGISTERED");
  case STATE_CONNECTING: return("CONNECTING");
  case STATE_CONNECTED: return("CONNECTED");
  case STATE_PUBLISHING: return("PUBLISHING");
  case STATE_DISCONNECTED: return("DISCONNECTED");
  case STATE_NEWCONFIG: return("NEWCONFIG");
  default: return("?");
  }
}

static char *connstatestr(mqtt_conn_state_t connstate) {
  switch (connstate) {
  case MQTT_CONN_STATE_ERROR: return("ERROR");
  case MQTT_CONN_STATE_DNS_ERROR: return("DNS_ERROR");
  case MQTT_CONN_STATE_DISCONNECTING: return("DISCONNECTING");
  case MQTT_CONN_STATE_ABORT_IMMEDIATE: return("ABORT_IMMEDIATE");
  case MQTT_CONN_STATE_NOT_CONNECTED: return("NOT_CONNECTED");
  case MQTT_CONN_STATE_DNS_LOOKUP: return("DNS_LOOKUP");
  case MQTT_CONN_STATE_TCP_CONNECTING: return("TCP_CONNECTING");
  case MQTT_CONN_STATE_TCP_CONNECTED: return("TCP_CONNECTED");
  case MQTT_CONN_STATE_CONNECTING_TO_BROKER: return("CONNECTING_TO_BROKER");
  case MQTT_CONN_STATE_CONNECTED_TO_BROKER: return("CONNECTED_TO_BROKER");
  case MQTT_CONN_STATE_SENDING_MQTT_DISCONNECT: return("SENDING_MQTT_DISCONNECT");
  default: return("?");
  }
}

PROCESS_THREAD(mqtt_checker_process, ev, data)
{
  static uint8_t stale_publishing = 0, stale_tcp_connecting = 0, stale_connecting = 0;
  static uint16_t seen_seq_nr_value = 0;

  PROCESS_BEGIN();
  etimer_set(&checktimer, WATCHDOG_INTERVAL);

  /* Main loop */
  while(1) {

    PROCESS_YIELD();

    if((ev == PROCESS_EVENT_TIMER) && (data == &checktimer)) {
      printf("MQTT client: state %s (%d) conn.state %s (%d)\n", statestr(state), state, connstatestr(conn.state), conn.state);
      if (state == STATE_PUBLISHING) { 
        stale_connecting = 0;
        if (seq_nr_value > seen_seq_nr_value) {
          stale_publishing = 0;
        }
        else {
          if (++stale_publishing > STALE_PUBLISHING_WATCHDOG) {
            /* In publishing state, but nothing published for a while. 
             * Milder reset -- call mqtt_disconnect() to trigger mqtt to restart the session
             */
            printf("MQTT watchdog -- publishing\n");
            mqtt_disconnect(&conn);
            stale_publishing = 0;
            watchdog_stats.stale_publishing++;
            stale_publishing = 0;
          }
        }
        seen_seq_nr_value = seq_nr_value;
      }
      else if (conn.state > MQTT_CONN_STATE_NOT_CONNECTED) {
        if (++stale_tcp_connecting > STALE_TCP_CONNECTING_WATCHDOG) {
          /* Waiting for mqtt connection, but nothing happened for a while.
           * Trigger communication error by closing TCP socket 
           */
          printf("MQTT watchdog -- tcp connecting\n");
          tcp_socket_close(&conn.socket);
          watchdog_stats.stale_tcp_connecting++;
          stale_tcp_connecting = 0;
        }       
      }
      else if (state > STATE_REGISTERED) {
        if (++stale_connecting > STALE_CONNECTING_WATCHDOG) {
          /* Mismatch - we are waiting for MQTT but MQTT is not doing anything 
           * Reset and start over
           */
          printf("MQTT watchdog -- connecting\n");
          tcp_socket_close(&conn.socket);
          state = STATE_REGISTERED;
          watchdog_stats.stale_connecting++;
          stale_connecting = 0;
        }       
      }
      else {
        stale_tcp_connecting = 0;
        stale_connecting = 0;
      }
      
      etimer_reset(&checktimer);
    }
  }
  PROCESS_END();
}

#endif /* MQTT_WATCHDOG */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
