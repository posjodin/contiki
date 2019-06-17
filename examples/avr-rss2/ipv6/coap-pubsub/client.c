/*
 * Copyright (c) 2017, Jussi Haikara
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
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      CoAP pubsub client example for the avr-rss2.
 *
 *      Attempts to find a neigboring broker, and publishes the value of 
 *      the temperature sensor.
 *
 * \author
 *      Jussi Haikara <haikara@kth.se>
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

PROCESS(coap_client, "CoAP pubsub client");
AUTOSTART_PROCESSES(&coap_client);

static broker_t broker;
static client_topic_t topic_temp, topic_dir;
static uint8_t content_buffer[128];
static struct etimer publish_timer;

PROCESS_THREAD(coap_client, ev, data)
{
  static int found_broker = 0;
  int i;

  PROCESS_BEGIN();
  SENSORS_ACTIVATE(temp_sensor);

  topic_dir.broker = &broker;
  topic_temp.broker = &broker;

  broker.port = UIP_HTONS(COAP_DEFAULT_PORT);

  /* convert the to a numeric IPv6 address */
  if(uiplib_ip6addrconv(COAP_DEMO_BROKER_IP_ADDR, &broker.address) == 0) {
    return 0;
  }

  topic_dir.content_type = 40;
  topic_temp.content_type = 0;
  topic_temp.content = (uint8_t *)&content_buffer;
  topic_temp.max_age = 300;

  for(i = 0; i < 8; i++) {
    sprintf(((char *)&topic_dir.url) + 2 * i * sizeof(char), "%02x", uip_lladdr.addr[i]);
  }
  PRINTF("Using %s as node ID\n", topic_dir.url);

  strcpy(topic_temp.url, topic_dir.url);
  sprintf(((char *)&topic_temp.url) + strlen(topic_dir.url) * sizeof(char), "/temp");

  PRINTF("Temperature topic url = %s\n", topic_temp.url);
  etimer_set(&publish_timer, CLOCK_SECOND * 5);
  coap_pubsub_init_client();

  PROCESS_PAUSE();

  while(1) {
    PROCESS_WAIT_EVENT();

    if(etimer_expired(&publish_timer)){
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

            PRINTF("CREATE dir topic\n");
            COAP_PUBSUB_CREATE(&topic_dir);

            PRINTF("CREATE finished, return code %d\n", topic_dir.last_response_code);

            if(topic_dir.last_response_code == CREATED_2_01 
                || topic_dir.last_response_code == FORBIDDEN_4_03){
              PRINTF("Dir topic created\n");

              PRINTF("CREATE temperature topic\n");

              COAP_PUBSUB_CREATE(&topic_temp);

              PRINTF("CREATE finished, return code %d\n", topic_temp.last_response_code);

              if(topic_temp.last_response_code == CREATED_2_01 
                  || topic_temp.last_response_code == FORBIDDEN_4_03){
                PRINTF("Temperature topic created\n");

                found_broker = 1;
              }
            }
          }
      }
      else {
        sprintf((char *)topic_temp.content, "%-5.2f", ((double) temp_sensor.value(0)/100.));
        topic_temp.content_len = strlen((char *)topic_temp.content); /* Does not include the \0 of the string. */

        PRINTF("PUBLISH value %s, len %u\n", topic_temp.content, topic_temp.content_len);

        COAP_PUBSUB_PUBLISH(&topic_temp);

        PRINTF("PUBLISH finished, return code %d\n", topic_temp.last_response_code );

        if(topic_temp.last_response_code == NOT_FOUND_4_04){
          PRINTF("No topic!\n");
          found_broker = 0;
        }
      }
      etimer_restart(&publish_timer);
    }
  }
  PROCESS_END();
}
