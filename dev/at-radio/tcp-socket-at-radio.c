/*
 * Copyright (c) 2012-2014, Thingsquare, http://www.thingsquare.com/.
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
 */

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

#include "contiki.h"
#include "sys/cc.h"
#include "contiki-net.h"

#include "lib/list.h"

#include "tcp-socket-at-radio.h"
#include "at-radio.h"
#include <string.h>

LIST(socketlist_at_radio);
/*---------------------------------------------------------------------------*/

static void
call_event(struct tcp_socket_at_radio *s, tcp_socket_at_radio_event_t event)
{
  if(s != NULL && s->event_callback != NULL) {
    s->event_callback(s, s->ptr, event);
  }
}
/*---------------------------------------------------------------------------*/
/* 
 * Unlike tcp-socket.c:senddata(), at_radio_send() will send all data before 
 * generating a callback() confirming data sent. 
 */
static void
senddata(struct tcp_socket_at_radio *s)
{
  struct at_radio_connection *at_radioconn;
  //int len = MIN(s->output_data_max_seg, uip_mss());
  int len = AT_RADIO_MAX_SEND_LEN; /* Not really MSS...*/

  if(s->output_senddata_len > 0) {
    len = MIN(s->output_senddata_len, len);
    len = s->output_senddata_len;
    s->output_data_send_nxt = len;
    at_radioconn = s->g_c;
    at_radioconn->output_data_ptr = s->output_data_ptr;
    at_radioconn->output_data_len = len;
    at_radio_send(at_radioconn);
  }
}
/*---------------------------------------------------------------------------*/
static void
acked(struct tcp_socket_at_radio *s)
{
  if(s->output_senddata_len > 0) {
    /* Copy the data in the outputbuf down and update outputbufptr and
       outputbuf_lastsent */

    if(s->output_data_send_nxt > 0) {
      memcpy(&s->output_data_ptr[0],
             &s->output_data_ptr[s->output_data_send_nxt],
             s->output_data_maxlen - s->output_data_send_nxt);
    }
    if(s->output_data_len < s->output_data_send_nxt) {
      PRINTF("tcp-gprs: acked assertion failed s->output_data_len (%d) < s->output_data_send_nxt (%d)\n",
             s->output_data_len,
             s->output_data_send_nxt);
      //tcp_markconn(uip_conn, NULL);
      //uip_abort();
      call_event(s, TCP_SOCKET_ABORTED);
      return;
    }
    s->output_data_len -= s->output_data_send_nxt;
    s->output_senddata_len = s->output_data_len;
    s->output_data_send_nxt = 0;

    call_event(s, TCP_SOCKET_DATA_SENT);
    /* Unlike tcp-socket.c, there is no polling from "below", so
     * activate sending of any remaining data from here.
     */
    if (s->output_senddata_len > 0) {
      senddata(s);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
at_radio_input_callback(struct at_radio_connection *at_radioconn, void *callback_arg,
                   const uint8_t *input_data_ptr, int input_data_len)
{
  struct tcp_socket_at_radio *s;
  uint16_t len, copylen, bytesleft;
  const uint8_t *dataptr;

  len = input_data_len;
  dataptr = input_data_ptr;

  /* We have a segment with data coming in. We copy as much data as
     possible into the input buffer and call the input callback
     function. The input callback returns the number of bytes that
     should be retained in the buffer, or zero if all data should be
     consumed. If there is data to be retained, the highest bytes of
     data are copied down into the input buffer. */
  s = callback_arg;
#if 0
  int i;
  printf("newdata %d @0x%x: '", len, (unsigned) dataptr);
  for (i = 0; i < len; i++)
    printf("%02x", (unsigned char ) dataptr[i], i);
  printf("'\n");
#endif
  do {
    copylen = MIN(len, s->input_data_maxlen);
    memcpy(s->input_data_ptr, dataptr, copylen);
    if(s->input_callback) {
      bytesleft = s->input_callback(s, s->ptr,
				    s->input_data_ptr, copylen);
    } else {
      bytesleft = 0;
    }
    if(bytesleft > 0) {
      printf("tcp: newdata, bytesleft > 0 (%d) not implemented\n", bytesleft);
    }
    dataptr += copylen;
    len -= copylen;

  } while(len > 0);

  return;
}
/*---------------------------------------------------------------------------*/
static char *tcp_event_str( tcp_socket_at_radio_event_t event) {
  switch (event) {
  case AT_RADIO_CONN_CONNECTED: return("CONNECTED");
  case AT_RADIO_CONN_SOCKET_CLOSED: return("CLOSED"); 
  case AT_RADIO_CONN_SOCKET_TIMEDOUT: return("TIMEDOUT"); 
  case AT_RADIO_CONN_ABORTED: return("ABORTED"); 
  case AT_RADIO_CONN_DATA_SENT: return("DATA_SENT"); 
  default: return("Unknown ");
  }
}

static void
at_radio_event_callback(void *callback_arg,
                        at_radio_conn_event_t event)
{
  struct tcp_socket_at_radio *s;
  s = (struct tcp_socket_at_radio *) callback_arg;
  switch (event) {
  case AT_RADIO_CONN_CONNECTED:
  case AT_RADIO_CONN_SOCKET_CLOSED:
  case AT_RADIO_CONN_SOCKET_TIMEDOUT:
  case AT_RADIO_CONN_ABORTED:
    if (s != NULL && s->event_callback != NULL) {
      s->event_callback(s, s->ptr, event);
    }
    else {
      printf("No callback for '%s'\n", tcp_event_str(event));
    }
    break;
  case AT_RADIO_CONN_DATA_SENT:
    acked(s);
  }
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  static uint8_t inited = 0;
  if(!inited) {
    list_init(socketlist_at_radio);
    //process_start(&tcp_socket_at_radio_process, NULL);
    inited = 1;
  }
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_at_radio_register(struct tcp_socket_at_radio *s, void *ptr,
		    uint8_t *input_databuf, int input_databuf_len,
		    uint8_t *output_databuf, int output_databuf_len,
		    tcp_socket_at_radio_data_callback_t input_callback,
		    tcp_socket_at_radio_event_callback_t event_callback)
{
  struct at_radio_connection *at_radioconn;

  init();

  at_radioconn = alloc_at_radio_connection();
  if (at_radioconn == NULL){
    return -1;
  }
  if(s == NULL) {
    return -1;
  }
  s->ptr = ptr;
  s->input_data_ptr = input_databuf;
  s->input_data_maxlen = input_databuf_len;
  s->output_data_len = 0;
  s->output_data_ptr = output_databuf;
  s->output_data_maxlen = output_databuf_len;
  s->output_senddata_len = 0;
  s->output_data_send_nxt = 0;

  s->input_callback = input_callback;
  s->event_callback = event_callback;

  s->listen_port = 0;
  s->flags = TCP_SOCKET_FLAGS_NONE;

  list_add(socketlist_at_radio, s);
  if (at_radio_register(at_radioconn, (void *) s,
                    at_radio_input_callback, at_radio_event_callback) < 0) {
    return -1;
  }
  else {
    s->g_c = at_radioconn;
    return 1;
  }
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_at_radio_connect_strhost(struct tcp_socket_at_radio *s,
                                const char *host,
                                uint16_t port)
{
  if(s == NULL) {
    return -1;
  }
#if 0
  if(s->c != NULL) {
    tcp_markconn(s->c, NULL);
  }
#endif

  //s->g_c = at_radio_connection("TCP", host, uip_htons(port), s);
  //at_radio_connection(s->g_c, "TCP", host, uip_htons(port));
  if(s->g_c == NULL) {
    return -1;
  } else {
    return 1;
  }
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_at_radio_connect(struct tcp_socket_at_radio *s,
                   const uip_ipaddr_t *ipaddr,
                   uint16_t port)
{
  return at_radio_connection(s->g_c, "TCP", ipaddr, uip_htons(port)) != NULL;
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_at_radio_listen(struct tcp_socket_at_radio *s,
           uint16_t port)
{
  if(s == NULL) {
    return -1;
  }

  s->listen_port = port;
  tcp_listen(uip_htons(port));
  s->flags |= TCP_SOCKET_FLAGS_LISTENING;
  return 1;
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_at_radio_unlisten(struct tcp_socket_at_radio *s)
{
  if(s == NULL) {
    return -1;
  }

  tcp_unlisten(uip_htons(s->listen_port));
  s->listen_port = 0;
  s->flags &= ~TCP_SOCKET_FLAGS_LISTENING;
  return 1;
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_at_radio_send(struct tcp_socket_at_radio *s,
                const uint8_t *data, int datalen)
{
  int len;
  
  if(s == NULL) {
    return -1;
  }

  len = MIN(datalen, s->output_data_maxlen - s->output_data_len);

  memcpy(&s->output_data_ptr[s->output_data_len], data, len);
  s->output_data_len += len;

  if(s->output_senddata_len == 0) {
    s->output_senddata_len = s->output_data_len;
    senddata(s);
  }

  //at_radio_send(s);
  //tcpip_poll_tcp(s->c);
  return len;
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_at_radio_send_str(struct tcp_socket_at_radio *s,
             const char *str)
{
  return tcp_socket_at_radio_send(s, (const uint8_t *)str, strlen(str));
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_at_radio_close(struct tcp_socket_at_radio *s)
{
  struct at_radio_connection *at_radioconn;
  if(s == NULL) {
    return -1;
  }

  at_radioconn = s->g_c;
  at_radio_close(at_radioconn);
  return 1;
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_at_radio_unregister(struct tcp_socket_at_radio *s)
{
  if(s == NULL) {
    return -1;
  }

  tcp_socket_at_radio_unlisten(s);

#if 0
  if(s->c != NULL) {
    tcp_attach(s->c, NULL);
  }
#endif
  if (s->g_c != NULL) {
    at_radio_unregister(s->g_c);
  }
  list_remove(socketlist_at_radio, s);
  return 1;
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_at_radio_max_sendlen(struct tcp_socket_at_radio *s)
{
  return s->output_data_maxlen - s->output_data_len;
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_at_radio_queuelen(struct tcp_socket_at_radio *s)
{
  return s->output_data_len;
}
/*---------------------------------------------------------------------------*/
