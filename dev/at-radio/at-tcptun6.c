/*
 * Copyright (c) 2017, Copyright Robert Olsson
 * KTH Royal Institute of Technology NSLAB KISTA STOCHOLM
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
 * Author  : Robert Olsson roolss@kth.se
 * Created : 2017-05-22
 */

/**
 * \file
 *         A simple application showing sc16is I2C UART & GPIO
 *         Example uses avr-rss2 platform
 *         
 */

#include "contiki.h"
#include "sys/etimer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki-net.h"
#include "contiki-lib.h"

#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"

#include "i2c.h"
#include "dev/leds.h"
#include "dev/sc16is/sc16is.h"
#include "sc16is-common.h"
#include "tcp-socket-at-radio.h"
#include "at-radio.h"
//#include "gprs-a6.h"
#include "at-wait.h"
/*---------------------------------------------------------------------------*/
PROCESS(tcptun_at_radio, "TCP tunnel over AT_RADIO");
static int connected = 0;
struct tcp_socket_at_radio atsocket;
uint8_t inbuf[sizeof(uip_buf)];
uint8_t outbuf[sizeof(uip_buf)];

int
at_data_callback(struct tcp_socket_at_radio *atsptr, void *ptr, const uint8_t *input_data_ptr, int input_data_len) {
  printf("Here is data callback\n");
  return 0;
}
/*-------------------*/
void
at_event_callback(struct tcp_socket_at_radio *atsptr, void *ptr, tcp_socket_at_radio_event_t event) {
  printf("Here is event callback\n");
  if (event == TCP_SOCKET_CONNECTED) {
    printf("tnne connected!\n");
    connected = 1;
  }
}
/*-------------------*/
int 
attun6_output(void) {
  printf("Here is attun6_purpurtcpipoutput: \n");
  printf("uip_len %d, uip_buf 0x%x (sizeof uip_buf) %d\n",
         uip_len, (unsigned) uip_buf, sizeof(uip_buf));
  if (uip_buf[0] == 0x60) {
    /* IPv6 */
    int i;
    printf("ip6dst:");
    for (i = 24; i <40; i++) {
      printf(" %02x", (unsigned) uip_buf[i]);
    }
    printf("\n");
  }
  if (connected) {
    tcp_socket_at_radio_send(&atsocket, uip_buf, uip_len);
  }
  return 1;
}
/*-------------------*/
//char *tcptunserver = "::82ed:16db"; /*"::130.237.22.219";*/
//char *tcptunserver = "::c010:7dea"; /*"::192.16.125.234";*/
char *tcptunserver = "::c010:7de8"; /*"::192.16.125.232"*/
uint16_t tcptunport = 7020;

PROCESS_THREAD(tcptun_at_radio, ev, data) {
  uip_ip6addr_t ip6addr;

  PROCESS_BEGIN();
  //tcpip_set_outputfunc(at_radio_tcpipoutput);
  tcp_socket_at_radio_register(&atsocket, &atsocket,
                               inbuf, sizeof(inbuf),
                               outbuf, sizeof(outbuf),
                               at_data_callback, at_event_callback);

  PROCESS_WAIT_EVENT_UNTIL(ev == at_radio_ev_init);

  if(uiplib_ip6addrconv(tcptunserver, &ip6addr) == 0) {
    printf("Address foncer error fo\n");
  }
  else {
    int i;
    for (i = 0; i < sizeof(ip6addr); i++) {
      printf(" %02x", ((uint8_t *) &ip6addr)[i]);
    }
    printf("\n");
  }
  tcp_socket_at_radio_connect(&atsocket, &ip6addr, tcptunport);
  PROCESS_END();
}

void
attun6_init() {
  static uip_ipaddr_t loc_fipaddr;
  connected = 0;

  loc_fipaddr.u16[0] = 0xfd;
  loc_fipaddr.u16[1] = rand();
  loc_fipaddr.u16[2] = rand();  
  loc_fipaddr.u16[3] = rand();
  loc_fipaddr.u16[4] = rand();
  loc_fipaddr.u16[5] = rand();
  uip_ds6_set_addr_iid(&loc_fipaddr, &uip_lladdr);
  uip_ds6_addr_add(&loc_fipaddr, 0, ADDR_MANUAL);
  /* end Hack */


  process_start(&tcptun_at_radio, NULL);
}

const struct uip_fallback_interface attun6_interface = {
  attun6_init, attun6_output
};
