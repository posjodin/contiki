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
 *      Project config file for copa-pubsub examples
 * \author
 *      Jussi Haikara <haikara@kth.se>
 */

#define UIP_CONF_TCP 0

/* The observe client is required to subscribe to topics. Removing it can free memory. */
#define COAP_OBSERVE_CLIENT 1
#define COAP_MAX_AGE 240
/* KTH configuration: this is a default configuration */
#define COAP_DEMO_TOPIC_BASE 	"KTH/avr-rss2"
//#define COAP_DEMO_BROKER_IP_ADDR "0064:ff9b::c010:7dea"
#define COAP_DEMO_BROKER_IP_ADDR "0064:ff9b::c010:7de8"
//#define COAP_DEMO_BROKER_IP_ADDR "::ffff:c010:7de8"
#define RPL_CONF_DEFAULT_INSTANCE 0x1d
#define IEEE802154_CONF_PANID 0xFEED
#define CHANNEL_CONF_802_15_4 15
//#define CHANNEL_CONF_802_15_4 26
#define RPL_CONF_WITH_DAO_ACK 1


#define NETSTACK_CONF_FRAMER      framer_802154
#define NETSTACK_CONF_RADIO       rf230_driver

//#define RDC NORDC
//#define NETSTACK_CONF_RDC nullrdc_driver
//#define NETSTACK_CONF_MAC nullmac_driver

#define RDC CONTIKIMAC
#define NETSTACK_CONF_MAC         csma_driver
#define NETSTACK_CONF_RDC         contikimac_driver
#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8

#define COAP_CONF_PUBLISH_INTERVAL  (10 * CLOCK_SECOND)

#define MCU_SLEEP_STATS   1

/* The observer URL length needs to be set long enough to conation the topic URL. */
/* This needs to be set in /apps/er-coap/er-coap-observe.h for the time being. */
//#define COAP_OBSERVER_URL_LEN 32

