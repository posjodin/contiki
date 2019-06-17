/*
 * Copyright (c) 2015, Texas Instruments Incorporated - http://www.ti.com/
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
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup mqtt-engine
 * @{
 */
/**
 * \file
 *    Implementation of the Contiki MQTT engine
 *
 * \author
 *    Texas Instruments
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "sys/pt.h"
#include "net/rpl/rpl.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/leds.h"

#include "tcp-socket.h"

#include "lib/assert.h"
#include "lib/list.h"
#include "sys/cc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dev/at-radio/at-radio.h"

#define PPP_6TO4_STATS
#ifdef PPP_6TO4_STATS
#define REPORT 30
struct timer reporttimer;
uint16_t ppp_6to4good, ppp_6to4bad, ppp_4to6good, ppp_4to6bad;
#endif /* PPP_6TO4_STATS */

struct dummy_connection {
  /* Used by the list interface, must be first in the struct */
  struct timer t;


  /* Outgoing data related */
  uint8_t *out_buffer_ptr;
  uint8_t out_buffer[512];
  uint8_t out_buffer_sent;
  uint32_t out_write_pos;
  uint16_t max_segment_size;

  /* Incoming data related */
  uint8_t in_buffer[512];

  /* TCP related information */
  char *server_host;
  uip_ipaddr_t server_ip;
  uint16_t server_port;
  struct tcp_socket socket;
} dummy_connection;

/*---------------------------------------------------------------------------*/
#define DEBUG 1
#if DEBUG
#define DBG printf
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define DBG(...) 
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
PROCESS(tcp_dummy_process, "TCP dummy process");
/*---------------------------------------------------------------------------*/
/* Prototypes */
static int
tcp_input(struct tcp_socket *s, void *ptr, const uint8_t *input_data_ptr,
          int input_data_len);

static void tcp_event(struct tcp_socket *s, void *ptr,
                      tcp_socket_event_t event);

/*---------------------------------------------------------------------------*/
static void
connect_tcp(struct dummy_connection *conn)
{
  tcp_socket_register(&(conn->socket),
                      conn,
                      conn->in_buffer,
                      512,
                      conn->out_buffer,
                      512,
                      tcp_input,
                      tcp_event);
#ifdef MQTT_GPRS 
  /* GPRS takes IP addresses as strings */
  tcp_socket_gprs_connect_strhost(&(conn->socket), conn->server_host, conn->server_port);
#else
  tcp_socket_connect(&(conn->socket), &(conn->server_ip), conn->server_port);
#endif /* MQTT_GPRS */
}

uint16_t dummy_connections;
/*---------------------------------------------------------------------------*/
static int
tcp_input(struct tcp_socket *s,
          void *ptr,
          const uint8_t *input_data_ptr,
          int input_data_len)
{

  if(input_data_len == 0) {
    return 0;
  }

  DBG("tcp_input with %i bytes of data:\n", input_data_len);
  return 0;
}
enum state {
  NONE, CONNECTING, CONNECTED, CLOSING
} state;

/*---------------------------------------------------------------------------*/
/*
 * Handles TCP events from Simple TCP
 */
uint16_t dummy_sent;
static void
tcp_event(struct tcp_socket *s, void *ptr, tcp_socket_event_t event)
{
  struct dummy_connection *conn = ptr;

  /* Take care of event */
  switch(event) {

  /* Fall through to manage different disconnect event the same way. */
  case TCP_SOCKET_CLOSED:
  case TCP_SOCKET_TIMEDOUT:
  case TCP_SOCKET_ABORTED: {

    printf("Dummy - Disconnected by tcp event %d\n", event);
    /* Clear socket */
    state = NONE;
    tcp_socket_unregister(&conn->socket);
    memset(&conn->socket, 0, sizeof(conn->socket));

    break;
  }
  case TCP_SOCKET_CONNECTED: {
    state = CONNECTED;
    dummy_connections++;
    printf("Dummy -- connected (%d)\n", dummy_connections);
    break;
  }
  case TCP_SOCKET_DATA_SENT: {
    dummy_sent++;
    DBG("Dummy - Got TCP_DATA_SENT %d\n", dummy_sent);

    if(conn->socket.output_data_len == 0) {
      conn->out_buffer_sent = 1;
      conn->out_buffer_ptr = conn->out_buffer;
    }

    break;
  }
  default: {
    DBG("Dummy - TCP Event %d is currently not managed by the tcp event callback\n",
        event);
  }
  }
}
/*---------------------------------------------------------------------------*/
//static char *host = "0064:ff9b::c010:7dea"; /* 192.16.125.234 */
static char *host = "0064:ff9b::c010:7de8"; /* 192.16.125.232 */

static uint16_t port = 7020;
PROCESS_THREAD(tcp_dummy_process, ev, data)
{
  static struct dummy_connection dummy_connection;
  static struct dummy_connection *conn = &dummy_connection;
  static struct etimer et;

  static struct timer connect_timer;
  
  PROCESS_BEGIN();
  uip_ip6addr_t ip6addr;
  uip_ipaddr_t *ipaddr;
  ipaddr = &ip6addr;

  conn->server_host = host;
  conn->server_port = port;
  conn->out_buffer_ptr = conn->out_buffer;

  /* convert the string IPv6 address to a numeric IPv6 address */
  if(uiplib_ip6addrconv(host, &ip6addr) == 0) {
    printf("Can't convert host address %s\n", host);
  }

  uip_ipaddr_copy(&(conn->server_ip), ipaddr);

  printf("Here is dummy!\n");
  dummy_sent = 0;
  dummy_connections = 0;
  timer_set(&connect_timer, 0);

  static int i;
  i = 0;
  timer_set(&reporttimer, REPORT*CLOCK_SECOND);
  while (1) {
    i++;
    etimer_set(&et, 5*CLOCK_SECOND);
    while (!etimer_expired(&et)) {
      PROCESS_PAUSE();
    } 
    static uint16_t attempt = 0;
    if ((state != CONNECTED) && timer_expired(&connect_timer)) {
      printf("Connect attempt %d\n", ++attempt);
      if (state == CONNECTING) {
        printf("dummy close\n");
        tcp_socket_close(&conn->socket);
        state = CLOSING;
      }
      else if (state == CLOSING) {
        printf("DUmmy already closing -- do nothing \n");
      }
      else {
        connect_tcp(conn);
        timer_set(&connect_timer, 30*CLOCK_SECOND);
        state = CONNECTING;
      }
    }
    else if (state == CONNECTED) {
      static char buf[16];
      attempt = 0;
      printf("Dummy - send %d\n", i);
      sprintf(buf, "HEJ %d ", i);
      tcp_socket_send(&conn->socket, (uint8_t *) buf, strlen(buf));
    }
#ifdef PPP_6TO4_STATS
    if (timer_expired(&reporttimer)) {
      extern uint16_t dummy_connections;
      printf("Dummy 6to4: good %d bad %d. 4to6: good %d bad %d.", ppp_6to4good, ppp_6to4bad, ppp_4to6good, ppp_4to6bad);
      printf(" %d connections", dummy_connections);
      printf(".\n");
      timer_reset(&reporttimer);
      print_local_addresses();
    }
#endif /* PPP_6TO4_STATS */
  }
  PROCESS_END();
}

static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  printf("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      printf(" ");
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }
}


/*----------------------------------------------------------------------------*/
/** @} */
