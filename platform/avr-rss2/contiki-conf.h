/*
 * Copyright (c) 2006, Technical University of Munich
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
 * @(#)$$
 */

/**
 * \file
 *         Configuration for RSS2 (radio-sensors.com) platform
 */

#ifndef CONTIKI_CONF_H_
#define CONTIKI_CONF_H_

#include <stdint.h>

/* include the project config */
/* PROJECT_CONF_H might be defined in the project Makefile */
#ifdef PROJECT_CONF_H
#include PROJECT_CONF_H
#endif

/* Platform name, type, and MCU clock rate */
#define PLATFORM_NAME  "rss2"
#define PLATFORM_TYPE  ATMEGA256RFR2
#ifndef F_CPU
#define F_CPU          8000000UL
#endif

#define MCUCSR  MCUSR

/* RDC Selector. Needs to in sync with project-conf.h */
#define NORDC       1
#define TSCH        2
#define CONTIKIMAC  3

#ifndef RDC
#define RDC  CONTIKIMAC
#endif

#if RDC == TSCH
/* Delay between GO signal and SFD */
#define RADIO_DELAY_BEFORE_TX ((unsigned)US_TO_RTIMERTICKS(204)) //204
/* Delay between GO signal and start listening */
#define RADIO_DELAY_BEFORE_RX ((unsigned)US_TO_RTIMERTICKS(204))
/* Delay between the SFD finishes arriving and it is detected in software */
#define RADIO_DELAY_BEFORE_DETECT ((unsigned)US_TO_RTIMERTICKS(120)) //30

#ifndef TSCH_CONF_TIMESYNC_REMOVE_JITTER 
#define TSCH_CONF_TIMESYNC_REMOVE_JITTER 1
#endif

/* Use hardware timestamps */
#ifndef TSCH_CONF_RESYNC_WITH_SFD_TIMESTAMPS
#define TSCH_CONF_RESYNC_WITH_SFD_TIMESTAMPS 0
#endif

#ifndef TSCH_CONF_BASE_DRIFT_PPM
/* The drift compared to "true" 10ms slots.
 * Enable adaptive sync to enable compensation for this. */
//#define TSCH_CONF_BASE_DRIFT_PPM +977
#endif

#ifndef TSCH_CONF_RX_WAIT
#define TSCH_CONF_RX_WAIT 1800
#endif

#define WITH_SEND_CCA 0
#define RF230_CONF_AUTOACK 0
#define RF230_CONF_AUTORETRIES 0

//#define TSCH_DEBUG 0
#if TSCH_DEBUG
#define TSCH_DEBUG_INIT() do {DDRD |= (1<<PD6);} while(0);
#define TSCH_DEBUG_SLOT_START() do{ PORTD |= (1<<PD6); } while(0);
#define TSCH_DEBUG_SLOT_END() do{ PORTD &= ~(1<<PD6); } while(0);
#endif /* TSCH_DEBUG */
#endif /* RDC == TSCH */


#ifndef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC     csma_driver
#endif

#ifndef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     contikimac_driver
#endif

#ifndef NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE
#define NETSTACK_CONF_RDC_CHANNEL_CHECK_RATE 8
#endif

#ifndef NETSTACK_CONF_FRAMER
#if NETSTACK_CONF_WITH_IPV6
#define NETSTACK_CONF_FRAMER  contikimac_framer
#else
#define NETSTACK_CONF_FRAMER  framer_802154
#endif
#endif /* RDC == TSCH */

#ifndef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO   rf230_driver
#endif

/* AUTOACK receive mode gives better rssi measurements, even if ACK is never requested */
#ifndef RF230_CONF_AUTOACK
#define RF230_CONF_AUTOACK        1
#endif

/*
   TX Filter. Be careful. This setting impacts RF-emission. And might violate CE, FCC
   regulations. RS boards has separate bandpass filter and conform to CE with any setting.
 */
#ifndef RF230_CONF_PLL_TX_FILTER
#define RF230_CONF_PLL_TX_FILTER    0
#endif

#define TIMESYNCH_CONF_ENABLED 0
#define RF230_CONF_TIMESTAMPS 0

/* The AVR tick interrupt usually is done with an 8 bit counter around 128 Hz.
 * 125 Hz needs slightly more overhead during the interrupt, as does a 32 bit
 * clock_time_t.
 */
/* Clock ticks per second */
#define CLOCK_CONF_SECOND 128
/* RSS2 boards has a 32768Hz on TIMER2 */
#define AVR_CONF_USE32KCRYSTAL 1
/* The radio needs to interrupt during an rtimer interrupt */
#define RTIMER_CONF_NESTED_INTERRUPTS 1
#define RTIMER_ARCH_PRESCALER 256
#define RTIMER_ARCH_CORRECTION -1

typedef unsigned long clock_time_t;
#define CLOCK_LT(a, b)  ((signed long)((a) - (b)) < 0)
#define INFINITE_TIME 0xffffffff
/* These routines are not part of the contiki core but can be enabled in cpu/avr/clock.c */
void clock_delay_msec(uint16_t howlong);
void clock_adjust_ticks(clock_time_t howmany);

/* The radio needs to interrupt during an rtimer interrupt */
#define RTIMER_CONF_NESTED_INTERRUPTS 1

/* RSS2 boards has a 32768Hz on TIMER2 */
#define AVR_CONF_USE32KCRYSTAL 1

#ifdef USART0_CONF_CSRC
#define USART0_CSRC USART0_CONF_CSRC
#else
#define USART0_CSRC USART_PARITY_NONE|USART_STOP_BITS_1|USART_DATA_BITS_8
#endif

#ifdef USART1_CONF_CSRC
#define USART1_CSRC USART1_CONF_CSRC
#else
#define USART1_CSRC USART_PARITY_NONE|USART_STOP_BITS_1|USART_DATA_BITS_8
#endif

#ifndef USART0_CONF_BAUD_RATE
#define USART0_CONF_BAUD_RATE   USART_BAUD_38400 /**< Default USART0 baud rate */
#endif

#ifndef USART1_CONF_BAUD_RATE
#define USART1_CONF_BAUD_RATE   USART_BAUD_38400 /**< Default USART1 baud rate */
#endif

#ifndef SERIAL_LINE_CONF_UART
#define SERIAL_LINE_CONF_UART       0 /**< USART to use with serial line */
#endif

#ifndef SLIP_ARCH_CONF_USART
#define SLIP_ARCH_CONF_USART         0 /**< USART to use with SLIP */
#endif

#define SLIP_PORT SLIP_ARCH_CONF_USART

/* Pre-allocated memory for loadable modules heap space (in bytes)*/
/* Default is 4096. Currently used only when elfloader is present. Not tested on Raven */
/* #define MMEM_CONF_SIZE 256 */

/* Starting address for code received via the codeprop facility. Not tested. */
//typedef unsigned long off_t;
/* #define EEPROMFS_ADDR_CODEPROP 0x8000 */

/* Logging adds 200 bytes to program size. RS232 output slows down webserver. */
/* #define LOG_CONF_ENABLED         1 */

/* Saves some cycles for etimer scheduling from ISR */
#define OPTIMIZE_ETIMER_POLL 1

/* RADIOSTATS is used in rf230bb, clock.c and the webserver cgi to report radio usage */
/* It has less overhead than ENERGEST */
#define RADIOSTATS                1

/* More extensive stats, via main loop printfs or webserver status pages */
#define ENERGEST_CONF_ON          0

/* Packet statistics */
typedef unsigned short uip_stats_t;
#define UIP_STATISTICS            0

/* Available watchdog timeouts depend on mcu. Default is WDTO_2S. -1 Disables the watchdog. */
/* AVR Studio simulator tends to reboot due to clocking the WD 8 times too fast */
/* #define WATCHDOG_CONF_TIMEOUT -1 */

/* Debugflow macro, useful for tracing path through mac and radio interrupts */
/* #define DEBUGFLOWSIZE 128 */

/* Define MAX_*X_POWER to reduce tx power and ignore weak rx packets for testing a miniature multihop network.
 * Leave undefined for full power and sensitivity.
 * tx=0 (3dbm, default) to 15 (-17.2dbm)
 * RF230_CONF_AUTOACK sets the extended mode using the energy-detect register with rx=0 (-91dBm) to 84 (-7dBm)
 *   else the rssi register is used having range 0 (91dBm) to 28 (-10dBm)
 *   For simplicity RF230_MIN_RX_POWER is based on the energy-detect value and divided by 3 when autoack is not set.
 * On the RF230 a reduced rx power threshold will not prevent autoack if enabled and requested.
 * These numbers applied to both Raven and Jackdaw give a maximum communication distance of about 15 cm
 * and a 10 meter range to a full-sensitivity RF230 sniffer.
 *#define RF230_MAX_TX_POWER 15
 *#define RF230_MIN_RX_POWER 30
 */
/* The rf231 and atmega128rfa1 can use an rssi threshold for triggering rx_busy that saves 0.5ma in rx mode */
/* 1 - 15 maps into -90 to -48 dBm; the register is written with RF230_MIN_RX_POWER/6 + 1. Undefine for -100dBm sensitivity */
/* #define RF230_MIN_RX_POWER        0 */

/* Network setup */
/* TX routine passes the cca/ack result in the return parameter */
#define RDC_CONF_HARDWARE_ACK    1
/* TX routine does automatic cca and optional backoffs */
#define RDC_CONF_HARDWARE_CSMA   1
/* Allow MCU sleeping between channel checks */
#define RDC_CONF_MCU_SLEEP       1

#if NETSTACK_CONF_WITH_IPV6

#ifndef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS     20
#endif 
#ifndef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES      20
#endif 
#ifndef UIP_CONF_BUFFER_SIZE
#define UIP_CONF_BUFFER_SIZE    1280
#endif
#define LINKADDR_CONF_SIZE        8
#define UIP_CONF_ICMP6            1
#define UIP_CONF_UDP              1
#ifndef UIP_CONF_TCP
#define UIP_CONF_TCP              1
#endif
#ifndef UIP_CONF_TCP_MSS
#define UIP_CONF_TCP_MSS         64
#endif
#ifndef NETSTACK_CONF_NETWORK
#define NETSTACK_CONF_NETWORK     sicslowpan_driver
#endif
#define SICSLOWPAN_CONF_COMPRESSION SICSLOWPAN_COMPRESSION_HC06
#else
/* ip4 should build but is largely untested */
#define LINKADDR_CONF_SIZE        2
#define NETSTACK_CONF_NETWORK     rime_driver
#endif

#define UIP_CONF_LL_802154        1
#define UIP_CONF_LLH_LEN          0

/* 10 bytes per stateful address context - see sicslowpan.c */
/* Default is 1 context with prefix aaaa::/64 */
/* These must agree with all the other nodes or there will be a failure to communicate! */
#define SICSLOWPAN_CONF_MAX_ADDR_CONTEXTS 1
#define SICSLOWPAN_CONF_ADDR_CONTEXT_0 { addr_contexts[0].prefix[0] = 0xaa; addr_contexts[0].prefix[1] = 0xaa; }
#define SICSLOWPAN_CONF_ADDR_CONTEXT_1 { addr_contexts[1].prefix[0] = 0xbb; addr_contexts[1].prefix[1] = 0xbb; }
#define SICSLOWPAN_CONF_ADDR_CONTEXT_2 { addr_contexts[2].prefix[0] = 0x20; addr_contexts[2].prefix[1] = 0x01; addr_contexts[2].prefix[2] = 0x49; addr_contexts[2].prefix[3] = 0x78, addr_contexts[2].prefix[4] = 0x1d; addr_contexts[2].prefix[5] = 0xb1; }

/* Take the default TCP maximum segment size for efficiency and simpler wireshark captures */
/* Use this to prevent 6LowPAN fragmentation (whether or not fragmentation is enabled) */
/* #define UIP_CONF_TCP_MSS       48 */

#define UIP_CONF_IP_FORWARD      0
#define UIP_CONF_FWCACHE_SIZE    0

#define UIP_CONF_IPV6_CHECKS     1
#define UIP_CONF_IPV6_QUEUE_PKT  1
#define UIP_CONF_IPV6_REASSEMBLY 0

#define UIP_CONF_UDP_CHECKSUMS   1
#define UIP_CONF_TCP_SPLIT       1
#define UIP_CONF_DHCP_LIGHT      1


#if 1  /* Contiki-mac radio cycling */

#define RIMESTATS_CONF_ENABLED                 0

/* A 0 here means non-extended mode; 1 means extended mode with no retry, >1 for retrys */
/* Contikimac strobes on its own, but hardware retries are faster */
#if RDC == TSCH
/* 1 + Number of auto retry attempts 0-15 (0 implies don't use extended TX_ARET_ON mode) */
#define RF230_CONF_FRAME_RETRIES  0
/* Number of csma retry attempts 0-5 in extended tx mode (7 does immediate tx with no csma) */
#define RF230_CONF_CSMA_RETRIES   0
#endif
#if RDC == CONTIKIMAC
#define RF230_CONF_FRAME_RETRIES  5
#define RF230_CONF_CSMA_RETRIES   2
#define CONTIKIMAC_FRAMER_CONF_SHORTEST_PACKET_SIZE   (43 - 18)  /* multicast RPL DIS length */
/* Not tested much yet -- packet loss with  */
#define CONTIKIMAC_CONF_WITH_PHASE_OPTIMIZATION 0
#define CONTIKIMAC_CONF_COMPOWER               0
#endif
#if RDC == NORDC
#define RF230_CONF_FRAME_RETRIES  5
#define RF230_CONF_CSMA_RETRIES   5
#endif

/* In extended mode, we don't get statistic on how many retransmission is done */
/* This may lead to wrong ETX calculation. We may want to retransmit in software instead */
/* We add NULLRDC_CONF_FRAME_RETRIES_SW as a generic parameter for this in nullrdc case */
/* It is expected to be used together with NULLRDC_CONF_802154_AUTOACK_HW */
/* We also add RF230_CONF_FRAME_RETRIES_SW for radio hardware specific configuration */
/* If NULLRDC_CONF_FRAME_RETRIES_SW is set, then we configure the radio accordingly */
#ifdef NULLRDC_CONF_FRAME_RETRIES_SW
#define RF230_CONF_FRAME_RETRIES_SW
#endif
/* Long csma backoffs will compromise radio cycling; set to 0 for 1 csma */
#define SICSLOWPAN_CONF_FRAG      1
#define SICSLOWPAN_CONF_MAXAGE    3
/* 54 bytes per queue ref buffer */
#define QUEUEBUF_CONF_REF_NUM     2
/* Allocate remaining RAM. Not much left due to queuebuf increase  */
#define UIP_CONF_MAX_CONNECTIONS  2
#define UIP_CONF_MAX_LISTENPORTS  4
#define UIP_CONF_UDP_CONNS        5
#define UIP_CONF_DS6_DEFRT_NBU    2
#define UIP_CONF_DS6_PREFIX_NBU   3
#define UIP_CONF_DS6_ADDR_NBU     3
#define UIP_CONF_DS6_MADDR_NBU    0
#define UIP_CONF_DS6_AADDR_NBU    0

#elif 0  /* cx-mac radio cycling */
/* RF230 does clear-channel assessment in extended mode (autoretries>0) */
/* These values are guesses */
#define RF230_CONF_FRAME_RETRIES  10
#define RF230_CONF_CSMA_RETRIES   2
#if RF230_CONF_CSMA_RETRIES
#define NETSTACK_CONF_MAC         nullmac_driver
#else
#define NETSTACK_CONF_MAC         csma_driver
#endif
#define NETSTACK_CONF_RDC         cxmac_driver
#define NETSTACK_CONF_FRAMER      framer_802154
#define SICSLOWPAN_CONF_FRAG      1
#define SICSLOWPAN_CONF_MAXAGE    3
#define CXMAC_CONF_ANNOUNCEMENTS  0
/* 54 bytes per queue ref buffer */
#define QUEUEBUF_CONF_REF_NUM     2
/* Allocate remaining RAM. Not much left due to queuebuf increase  */
#define UIP_CONF_MAX_CONNECTIONS  2
#define UIP_CONF_MAX_LISTENPORTS  4
#define UIP_CONF_UDP_CONNS        5
#define UIP_CONF_DS6_DEFRT_NBU    2
#define UIP_CONF_DS6_PREFIX_NBU   3
#define UIP_CONF_DS6_ADDR_NBU     3
#define UIP_CONF_DS6_MADDR_NBU    0
#define UIP_CONF_DS6_AADDR_NBU    0
/* Below gives 10% duty cycle, undef for default 5% */
/* #define CXMAC_CONF_ON_TIME (RTIMER_ARCH_SECOND / 80) */
/* Below gives 50% duty cycle */
/* #define CXMAC_CONF_ON_TIME (RTIMER_ARCH_SECOND / 16) */

#else
/* #error Network configuration not specified! */
#endif /* Network setup */

#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM         15
#endif

/* ************************************************************************** */
/* #pragma mark RPL Settings */
/* ************************************************************************** */
#if UIP_CONF_IPV6_RPL

#define UIP_CONF_ROUTER                 1
#define UIP_CONF_ND6_SEND_RA        0
#define UIP_CONF_ND6_REACHABLE_TIME     600000
#define UIP_CONF_ND6_RETRANS_TIMER      10000

/* For slow slip connections, to prevent buffer overruns */
/* #define UIP_CONF_RECEIVE_WINDOW 300 */
#undef UIP_CONF_FWCACHE_SIZE
#define UIP_CONF_FWCACHE_SIZE    30
#define UIP_CONF_BROADCAST       1
#define UIP_ARCH_IPCHKSUM        1
#define UIP_CONF_PINGADDRCONF    0
#define UIP_CONF_LOGGING         0

#endif /* RPL */

#define CCIF
#define CLIF
#ifndef CC_CONF_INLINE
#define CC_CONF_INLINE inline
#endif

#endif /* CONTIKI_CONF_H_ */
