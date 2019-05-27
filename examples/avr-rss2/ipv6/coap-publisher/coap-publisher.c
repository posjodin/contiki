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

static broker_t broker;
static client_topic_t topic_dir, topic_bme, topic_pm, topic_light;
static uint8_t content_buffer[128];
static struct etimer publish_timer;
static char *buf_ptr;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coap_client, ev, data)
{
  static int found_broker = 0;
  int i, len, remaining = 0;

  PROCESS_BEGIN();

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

  broker.port = UIP_HTONS(COAP_DEFAULT_PORT);

  /* convert the to a numeric IPv6 address */
  if(uiplib_ip6addrconv(COAP_DEMO_BROKER_IP_ADDR, &broker.address) == 0) {
    return 0;
  }

  topic_dir.content_type = 40;

  topic_bme.content_type = 0;
  topic_bme.content = (uint8_t *)&content_buffer;
  topic_bme.max_age = COAP_MAX_AGE;

  topic_pm.content_type = 0;
  topic_pm.content = (uint8_t *)&content_buffer;
  topic_pm.max_age = COAP_MAX_AGE;

  topic_light.content_type = 0;
  topic_light.content = (uint8_t *)&content_buffer;
  topic_light.max_age = COAP_MAX_AGE;

  for(i = 0; i < 8; i++) {
    sprintf(((char *)&topic_dir.url) + 2 * i * sizeof(char), "%02x", uip_lladdr.addr[i]);
  }
  strncat((char *)&topic_dir.url, "/", 1);
  PRINTF("Node-ID=%s\n", topic_dir.url);

  strcpy(topic_light.url, topic_dir.url);
  sprintf(((char *)&topic_light.url) + strlen(topic_dir.url) * sizeof(char), "light/");
  PRINTF("light topic url = %s\n", topic_light.url);

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
	    
	    COAP_PUBSUB_CREATE(&topic_light);
	    PRINTF("CREATE %s, return code %d\n", topic_light.url, topic_light.last_response_code);
	    if(topic_light.last_response_code == CREATED_2_01 
	       || topic_light.last_response_code == FORBIDDEN_4_03){
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
	  bme280_sensor.value(BME280_SENSOR_TEMP);
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
