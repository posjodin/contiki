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
#ifdef RPL_BORDER_ROUTER
#include "rpl.h"
#endif /* RPL_BORDER_ROUTER */
#ifdef AT_RADIO
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "at-radio.h"
#endif /* MQTT_AT_RADIO */  
#include <avr/eeprom.h> /* Görs i contiki-main.c */

#define EEUPSTATSSIZE 8
#define EEUPSTATSMAGIC 0xacdc

static uint16_t bootcount;

static uint16_t EEMEM ee_magic;
static uint16_t EEMEM ee_bootcount;

uint32_t EEMEM ee_debug;
PROCESS(eestats, "Boot stats in eeprom");

static void init_upstats() {
  uint16_t magic;
  
  /* Check magic number to make sure there is valid stats in eeprom */
  eeprom_read_block((void*)&magic, (void *)&ee_magic, sizeof(magic));  
  if (magic != EEUPSTATSMAGIC) {
    magic = EEUPSTATSMAGIC;
    eeprom_write_block((void*)&magic, (void *)&ee_magic, sizeof(magic));
    bootcount = 0;
    eeprom_write_block((void*)&ee_bootcount, (void *)&bootcount, sizeof(uint16_t));
  }
}

static void update_bootcount() {
  
  eeprom_read_block((void*)&bootcount, (void *)&ee_bootcount, sizeof(bootcount));
  bootcount += 1;
  eeprom_write_block((void*)(void *)&bootcount, &ee_bootcount, sizeof(ee_bootcount));
  printf("Boot count: %d\n", bootcount);

}

uint16_t eestats_get_bootcount() {
  return bootcount;
}

PROCESS_THREAD(eestats, ev, data) {
  
  PROCESS_BEGIN();
  printf("Here is eestats process\n");
  init_upstats();
  update_bootcount();
  PROCESS_END();
}
