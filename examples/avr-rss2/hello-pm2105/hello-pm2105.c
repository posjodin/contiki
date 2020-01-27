/*
 * Copyright (c) 2015, Copyright Robert Olsson / Radio Sensors AB  
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
 *
 * Author  : Robert Olsson robert@radio-sensors.com
 * Created : 2015-11-22
 */

/**
 * \file
 *         A simple application for airsensor reading on RSS2 mote
 */

#include "contiki.h"
#include "pt.h"
#include "sys/etimer.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "adc.h"
#include "i2c.h"
#include "dev/leds.h"
#include "dev/battery-sensor.h"
#include "dev/temp-sensor.h"
#include "dev/temp_mcu-sensor.h"
#include "dev/light-sensor.h"
#include "dev/pulse-sensor.h"
#include "dev/bme280/bme280-sensor.h"
#include "dev/co2_sa_kxx-sensor.h"
#include "dev/pm2105/pm2105.h"
#include "dev/pm2105/pm2105-sensor.h"
#include "i2c.h"

PROCESS(hello_airsensors_process, "Hello sensor process");
AUTOSTART_PROCESSES(&hello_airsensors_process);

static struct etimer et;
unsigned char buf[70];

PROCESS_THREAD(hello_airsensors_process, ev, data)
{
  PROCESS_BEGIN();
  if(i2c_probed & I2C_PM2105 ) {
    SENSORS_ACTIVATE(pm2105_sensor);
  }

  leds_init(); 
  pm2105_init();
  
  etimer_set(&et, CLOCK_SECOND * 5);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    if( pm2105_sensor.value(PM2105_SENSOR_STATUS) == 2) {
    
      printf("pm: %u %u %u tsi: %u %u %u db: %u %u %u %u %u %u\n",
	     pm2105_sensor.value(PM2105_SENSOR_PM1),
	     pm2105_sensor.value(PM2105_SENSOR_PM2_5),
	     pm2105_sensor.value(PM2105_SENSOR_PM10),
	     pm2105_sensor.value(PM2105_SENSOR_PM1_TSI),
	     pm2105_sensor.value(PM2105_SENSOR_PM2_5_TSI),
	     pm2105_sensor.value(PM2105_SENSOR_PM10_TSI),

	     pm2105_sensor.value(PM2105_SENSOR_DB0_3),
	     pm2105_sensor.value(PM2105_SENSOR_DB0_5),
	     pm2105_sensor.value(PM2105_SENSOR_DB1),
	     pm2105_sensor.value(PM2105_SENSOR_DB2_5),
	     pm2105_sensor.value(PM2105_SENSOR_DB5),
	     pm2105_sensor.value(PM2105_SENSOR_DB10)
	     );
    }
    etimer_reset(&et);
  }
  PROCESS_END();
}
