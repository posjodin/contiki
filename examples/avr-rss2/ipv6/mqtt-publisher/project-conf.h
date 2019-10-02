/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
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
 *
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
/**
 * \addtogroup cc2538-mqtt-demo
 * @{
 *
 * \file
 * Project specific configuration defines for the MQTT demo
 */
/*---------------------------------------------------------------------------*/
#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_
/*---------------------------------------------------------------------------*/
/* User configuration */
#define MQTT_DEMO_STATUS_LED      LEDS_YELLOW
#define MQTT_DEMO_PUBLISH_TRIGGER &button_right_sensor

#define MQTT_WATCHDOG

/* If undefined, the demo will attempt to connect to IBM's quickstart */
//#define MQTT_DEMO_BROKER_IP_ADDR "aaaa::1"
//#define MQTT_DEMO_BROKER_IP_ADDR "::ffff:c010:7dea" 
//#define MQTT_DEMO_BROKER_IP_ADDR "::ffff:c010:7dea" 
//#define MQTT_DEMO_BROKER_IP_ADDR "0064:ff9b:0000:0000:0000:0000:c010:7dea"

//#define NB_IOT_TELE2 1
//#define URBAN_ICT_CONF  1
#define NB_IOT_TELIA 1
#ifdef MQTT_GPRS
//#define MQTT_CONF_PUBLISH_INTERVAL    (30 * CLOCK_SECOND)
#define MQTT_CONF_PUBLISH_INTERVAL    (10 * CLOCK_SECOND)
#else
//#define MQTT_CONF_PUBLISH_INTERVAL    (60 * CLOCK_SECOND)
#define MQTT_CONF_KEEP_ALIVE_TIMER    300L
#define MQTT_CONF_PUBLISH_INTERVAL    (220L * CLOCK_SECOND)
#endif /* MQTT_GPRS */

#if defined(NB_IOT_TELE2)
#undef MQTT_CONF_PUBLISH_INTERVAL
#define MQTT_CONF_PUBLISH_INTERVAL    (120 * CLOCK_SECOND)
#endif


#ifdef URBAN_ICT_CONF 
//#define GPRS_CONF_APN "vnl"
#define GPRS_CONF_APN "4g.tele2.se"
#elif defined(GPRS_TELIA) 
#define GPRS_CONF_APN "online.telia.se"
#elif defined(NB_IOT_TELIA) 
#define GPRS_CONF_APN "lpwa.telia.iot" 
#elif defined(NB_IOT_TELE2) 
#define GPRS_CONF_APN "m2m.tele2.se"
#else
#define GPRS_CONF_APN "4g.tele2.se"
#endif

#define NETSTACK_CONF_RDC nullrdc_driver
#define NETSTACK_CONF_MAC nullmac_driver

//#define NETSTACK_CONF_MAC         csma_driver
//#define NETSTACK_CONF_RDC         contikimac_driver
#define NETSTACK_CONF_FRAMER      framer_802154
#define NETSTACK_CONF_RADIO       rf230_driver

#define BOARD_STRING  "greeniot"

//#define RS232_BAUDRATE USART_BAUD_9600

#define RPL_CONF_ACCEPT_DEFAULT_INSTANCE_ONLY 1
#define NULLRDC_CONF_802154_AUTOACK_HW  1

//#define TESTBED_UPWIS_CONF 1
/* uncomment the line above to use UPWIS configuration */
#ifdef TESTBED_UPWIS_CONF /* UPWIS configuration */
#define MQTT_DEMO_TOPIC_BASE 	"greeniot/KTH/avr-rss2"
#define MQTT_DEMO_BROKER_IP_ADDR "bbbb::1"
#define RPL_CONF_DEFAULT_INSTANCE 0x1e
#define IEEE802154_CONF_PANID 0x5EE9
#define CHANNEL_CONF_802_15_4 20
#elif defined(MQTT_GPRS) /* Native MQTT over GPRS */
#ifdef URBAN_ICT_CONF
#define MQTT_DEMO_TOPIC_BASE 	"UICTA/avr-rss2"
#else
//#define MQTT_DEMO_TOPIC_BASE 	"KTH/avr-rss2"
#define MQTT_DEMO_TOPIC_BASE 	"RO/nuppland/avr-rss2"
#endif
//#define MQTT_DEMO_BROKER_IP_ADDR "192.16.125.234" /* 192.16.125.234 */
#define MQTT_DEMO_BROKER_IP_ADDR "2001:6b0:32:13:221:5aff:fe17:662e"
#define RPL_CONF_DEFAULT_INSTANCE 0x1d
#define IEEE802154_CONF_PANID 0xFEED
#define CHANNEL_CONF_802_15_4 25

#else	/* KTH configuration: this is a default configuration */
#define MQTT_DEMO_TOPIC_BASE 	"KTH/avr-rss2"
#define MQTT_DEMO_BROKER_IP_ADDR "0064:ff9b::c010:7dea"
//#define MQTT_DEMO_BROKER_IP_ADDR "2001:6b0:32:13:221:5aff:fe17:662e"
#define RPL_CONF_DEFAULT_INSTANCE 0x1d
#define IEEE802154_CONF_PANID 0xFEED
#define CHANNEL_CONF_802_15_4 25
#endif /* TESTBED_UPWIS_CONF */

#define RPL_CONF_STATS 1
#define RF230_DEBUG 1

#ifdef NBIOT
#define HW_FLOW_NONE 1
#endif

/* PMSx003 sensors -- I2C and/or UART*/
#define PMS5003_CONF_SERIAL_I2C 1
#define PMS5003_CONF_SERIAL_UART 0
#if PMS5003_CONF_SERIAL_UART
#define RS232_BAUDRATE USART_BAUD_9600
#endif

/* cli config */
#define CLI_CONF_COMMAND_PROMPT  "KTH-MQTT> "
#define CLI_CONF_PROJECT  "GreenIoT V1.0 2017-03-13"

#define SERIAL_LINE_CONF_HUMAN 1

/* HW watchdog */
/* #define  WATCHDOG_CONF_TIMEOUT -1 */
/* #define  WATCHDOG_CONF_TIMEOUT WDTO_8S */

/* Use MQTT CLI? */
//#define MQTT_CLI

#define USART1_CONF_ENABLE 1 
#define USART1_CONF_BAUD_RATE   USART_BAUD_9600

/*---------------------------------------------------------------------------*/
#endif /* PROJECT_CONF_H_ */
/*---------------------------------------------------------------------------*/
/** @} */
