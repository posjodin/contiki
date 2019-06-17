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
 */

#include <stdlib.h>

#include "contiki.h"
#include "lib/sensors.h"
#include "pm2105.h"
#include "pm2105-sensor.h"

const struct sensors_sensor pm2105_sensor;

enum {
  ON, OFF
};
static uint8_t state = OFF;

/*---------------------------------------------------------------------------*/
PROCESS(pm2105_sensor_process, "PM2105 sensor process");
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  switch(type) {
  case PM2105_SENSOR_PM1:
    return pm2105_pm1();
  case PM2105_SENSOR_PM2_5:
    return pm2105_pm2_5();
  case PM2105_SENSOR_PM10:
    return pm2105_pm10();
  case PM2105_SENSOR_PM1_TSI:
    return pm2105_pm1_tsi();
  case PM2105_SENSOR_PM2_5_TSI:
    return pm2105_pm2_5_tsi();
  case PM2105_SENSOR_PM10_TSI:
    return pm2105_pm10_tsi();
  case PM2105_SENSOR_DB0_3:
    return pm2105_db0_3();
  case PM2105_SENSOR_DB0_5:
    return pm2105_db0_5();
  case PM2105_SENSOR_DB1:
    return pm2105_db1();
  case PM2105_SENSOR_DB2_5:
    return pm2105_db2_5();
  case PM2105_SENSOR_DB5:
    return pm2105_db5();
  case PM2105_SENSOR_DB10:
    return pm2105_db10();
  case PM2105_SENSOR_STATUS:
    return pm2105_status();
  case PM2105_SENSOR_MODE:
    return pm2105_mode();
  case PM2105_SENSOR_CALIBRATION:
    return pm2105_calibration();
  case PM2105_SENSOR_TIMESTAMP:
    return pm2105_timestamp();
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  switch(type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return state == ON;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  switch(type) {
  case SENSORS_ACTIVE:
    if(c) {
      if(!status(SENSORS_ACTIVE)) {
        pm2105_init();
        process_start(&pm2105_sensor_process, NULL);
        state = ON;
      }
    } else {
      pm2105_off();
      state = OFF;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(pm2105_sensor, "pm2105",
               value, configure, status);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(pm2105_sensor_process, ev, data)
{

  PROCESS_BEGIN();
  while(1) {
    do {
      PROCESS_WAIT_EVENT();
    } while(ev != pm2105_event);
    sensors_changed(&pm2105_sensor);
  }
  PROCESS_END();
}
