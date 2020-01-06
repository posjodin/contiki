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
struct upbuf {
  uint16_t magic;
  uint8_t put;
  uint8_t length;
  uint8_t size;
};

struct upstat {
    uint8_t bootcause;
    uint32_t upsecs;
};

struct upbuf EEMEM ee_upbuf;
struct upbuf upbuf;
struct upstat EEMEM ee_stats[EEUPSTATSSIZE];
struct upstat upstat;
uint16_t EEMEM ee_bootcount;

uint32_t EEMEM ee_debug;
PROCESS(eestats, "Boot stats in eeprom");

static void init_upstats() {
  uint16_t bootcount;

  upbuf.magic = EEUPSTATSMAGIC;
  upbuf.put = 0;
  upbuf.length = 0;
  upbuf.size = EEUPSTATSSIZE;
  eeprom_write_block((void*)&upbuf, (void *)&ee_upbuf, sizeof(struct upbuf));
  bootcount = 0;
  eeprom_write_block((void*)&ee_bootcount, (void *)&bootcount, sizeof(uint16_t));
}

static struct upstat *alloc_upstat() {
  struct upstat *put;
  
  put = &ee_stats[upbuf.put];
  upbuf.put = (upbuf.put + 1) % upbuf.size;
  if (upbuf.length < upbuf.size) {
    upbuf.length += 1;
  }
  eeprom_write_block((void*)&upbuf, (void *)&ee_upbuf, sizeof(struct upbuf));  
  return put;
}

static void read_ee_upbuf() {
 again:
  eeprom_read_block((void*)&upbuf, (void *)&ee_upbuf, sizeof(struct upbuf));  
  if (upbuf.magic != EEUPSTATSMAGIC) {
    printf("New upstat block\n");
    init_upstats();
    goto again;
  }
  else {
    printf("There was an upstat block\n");
  }
}

uint16_t bootcount;

static void update_bootcount() {
  eeprom_read_block((void*)&bootcount, (void *)&ee_bootcount, sizeof(uint16_t));
  bootcount += 1;
  eeprom_write_block((void*)(void *)&bootcount, &ee_bootcount, sizeof(ee_bootcount));
  printf("Boot count: %d\n", bootcount);

}

static void upstats_update_bootcause(struct upstat *ee_upstatp, uint8_t bootcause) {
  upstat.bootcause = bootcause;
  eeprom_write_block(&upstat.bootcause, &ee_upstatp->bootcause, sizeof(ee_upstatp->bootcause));
}  

static void upstats_update_uptime(struct upstat *ee_upstatp, uint32_t upsecs) {
  upstat.upsecs = upsecs;
  eeprom_write_block(&upstat.upsecs, &ee_upstatp->upsecs, sizeof(ee_upstatp->upsecs));
}  

static void put_upstatsbuf() {
  struct upbuf upbuf;
  upbuf.magic = EEUPSTATSMAGIC;
  upbuf.put = 0;
  upbuf.length = 0;
  upbuf.size = EEUPSTATSSIZE;
  eeprom_write_block((void*)&upbuf, (void *)&ee_upbuf, sizeof(struct upbuf));
}

static struct update_intervals {
  uint16_t update_interval; /* Time between updates (minutes) */
  uint16_t update_count; /* How many updates to do with this interval - 0 is infinite */
} update_intervals[] = {{5, 6}, {30, 47}, {360, 0}};

PROCESS_THREAD(eestats, ev, data) {
  static uint32_t uptime = 0; /* minutes */
  static uint8_t cur_interval = 0;
  static uint8_t cur_count = 0;
  static struct upstat *ee_upstatp;
  static struct etimer et;
  uint32_t update_interval;
  
  PROCESS_BEGIN();
  printf("Here is eestats process\n");
  read_ee_upbuf();
  printf("upbuf: put %d length %d size %d\n", upbuf.put, upbuf.length, upbuf.size);
  update_bootcount();

  ee_upstatp = alloc_upstat();
  upstats_update_bootcause(ee_upstatp, GPIOR0);
  cur_count = 0;
  while (1) {
    update_interval = update_intervals[cur_interval].update_interval;
    printf("Sleep %lu\n", update_interval);
    etimer_set(&et, update_interval*60L*CLOCK_SECOND);
    while (!etimer_expired(&et)) {
      PROCESS_PAUSE();
    } 
    uptime += update_interval;
    printf("Time to update %lu\n", uptime);
    if (update_intervals[cur_interval].update_count > 0) {
      if (++cur_count >= update_intervals[cur_interval].update_count) {
        cur_interval++;
        cur_count = 0;
      }
    }
  }

  PROCESS_END();
}
