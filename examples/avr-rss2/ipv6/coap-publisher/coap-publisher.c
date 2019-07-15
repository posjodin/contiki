/*
 * Copyright (c) 2018, 
 * All rights reserved. KTH
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
 *
 * This file is part of the Contiki operating system.
 */

/**
 *      This code is based on code
 *      Robert Olsson <roolss@kth.se>
 *      Jussi Haikara <haikara@kth.se>
 *      Peter Sjödin  <psj@kth.se>
 * 
 */

#include "sys/etimer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "lib/list.h"
#include "rest-engine.h"
#include "coap-pubsub-client.h"
#include "i2c.h"
#include "net/nbr-table.h"
#include "dev/temp-sensor.h"
#include "dev/bme280/bme280-sensor.h"
#include "dev/bme680/bme680-sensor.h"
#include "dev/pms5003/pms5003.h"
#include "dev/pms5003/pms5003-sensor.h"
#include "dev/light-sensor.h"
#include "mcu_sleep.h"
#include "apps/no2/no2.h"
#include "no2-arch.h"

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define PUTFMT(...) { \
    len = snprintf(buf_ptr, remaining, __VA_ARGS__);	\
		if (len < 0 || len >= remaining) { \
			printf("Line %d: Buffer too short. Have %d, need %d + \\0", __LINE__, remaining, len); \
			return -1; \
		} \
		remaining -= len; \
		buf_ptr += len; \
	}

PROCESS(coap_client, "CoAP publisher client");
AUTOSTART_PROCESSES(&coap_client);

extern struct process mcu_sleep_process;

static broker_t broker;
static client_topic_t topic_dir, topic_bme, topic_pm, topic_light, topic_seconds;
static client_topic_t topic_mcu_sleep, topic_bootcause, topic_no2;

static uint8_t content_buffer[128];
static struct etimer publish_timer;
static char *buf_ptr;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coap_client, ev, data)
{
  static int found_broker = 0;
  int i, len, remaining = 0;

  PROCESS_BEGIN();

  process_start(&mcu_sleep_process, NULL);

  if(i2c_probed & I2C_BME280 ) {
    SENSORS_ACTIVATE(bme280_sensor);
  }
  if( i2c_probed & I2C_BME680 ) {
    SENSORS_ACTIVATE(bme680_sensor);
  }

  if( i2c_probed & I2C_PMS5003 ) {
    SENSORS_ACTIVATE(pms5003_sensor);
  }
 
  topic_dir.broker = &broker;
  topic_bme.broker = &broker;
  topic_pm.broker = &broker;
  topic_light.broker = &broker;
  topic_mcu_sleep.broker = &broker;
  topic_seconds.broker = &broker;
  topic_bootcause.broker = &broker;
  topic_no2.broker = &broker;

  broker.port = UIP_HTONS(COAP_DEFAULT_PORT);

  /* convert the to a numeric IPv6 address */
  if(uiplib_ip6addrconv(COAP_DEMO_BROKER_IP_ADDR, &broker.address) == 0) {
    return 0;
  }

  topic_dir.content_type = 40;

  topic_bme.content_type = 0;
  topic_bme.content = (uint8_t *)&content_buffer;
  topic_bme.max_age = COAP_MAX_AGE*4;

  topic_pm.content_type = 0;
  topic_pm.content = (uint8_t *)&content_buffer;
  topic_pm.max_age = COAP_MAX_AGE*4;

  topic_light.content_type = 0;
  topic_light.content = (uint8_t *)&content_buffer;
  topic_light.max_age = COAP_MAX_AGE*4;

  topic_mcu_sleep.content_type = 0;
  topic_mcu_sleep.content = (uint8_t *)&content_buffer;
  topic_mcu_sleep.max_age = COAP_MAX_AGE*4;

  topic_seconds.content_type = 0;
  topic_seconds.content = (uint8_t *)&content_buffer;
  topic_seconds.max_age = COAP_MAX_AGE*4;

  topic_bootcause.content_type = 0;
  topic_bootcause.content = (uint8_t *)&content_buffer;
  topic_bootcause.max_age = COAP_MAX_AGE*4;

  topic_no2.content_type = 0;
  topic_no2.content = (uint8_t *)&content_buffer;
  topic_no2.max_age = COAP_MAX_AGE*4;

  for(i = 0; i < 8; i++) {
    sprintf(((char *)&topic_dir.url) + 2 * i * sizeof(char), "%02x", uip_lladdr.addr[i]);
  }
  strncat((char *)&topic_dir.url, "/", 1);
  PRINTF("Node-ID=%s\n", topic_dir.url);

  strcpy(topic_seconds.url, topic_dir.url);
  sprintf(((char *)&topic_seconds.url) + strlen(topic_dir.url) * sizeof(char), "seconds/");
  PRINTF("seconds topic url = %s\n", topic_seconds.url);

  strcpy(topic_light.url, topic_dir.url);
  sprintf(((char *)&topic_light.url) + strlen(topic_dir.url) * sizeof(char), "light/");
  PRINTF("light topic url = %s\n", topic_light.url);

  strcpy(topic_mcu_sleep.url, topic_dir.url);
  sprintf(((char *)&topic_mcu_sleep.url) + strlen(topic_dir.url) * sizeof(char), "mcu_sleep/");
  PRINTF("mcu_sleep topic url = %s\n", topic_mcu_sleep.url);

  strcpy(topic_bootcause.url, topic_dir.url);
  sprintf(((char *)&topic_bootcause.url) + strlen(topic_dir.url) * sizeof(char), "bootcause/");
  PRINTF("bootcause topic url = %s\n", topic_bootcause.url);

  strcpy(topic_no2.url, topic_dir.url);
  sprintf(((char *)&topic_no2.url) + strlen(topic_dir.url) * sizeof(char), "no2/");
  PRINTF("no2 topic url = %s\n", topic_no2.url);

  
  if(i2c_probed & I2C_BME280 ) {
    strcpy(topic_bme.url, topic_dir.url);
    sprintf(((char *)&topic_bme.url) + strlen(topic_dir.url) * sizeof(char), "T_RH_P/");
    PRINTF("T_RH_P topic url = %s\n", topic_bme.url);
  }

  if(i2c_probed & I2C_PMS5003 ) {
    strcpy(topic_pm.url, topic_dir.url);
    sprintf(((char *)&topic_pm.url) + strlen(topic_dir.url) * sizeof(char), "pm/");
    PRINTF("PM topic url = %s\n", topic_pm.url);
  }

#ifdef COAP_CONF_PUBLISH_INTERVAL
  etimer_set(&publish_timer, COAP_CONF_PUBLISH_INTERVAL);
#else
  etimer_set(&publish_timer, CLOCK_SECOND * 5);
#endif 
  coap_pubsub_init_client();

  PROCESS_PAUSE();
  
  while(1) {
    //PROCESS_WAIT_EVENT();
    PROCESS_YIELD();
    
    if(etimer_expired(&publish_timer)){
      etimer_reset(&publish_timer);
      
      if(!found_broker){
	PRINTF("Trying server ");
	PRINT6ADDR(&broker.address);
	PRINTF(" local/remote port %u/%u\n",
	       UIP_HTONS(broker.port), UIP_HTONS(broker.port));

	PRINTF("DISCOVER function set\n");
	COAP_PUBSUB_DISCOVER(NULL, &broker);
	PRINTF("DISCOVER finished, return code %d\n", broker.last_response_code);
	
	if(strlen(broker.base_url) > 1){
	  PRINTF("Broker function set at: %s\n", broker.base_url);

	  COAP_PUBSUB_CREATE(&topic_dir);
	  PRINTF("CREATE %s, return code %d\n", topic_dir.url, topic_dir.last_response_code);
	  
	  if(topic_dir.last_response_code == CREATED_2_01 
	     || topic_dir.last_response_code == FORBIDDEN_4_03){
	    
	    COAP_PUBSUB_CREATE(&topic_seconds);
	    PRINTF("CREATE %s, return code %d\n", topic_seconds.url, topic_seconds.last_response_code);
	    if(topic_seconds.last_response_code == CREATED_2_01 
	       || topic_seconds.last_response_code == FORBIDDEN_4_03){
	      found_broker = 1;
	    }

	    COAP_PUBSUB_CREATE(&topic_light);
	    PRINTF("CREATE %s, return code %d\n", topic_light.url, topic_light.last_response_code);
	    if(topic_light.last_response_code == CREATED_2_01 
	       || topic_light.last_response_code == FORBIDDEN_4_03){
	      found_broker = 1;
	    }

	    COAP_PUBSUB_CREATE(&topic_mcu_sleep);
	    PRINTF("CREATE %s, return code %d\n", topic_mcu_sleep.url, topic_mcu_sleep.last_response_code);
	    if(topic_mcu_sleep.last_response_code == CREATED_2_01 
	       || topic_mcu_sleep.last_response_code == FORBIDDEN_4_03){
	      found_broker = 1;
	    }

	    COAP_PUBSUB_CREATE(&topic_bootcause);
	    PRINTF("CREATE %s, return code %d\n", topic_bootcause.url, topic_bootcause.last_response_code);
	    if(topic_bootcause.last_response_code == CREATED_2_01 
	       || topic_bootcause.last_response_code == FORBIDDEN_4_03){
	      found_broker = 1;
	    }
	    
	    COAP_PUBSUB_CREATE(&topic_no2);
	    PRINTF("CREATE %s, return code %d\n", topic_no2.url, topic_no2.last_response_code);
	    if(topic_no2.last_response_code == CREATED_2_01 
	       || topic_no2.last_response_code == FORBIDDEN_4_03){
	      found_broker = 1;
	    }

	    if(i2c_probed & I2C_BME280 ) {
	      COAP_PUBSUB_CREATE(&topic_bme);
	      PRINTF("CREATE %s, return code %d\n", topic_bme.url, topic_bme.last_response_code);
	      if(topic_bme.last_response_code == CREATED_2_01 
		 || topic_bme.last_response_code == FORBIDDEN_4_03){
		found_broker = 1;
	      }
	    }

	    if(i2c_probed & I2C_PMS5003 ) {
	      COAP_PUBSUB_CREATE(&topic_pm);
	      PRINTF("CREATE %s, return code %d\n", topic_pm.url, topic_pm.last_response_code);
	      if(topic_pm.last_response_code == CREATED_2_01 
		 || topic_pm.last_response_code == FORBIDDEN_4_03){
		found_broker = 1;
	      }
	    }
	  }
	}
      }

      else {

	remaining = COAP_PUBSUB_MAX_CREATE_MESSAGE_LEN;
	buf_ptr = (char *) topic_seconds.content;
	PUTFMT("%-lu ",  clock_seconds())
	topic_seconds.content_len = strlen((char *)topic_seconds.content); 
	PRINTF("PUBLISH SECONDS value %s, len %u\n", topic_seconds.content, topic_seconds.content_len);
	COAP_PUBSUB_PUBLISH(&topic_seconds);
	PRINTF("PUBLISH finished, return code %d\n", topic_seconds.last_response_code );
	if(topic_seconds.last_response_code == NOT_FOUND_4_04){
	  PRINTF("No topic SECONDS!\n");
	  found_broker = 0;
	}

	remaining = COAP_PUBSUB_MAX_CREATE_MESSAGE_LEN;
	buf_ptr = (char *) topic_light.content;
	PUTFMT("%-u ",  light_sensor.value(0));
	topic_light.content_len = strlen((char *)topic_light.content); 
	PRINTF("PUBLISH LIGHT value %s, len %u\n", topic_light.content, topic_light.content_len);
	COAP_PUBSUB_PUBLISH(&topic_light);
	PRINTF("PUBLISH finished, return code %d\n", topic_light.last_response_code );
	if(topic_light.last_response_code == NOT_FOUND_4_04){
	  PRINTF("No topic LIGHT!\n");
	  found_broker = 0;
	}

	remaining = COAP_PUBSUB_MAX_CREATE_MESSAGE_LEN;
	buf_ptr = (char *) topic_mcu_sleep.content;
	PUTFMT("%lu ",  mcu_sleep);
	topic_mcu_sleep.content_len = strlen((char *)topic_mcu_sleep.content); 
	PRINTF("PUBLISH MCU_SLEEP value %s, len %u\n", topic_mcu_sleep.content, topic_mcu_sleep.content_len);
	COAP_PUBSUB_PUBLISH(&topic_mcu_sleep);
	PRINTF("PUBLISH finished, return code %d\n", topic_mcu_sleep.last_response_code );
	if(topic_mcu_sleep.last_response_code == NOT_FOUND_4_04){
	  PRINTF("No topic MCU_SLEEP!\n");
	  found_broker = 0;
	}

	remaining = COAP_PUBSUB_MAX_CREATE_MESSAGE_LEN;
	buf_ptr = (char *) topic_bootcause.content;
	PUTFMT("%u ",  GPIOR0);
	topic_bootcause.content_len = strlen((char *)topic_bootcause.content); 
	PRINTF("PUBLISH BOOTCAUSE value %s, len %u\n", topic_bootcause.content, topic_bootcause.content_len);
	COAP_PUBSUB_PUBLISH(&topic_bootcause);
	PRINTF("PUBLISH finished, return code %d\n", topic_bootcause.last_response_code );
	if(topic_bootcause.last_response_code == NOT_FOUND_4_04){
	  PRINTF("No topic BOOTCAUSE!\n");
	  found_broker = 0;
	}

	remaining = COAP_PUBSUB_MAX_CREATE_MESSAGE_LEN;
	buf_ptr = (char *) topic_no2.content;
	{
	  double ppb =  no2(no2_sen_2);
	  PUTFMT("%-5.0f %-5.0f", ppb, ppb2ugm3(ppb, NO2_MW, no2_temp()));
	}
	topic_no2.content_len = strlen((char *)topic_no2.content); 
	PRINTF("PUBLISH NO2 value %s, len %u\n", topic_no2.content, topic_no2.content_len);
	COAP_PUBSUB_PUBLISH(&topic_no2);
	PRINTF("PUBLISH finished, return code %d\n", topic_no2.last_response_code );
	if(topic_no2.last_response_code == NOT_FOUND_4_04){
	  PRINTF("No topic NO2!\n");
	  found_broker = 0;
	}

	if(0 && i2c_probed & I2C_BME680) {
	  remaining = COAP_PUBSUB_MAX_CREATE_MESSAGE_LEN;
	  buf_ptr = (char *) topic_bme.content;
	  /* Trigger burst read */
	  bme680_sensor.value(BME680_SENSOR_TEMP);
	  PUTFMT("%-5.2f ", (double)bme680.temp / 100.);
	  PUTFMT("%-5.2f ", (double)bme680.hum / 1024.);
	  PUTFMT("%-5.0f ", (double)bme680.pres);
	  PUTFMT("%ld ", bme680.gas.res);
	  topic_bme.content_len = strlen((char *)topic_bme.content);
	  PRINTF("PUBLISH BME280 value %s, len %u\n", topic_bme.content, topic_bme.content_len);
	  COAP_PUBSUB_PUBLISH(&topic_bme);
	  PRINTF("PUBLISH finished, return code %d\n", topic_bme.last_response_code );
	  if(topic_bme.last_response_code == NOT_FOUND_4_04){
	    PRINTF("No topic BME!\n");
	    found_broker = 0;
	  }
	}
	else if (i2c_probed & I2C_BME280) {


	  remaining = COAP_PUBSUB_MAX_CREATE_MESSAGE_LEN;
	  buf_ptr = (char *) topic_bme.content;
	  /* Trigger burst read */

	  MCU_SLEEP_DISABLE();
	  bme280_sensor.value(BME280_SENSOR_TEMP);
	  MCU_SLEEP_ENABLE();
	  
	  PUTFMT("%-5.2f ", (double)bme280_mea.t_overscale100 / 100.);
	  PUTFMT("%-5.2f ", (double)bme280_mea.h_overscale1024 / 1024.);
	  PUTFMT("%-5.0f ", (double)bme280_mea.p_overscale256 / 256.);

	  topic_bme.content_len = strlen((char *)topic_bme.content); 
	  PRINTF("PUBLISH BME280 value %s, len %u\n", topic_bme.content, topic_bme.content_len);    
	  COAP_PUBSUB_PUBLISH(&topic_bme);
	  PRINTF("PUBLISH finished, return code %d\n", topic_bme.last_response_code );
	  if(topic_bme.last_response_code == NOT_FOUND_4_04){
	    PRINTF("No topic BME!\n");
	    found_broker = 0;
	  }
	}

	if (i2c_probed & I2C_PMS5003 && pms5003_sensor.value(PMS5003_SENSOR_TIMESTAMP) != 0) {
	  remaining = COAP_PUBSUB_MAX_CREATE_MESSAGE_LEN;
	  buf_ptr = (char *) topic_pm.content;
	  PUTFMT("%d ", pms5003_sensor.value(PMS5003_SENSOR_PM1));
	  PUTFMT("%d ", pms5003_sensor.value(PMS5003_SENSOR_PM2_5));
	  PUTFMT("%d ", pms5003_sensor.value(PMS5003_SENSOR_PM10));
	  PUTFMT("ATM %d ", pms5003_sensor.value(PMS5003_SENSOR_PM1_ATM));
	  PUTFMT("%d ", pms5003_sensor.value(PMS5003_SENSOR_PM2_5_ATM));
	  PUTFMT("%d ", pms5003_sensor.value(PMS5003_SENSOR_PM10_ATM));
	  
	  PUTFMT("DB %d ", pms5003_sensor.value(PMS5003_SENSOR_DB0_3));
	  PUTFMT("%d ", pms5003_sensor.value(PMS5003_SENSOR_DB0_5));
	  PUTFMT("%d ", pms5003_sensor.value(PMS5003_SENSOR_DB1));
	  PUTFMT("%d ", pms5003_sensor.value(PMS5003_SENSOR_DB2_5));
	  PUTFMT("%d ", pms5003_sensor.value(PMS5003_SENSOR_DB5));
	  PUTFMT("%d ", pms5003_sensor.value(PMS5003_SENSOR_DB10));

	  topic_pm.content_len = strlen((char *)topic_pm.content);
	  PRINTF("PUBLISH PMSX003 value %s, len %u\n", topic_pm.content, topic_pm.content_len);    
	  COAP_PUBSUB_PUBLISH(&topic_pm);
	  PRINTF("PUBLISH finished, return code %d\n", topic_pm.last_response_code );
	  if(topic_pm.last_response_code == NOT_FOUND_4_04){
	    PRINTF("No topic PM!\n");
	    found_broker = 0;
	  }
	}
      }
    }
    etimer_restart(&publish_timer);
  }
  PROCESS_END();
 }
