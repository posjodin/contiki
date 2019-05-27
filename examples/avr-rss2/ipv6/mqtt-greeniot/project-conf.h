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
#define MQTT_CONF_PUBLISH_INTERVAL    (5 * CLOCK_SECOND)

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
#define NULLRDC_CONF_FRAME_RETRIES_SW 1

//#define TESTBED_UPWIS_CONF 1
/* uncomment the line above to use UPWIS configuration */
#ifdef TESTBED_UPWIS_CONF /* UPWIS configuration */
#define MQTT_DEMO_TOPIC_BASE 	"greeniot/KTH/avr-rss2"
#define MQTT_DEMO_BROKER_IP_ADDR "bbbb::1"
#define RPL_CONF_DEFAULT_INSTANCE 0x1e
#define IEEE802154_CONF_PANID 0x5EE9
#define CHANNEL_CONF_802_15_4 20
#define RPL_CONF_LEAF_ONLY  1
#define RPL_CONF_WITH_DAO_ACK 1
#else	/* KTH configuration: this is a default configuration */
#define MQTT_DEMO_TOPIC_BASE 	"KTH/avr-rss2"
#define MQTT_DEMO_BROKER_IP_ADDR "0064:ff9b::c010:7dea"
//#define MQTT_DEMO_BROKER_IP_ADDR "0064:ff9b::c0a8:0103"
#define RPL_CONF_DEFAULT_INSTANCE 0x1d
#define IEEE802154_CONF_PANID 0xFEED
//#define IEEE802154_CONF_PANID 0xABBA
//#define CHANNEL_CONF_802_15_4 12
#define CHANNEL_CONF_802_15_4 25
#define RPL_CONF_WITH_DAO_ACK 1
#endif /* TESTBED_UPWIS_CONF */

#define RPL_CONF_STATS 1
#define RF230_DEBUG 1
#define MQTT_CONF_KEEP_ALIVE_TIMER 120

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

#define USART1_CONF_ENABLE 1

/* M-Bus Settings */
//#define USART1_CONF_BAUD_RATE   BAUD_RATE(300)
#define USART1_CONF_BAUD_RATE   USART_BAUD_9600
#define USART1_CONF_CSRC USART_PARITY_EVEN|USART_STOP_BITS_1|USART_DATA_BITS_8

/* Use MQTT CLI? */
//#define MQTT_CLI	1

/*---------------------------------------------------------------------------*/
#endif /* PROJECT_CONF_H_ */
/*---------------------------------------------------------------------------*/
/** @} */
