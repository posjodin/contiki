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
AUTOSTART_PROCESSES(&tcp_dummy_process);
/*---------------------------------------------------------------------------*/
/* Prototypes */
static int
tcp_input(struct tcp_socket *s, void *ptr, const uint8_t *input_data_ptr,
          int input_data_len);

static void tcp_event(struct tcp_socket *s, void *ptr,
                      tcp_socket_event_t event);

/*---------------------------------------------------------------------------*/
static void
abort_connection(struct dummy_connection *conn)
{
  tcp_socket_close(&conn->socket);
}

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
/*---------------------------------------------------------------------------*/
static void
disconnect_tcp(struct dummy_connection *conn)
{
  tcp_socket_close(&(conn->socket));
  tcp_socket_unregister(&conn->socket);

  memset(&conn->socket, 0, sizeof(conn->socket));
}
/*---------------------------------------------------------------------------*/
static void
send_out_buffer(struct dummy_connection *conn)
{
  if(conn->out_buffer_ptr - conn->out_buffer == 0) {
    conn->out_buffer_sent = 1;
    return;
  }
  conn->out_buffer_sent = 0;

  DBG("Dummy - (send_out_buffer) Space used in buffer: %i\n",
      conn->out_buffer_ptr - conn->out_buffer);

  tcp_socket_send(&conn->socket, conn->out_buffer,
                  conn->out_buffer_ptr - conn->out_buffer);
}
/*---------------------------------------------------------------------------*/
static int
tcp_input(struct tcp_socket *s,
          void *ptr,
          const uint8_t *input_data_ptr,
          int input_data_len)
{
  struct dummy_connection *conn = ptr;

  if(input_data_len == 0) {
    return 0;
  }

  DBG("tcp_input with %i bytes of data:\n", input_data_len);
  return 0;
}
/*---------------------------------------------------------------------------*/
/*
 * Handles TCP events from Simple TCP
 */
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

    DBG("Dummy - Disconnected by tcp event %d\n", event);
    /* Clear socket */
    tcp_socket_unregister(&conn->socket);
    memset(&conn->socket, 0, sizeof(conn->socket));

    break;
  }
  case TCP_SOCKET_CONNECTED: {
    printf("Dummy -- connected\n");
    break;
  }
  case TCP_SOCKET_DATA_SENT: {
    DBG("Dummy - Got TCP_DATA_SENT\n");

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
  at_radio_init();
  while(1) {
    static int i;
    PROCESS_WAIT_EVENT();

    if (ev == at_radio_ev_init) {
      printf("Dummy: radio is init\n");
      //connect_tcp(conn);
      i = 0;
      while (1) {
        i++;
        etimer_set(&et, 10*CLOCK_SECOND);
        while (!etimer_expired(&et)) {
          PROCESS_PAUSE();
        } 
        if (i == 2) {
          printf("Dummy connect\n");
          connect_tcp(conn);
        }
        else if (i > 3) {
          static char buf[16];
          sprintf(buf, "HEJ %d ", i);
          tcp_socket_send(&conn->socket, buf, strlen(buf)-1);
        }
      }
    }
  }
  PROCESS_END();
}

/*----------------------------------------------------------------------------*/
/** @} */
