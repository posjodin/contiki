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
#ifdef MQTT_CLIENT
#include "mqtt.h"
#endif /* MQTT_CLIENT */
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "lib/sensors.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/temp-sensor.h"
#include "dev/battery-sensor.h"
#include <string.h>
#include <math.h> /* NO2 */
#include "adc.h"
#include "dev/button-sensor.h"
#include "dev/pulse-sensor.h"
#include "dev/pms5003/pms5003.h"
#include "dev/pms5003/pms5003-sensor.h"
#include "i2c.h"
#include "dev/bme280/bme280-sensor.h"
#include "dev/serial-line.h"
#include "watchdog.h"
#ifndef RF230_DEBUG
#define RF230_DEBUG 0
#else
#if RF230_DEBUG
#include "radio/rf230bb/rf230bb.h"
#endif /* #if RF230_DEBUG */
#endif /* #ifndef RF230_DEBUG */
#ifdef RPL_BORDER_ROUTER
#include "rpl.h"
#endif /* RPL_BORDER_ROUTER */
#ifdef AT_RADIO
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "at-radio.h"
#endif /* MQTT_AT_RADIO */  


extern void handle_serial_input(const char *line);

#ifdef AT_PPP
PROCESS_NAME(ip64_ppp_process);
#endif /* AT_PPP */

#ifdef MQTT_CLIENT 
PROCESS_NAME(mqtt_demo_process);
PROCESS_NAME(mqtt_client_process);
#endif /* MQTT_CLIENT */

PROCESS_NAME(tcp_dummy_process);

PROCESS(serial_in, "cli input process");
PROCESS(mqtt_demo_process, "MQTT Demo");

#if defined(MQTT) && defined(MQTT_WATCHDOG)
AUTOSTART_PROCESSES(&mqtt_demo_process, &sensors_process, &serial_in);
#else
AUTOSTART_PROCESSES(&mqtt_demo_process, &sensors_process);
#endif
//SENSORS(&button_sensor, &pms5003_sensor);

/*---------------------------------------------------------------------------*/
#ifdef RPL_BORDER_ROUTER
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  printf("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      printf(" ");
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t prefix;
void
set_prefix_64(uip_ipaddr_t *prefix_64)
{
  rpl_dag_t *dag;
  uip_ipaddr_t ipaddr;
  memcpy(&prefix, prefix_64, 16);
  memcpy(&ipaddr, prefix_64, 16);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  dag = rpl_set_root(RPL_DEFAULT_INSTANCE, &ipaddr);
  if(dag != NULL) {
    rpl_set_prefix(dag, &prefix, 64);
    printf("created a new RPL dag\n");
  }
}

void
set_rpl_root() {
  uip_ipaddr_t prefix;
  memset(&prefix, 0, sizeof(prefix));
  prefix.u8[0] = 0xfd;
#if (UIP_LLADDR_LEN == 8)
  memcpy(&prefix.u8[1], ((uint8_t *) &uip_lladdr)+3, 5);
#else
#error Can only handle 8 byte lladdr
#endif  
  set_prefix_64(&prefix);
  print_local_addresses();
}
#endif /* RPL_BORDER_ROUTER */
/*---------------------------------------------------------------------------*/
#ifdef AT_PPP
/*---------------------------------------------------------------------------*/
/*
 * A node with only an uplink may not get an IPv6 address through autoconf.
 * Assign one statically instead as a translatable address (64:ff9b::/96).
 */

static void
create_translatable_address(uip_ipaddr_t *ipaddr, uip_ip4addr_t *ip4addr) {

  memset(ipaddr, 0, sizeof(*ipaddr));
  ipaddr->u8[0] = 0x00;
  ipaddr->u8[1] = 0x64;  
  ipaddr->u8[2] = 0xff;
  ipaddr->u8[3] = 0x9b;  
  ipaddr->u8[12] = ip4addr->u8[0];
  ipaddr->u8[13] = ip4addr->u8[1];
  ipaddr->u8[14] = ip4addr->u8[2];
  ipaddr->u8[15] = ip4addr->u8[3];
}
/*---------------------------------------------------------------------------*/
static void get_ppp_v4addr(uip_ip4addr_t *addr, uip_ip4addr_t *mask) {
  struct at_radio_status *atstat;

  atstat = at_radio_status();
  *addr = atstat->ip4addr; *mask = atstat->ip4mask;
}
/*---------------------------------------------------------------------------*/
/* 
 * Set IPv4 address to use for source in IP64 translation
 */
static void set_ip64_v4addr() {
  uip_ip4addr_t ip4addr, ip4mask;

  get_ppp_v4addr(&ip4addr, &ip4mask);
  ip64_set_ipv4_address(&ip4addr, &ip4mask);
}
/*---------------------------------------------------------------------------*/
/* 
 * Set a local static IPv6 address 
 */
static void set_local_v6addr() {
  uip_ip4addr_t ip4addr, ip4mask;
  uip_ipaddr_t loc_ipaddr;

  get_ppp_v4addr(&ip4addr, &ip4mask);
  create_translatable_address(&loc_ipaddr, &ip4addr);
  uip_ds6_addr_add(&loc_ipaddr, 0, ADDR_ANYTYPE);

}

#include "dev/ppp/ppp.h"
#include "dev/ppp/ipcp.h"

static uint8_t ppp_isup() {
  return ((ipcp_state & IPCP_TX_UP) && (ipcp_state & IPCP_RX_UP));
}
#endif /* AT_PPP */
/*---------------------------------------------------------------------------*/
/*
 * Bring up the various components in proper order, and configure
 * IP addresses when needed.
 */ 
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

#if RF230_DEBUG
  printf("RF230_CONF_FRAME_RETRIES: %d\n", RF230_CONF_FRAME_RETRIES);
  printf("RF230_CONF_CMSA_RETRIES: %d\n", RF230_CONF_CSMA_RETRIES);
#endif  
  /* The data sink runs with a 100% duty cycle in order to ensure high 
     packet reception rates. */
  //NETSTACK_MAC.off(1);

  printf("MQTT Demo Launcher\n");

#ifdef RPL_BORDER_ROUTER
/* While waiting for the prefix to be sent through the PPP connection, the future
 * border router can join an existing DAG as a parent or child, or acquire a default
 * router that will later take precedence over the PPP fallback interface.
 * Prevent that by turning the radio off until we are initialized as a DAG root.
 */
  NETSTACK_MAC.off(0);
#endif /* RPL_BORDER_ROUTER */

#ifdef AT_RADIO
  at_radio_init();
  PROCESS_WAIT_EVENT_UNTIL(ev == at_radio_ev_init);
  printf("AT_RADIO initialized\n");
#endif /* MQTT_AT_RADIO */  

#ifdef AT_PPP
  process_start(&ip64_ppp_process, NULL);
  PROCESS_WAIT_UNTIL(ppp_isup());
  set_ip64_v4addr();
  /* With only an uplink, we get no address through autoconf. 
   * Set static address instead
   */
  set_local_v6addr();
#endif /* AT_PPP */

#ifdef RPL_BORDER_ROUTER
  NETSTACK_MAC.off(1);
  set_rpl_root();
#endif /* RPL_BORDER_ROUTER */

#ifdef MQTT_CLIENT
  process_start(&mqtt_client_process, NULL);
#else
  process_start(&tcp_dummy_process, NULL);
#endif /* MQTT */
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
#ifdef MQTT_CLIENT
extern void handle_serial_input(const char *line);

PROCESS_THREAD(serial_in, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message && data != NULL);
    handle_serial_input((const char *) data);
  }
  PROCESS_END();
}
#endif /* MQTT_CLIENT */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
