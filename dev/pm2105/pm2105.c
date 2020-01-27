/*
 * Copyright (c) 2017, Peter Sjodin, KTH Royal Institute of Technology
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Author  : Peter Sjodin, KTH Royal Institute of Technology
 * Created : 2017-04-21
 *
 * Robert Olsson rework for PM2105. 
 *
 */

/*
 * \file
 *         Driver for Cubic PM2105 particle sensors
 */

#include "contiki.h"
#include "sys/etimer.h"
#include "sys/pt.h"
#include <stdio.h>
#include <string.h>
#include "i2c.h"
#include "watchdog.h"
#include "dev/leds.h"
#include "dev/rs232.h"
#include "dev/pm2105-arch.h"
#include "pm2105.h"

#include "lib/ringbuf.h"

/* Preamble signature */
#define PRE 0x16
/* Measurement length*/
#define PM_MESSAGE_LEN 32
#define PMSBUFFER PM_MESSAGE_LEN

/* Frame assembly statistics */
static uint32_t invalid_frames, valid_frames;

/* Sensor configured on? */
static uint8_t configured_on = 0;

/* Last readings of sensor data */
static uint16_t PM1, PM2_5, PM10;
static uint16_t PM1_TSI, PM2_5_TSI, PM10_TSI;
static uint16_t DB0_3, DB0_5, DB1, DB2_5, DB5, DB10;
/* Time when last sensor data was read, in clock_seconds()*/
static unsigned long timestamp = 0;

static struct pms_config {
  unsigned sample_period;    /* Time between samples (sec) */
  unsigned warmup_interval; /* Warmup time (sec) */
} pms_config;

#define DEBUG 0

static struct pt read_values_pt;
static int now = 0;
static uint8_t status, setup = 1;
static uint16_t mode, calibration;
static uint8_t buf[PMSBUFFER];

/*---------------------------------------------------------------------------*/
PROCESS(pm2105_timer_process, "PM2105 periodic dust sensor process");
/*---------------------------------------------------------------------------*/
/**
 * Initialize. Create event, and start timer-driven process.
 * start PMS frame assembly process.
 */
void
pm2105_init()
{
  pm2105_config_sample_period(PMS_SAMPLE_PERIOD);
  pm2105_config_warmup_interval(PMS_WARMUP_INTERVAL);
  pm2105_event = process_alloc_event();
  process_start(&pm2105_timer_process, NULL);
  configured_on = 1;

#ifdef DEBUG
  printf("PM2105: I2C %d, sample period %d, warmup interval %d\n",
         PMS_SERIAL_I2C, pms_config.sample_period, pms_config.warmup_interval);
#endif /* DEBUG */
}
/*---------------------------------------------------------------------------*/
/**
 * Sensor API for PM2105
 */

void
pm2105_off()
{
  pm2105_set_standby_mode(STANDBY_MODE_ON);
  configured_on = 0;
}
uint16_t
pm2105_pm1()
{
  return PM1;
}
uint16_t
pm2105_pm2_5()
{
  return PM2_5;
}
uint16_t
pm2105_pm10()
{
  return PM10;
}
uint16_t
pm2105_pm1_tsi()
{
  return PM1_TSI;
}
uint16_t
pm2105_pm2_5_tsi()
{
  return PM2_5_TSI;
}
uint16_t
pm2105_pm10_tsi()
{
  return PM10_TSI;
}
uint16_t
pm2105_db0_3()
{
  return DB0_3;
}
uint16_t
pm2105_db0_5()
{
  return DB0_5;
}
uint16_t
pm2105_db1()
{
  return DB1;
}
uint16_t
pm2105_db2_5()
{
  return DB2_5;
}
uint16_t
pm2105_db5()
{
  return DB5;
}
uint16_t
pm2105_db10()
{
  return DB10;
}
uint8_t
pm2105_status()
{
  return status;
}
uint16_t
pm2105_mode()
{
  return mode;
}
uint8_t
pm2105_calibration()
{
  return calibration;
}
uint32_t
pm2105_timestamp()
{
  return timestamp;
}
uint32_t
pm2105_valid_frames()
{
  return valid_frames;
}
uint32_t
pm2105_invalid_frames()
{
  return invalid_frames;
}
void
pm2105_config_sample_period(unsigned int sample_period) {
  pms_config.sample_period = sample_period;
}
void
pm2105_config_warmup_interval(unsigned int warmup_interval) {
  pms_config.warmup_interval = warmup_interval;
}
unsigned
pm2105_get_sample_period(void) {
  return pms_config.sample_period;
}
unsigned
pm2105_get_warmup_interval(void) {
  return pms_config.warmup_interval;
}
/*---------------------------------------------------------------------------*/
/**
 * Check if it is time to put sensor in standby mode.
 * The condition is that the time from when last sample was taken until the
 * next is due, is greater than the time it takes to wake up the sensor. 
 */
static int
timetosleep(void) {
  return (timestamp + pms_config.sample_period >
          clock_seconds() + pms_config.warmup_interval);
}
/*---------------------------------------------------------------------------*/
/**
 * Check if it is time to get the next sample. 
 */
static int
timetoread(void) {
  return (clock_seconds() - timestamp >= pms_config.sample_period);
}
/*---------------------------------------------------------------------------*/
/**
 * Validate frame by checking preamble, length field and checksum.
 * Return 0 if invalid frame, otherwise 1.
 */
int
check_pmsframe(uint8_t *buf)
{
  int sum;
  int i;
  int len;

  if(buf[0] != PRE) {
    return 0;
  }

  len = buf[1];
  if(len != PM_MESSAGE_LEN) {
    return 0;
  }

  /* XOR bytewise, excluding checksum */
  sum = 0;
  for(i = 0; i < len - 1; i++) {
      sum ^= buf[i];
  }
#ifdef DEBUG_CHECKSUM
  /* Compare with received checksum last in frame*/
  printf("%02X vs %02X\n", buf[len-1], sum);
#endif
  return sum == buf[len-1]; 
}
/*---------------------------------------------------------------------------*/
#ifdef DEBUG
void
printpm()
{
  printf("PM %lu/%lu",
         valid_frames, invalid_frames);
  printf(" GRIM %4u %4u %4u", PM1, PM2_5, PM10);
  printf(" TSI %4u %4u %4u", PM1_TSI, PM2_5_TSI, PM10_TSI);
  //printf("DB0_3 = %4u, DB0_5 = %4u, DB1 = %4u, DB2_5 = %4u, DB5 = %4u, DB10 = %4u\n",
  printf(" DB %4u %4u %4u %4u %4u %4u\n",
         DB0_3, DB0_5, DB1, DB2_5, DB5, DB10);
}
#endif /* DEBUG */
/*---------------------------------------------------------------------------*/
/**
 * Frame received from PMS sensor. Validate and update sensor data.
 * Return 1 if valid frame, otherwise 0
 */
int
pmsframe(uint8_t *buf)
{
  if(check_pmsframe(buf)) {
    timestamp = clock_seconds();
    valid_frames++;
    /* Sensor Status */
    //status = buf[2];
    //mode = (buf[3] << 8) + buf[4];
    calibration = (buf[5] << 8) + buf[6];
#ifdef DEBUG
    printf("PM status=%d mode=%u, cal=%u\n", status, mode, calibration);
#endif

    /* Update sensor readings */
    PM1 = (( uint16_t) (buf[7] << 8)) | buf[8];
    PM2_5 = (buf[9] << 8) | buf[10];
    PM10 = (buf[11] << 8) | buf[12];
    PM1_TSI = (buf[13] << 8) | buf[14];
    PM2_5_TSI = (buf[15] << 8) | buf[16];
    PM10_TSI = (buf[17] << 8) | buf[18];
    /* Dust bins */
    DB0_3 = (buf[19] << 8) | buf[20];
    DB0_5 = (buf[21] << 8) | buf[22];
    DB1 = (buf[23] << 8) | buf[24];
    DB2_5 = (buf[25] << 8) | buf[26];
    DB5 = (buf[27] << 8) | buf[28];
    DB10 = (buf[29] << 8) | buf[30];
#ifdef DEBUG
    printpm();
#endif /* DEBUG */
    return 1;
  } else {
    invalid_frames++;
    return 0;
  }
}

PT_THREAD(read_values(struct pt *pt))
{
  PT_BEGIN(pt);  

  if(setup)  {
    setup = 0;
      buf[0] = 0x16;
      buf[1] = 3; /* Cont. Mode */
      buf[1] = 4; /* Time Mode -- Single */
      buf[1] = 5; /* Dynamic Mode */
      buf[1] = 7; /* Warm Mode */

      buf[1] = 4; /* Time Mode -- Single */
      buf[1] = 3; /* Cont. Mode */
	    
      buf[2] = 7;

      if(buf[1] == 3) {
	buf[3] = 0xFF;
	buf[4] = 0xFF;
      }
      else {
	buf[3] = 0;
	buf[4] = 60; //0xFF;
      }
      buf[5] = 0; /* Reserved */
      buf[6] = (buf[0]^buf[1]^buf[2]^buf[3]^buf[4]^buf[5]); /* Checksum */
      i2c_write_mem_buf(I2C_PM2105_ADDR, buf, 7);
  }
  if(i2c_probed & I2C_PM2105 ) {

    if(now)  {
      buf[0] = 0x16;
      if(now == 1) {  /* Close */
	buf[1] = 1; 
      }
      if(now == 2) {  /* Open */
	buf[1] = 2; 
      }
      buf[2] = 7;
      buf[3] = 0;
      buf[4] = 60; //0xFF;
      buf[5] = 0; /* Reserved */
      buf[6] = (buf[0]^buf[1]^buf[2]^buf[3]^buf[4]^buf[5]); /* Checksum */
      i2c_write_mem_buf(I2C_PM2105_ADDR, buf, 7);
    }
    memset(buf, 0, sizeof(buf));
    i2c_read_mem_buf(I2C_PM2105_ADDR, 0, buf, 32);
    status = buf[2];
    mode = (buf[3] << 8) + buf[4];

    /* Cont. Mode */
    if(mode == 3) {
      if(status == 0x80 ) {
	now = 1;
      }
      if(0 && status == 2 ) {
	static struct etimer timer;
	etimer_set(&timer, 30 * CLOCK_SECOND);
	PT_WAIT_UNTIL(pt, etimer_expired(&timer));
	now = 1;
      }
      if(0 && status == 1 ) {
	static struct etimer timer;
	etimer_set(&timer, 30 * CLOCK_SECOND);
	PT_WAIT_UNTIL(pt, etimer_expired(&timer));
	now = 2;
      }
    }

    /* Time Mode */
    if(mode == 4) {
      if(status == 0x80 ) {
	printf("**DR-Close: ");
	now = 1;
      }
      if(status == 1) {
	static struct etimer timer;
	etimer_set(&timer, 30 * CLOCK_SECOND);
	PT_WAIT_UNTIL(pt, etimer_expired(&timer));
	now = 2;
      }
    }

    /* Dynamic Mode */
    if(mode == 5) {
      if(status == 0x80 ) {
	printf("**DR: ");
      }
    }

    /* Warm Mode */
    if(mode == 7) {
      if(status == 0x80 ) {
	printf("** WARM: ");
      }
    }

#if 0
    for(i = 0; i < sizeof(buf) ; i++)  {
      printf(" %02x", buf[i]);
    }
    printf("\n");
#endif
  }
  PT_END(pt);  
}

/*---------------------------------------------------------------------------*/
/**
 * Timer thread: duty-cycle sensor. Toggle between idle and active mode.
 * For I2C, also read data when it is due.
 */
PROCESS_THREAD(pm2105_timer_process, ev, data)
{
  static struct etimer pmstimer;
  static uint8_t standbymode;

  PROCESS_BEGIN();
  PT_INIT(&read_values_pt);
  etimer_set(&pmstimer, CLOCK_SECOND * PMS_PROCESS_PERIOD);

/* Pretend there is a fresh reading, to postpone the first
 * reading for one cycle
 */
  timestamp = clock_seconds(); 

  /* Put sensor in standby, if there is enough time */
  if(timetosleep()) 
    pm2105_set_standby_mode(STANDBY_MODE_ON);
  else
    pm2105_set_standby_mode(STANDBY_MODE_OFF);

  pm2105_event = process_alloc_event();

  /* Main loop */
  while(1) {

    PROCESS_YIELD();
    if(!configured_on) {
      continue;
    }

    if((ev == PROCESS_EVENT_TIMER) && (data == &pmstimer)) {
      standbymode = pm2105_get_standby_mode();
      if(standbymode == STANDBY_MODE_OFF) {
        /* Read data over I2C if it is time */
        if(timetoread()) {
          if(pm2105_i2c_probe()) {
            leds_on(LEDS_RED);
	    read_values(&read_values_pt);
            /* Check frame and update sensor readings */
            if(pmsframe(buf)) {
              /* Tell other processes there is new data */
              if(process_post(PROCESS_BROADCAST, pm2105_event, NULL) == PROCESS_ERR_OK) {
                PROCESS_WAIT_EVENT_UNTIL(ev == pm2105_event);
              }
              /* Put sensor in standby, if there is enough time */
              if(timetosleep()) {
                pm2105_set_standby_mode(STANDBY_MODE_ON);
              }
            }
          }
        }
      } else if(standbymode == STANDBY_MODE_ON) {
        /* Time to warm up sensor for next reading? */
        if(!timetosleep()) {
          pm2105_set_standby_mode(STANDBY_MODE_OFF);
        }
      }
      etimer_reset(&pmstimer);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
