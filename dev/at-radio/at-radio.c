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
#include "uip.h"
#include "at-radio.h"
#include "at-wait.h"


#define AT_RADIO_MAX_CONNECTION 1
static struct at_radio_connection at_radio_connections[AT_RADIO_MAX_CONNECTION];
struct at_radio_context at_radio_context;

static void
event_init() {
  at_radio_ev_init = process_alloc_event();
  at_radio_ev_connection = process_alloc_event();
  at_radio_ev_send = process_alloc_event();
  at_radio_ev_close = process_alloc_event();  
}

#define AT_RADIO_MAX_NEVENTS 8
struct at_radio_event {
  process_event_t ev;
  void *data;
} at_radio_event_queue[AT_RADIO_MAX_NEVENTS];
static int at_radio_nevents;
static int at_radio_firstevent;

/*---------------------------------------------------------------------------*/
static void
event_queue_init() {
  at_radio_nevents = 0;
  at_radio_firstevent = 0;
}
/*---------------------------------------------------------------------------*/
#ifdef AT_RADIO_DEBUG
char *eventstr(process_event_t ev) {
  static char buf[64]; 

  if (ev == at_radio_ev_init) sprintf(buf, " at_radio_ev_init (%d)", ev); 
  else if (ev == at_radio_ev_connection) sprintf(buf, "at_radio_ev_connection (%d)", ev); 
  else if (ev == at_radio_ev_send) sprintf(buf, "at_radio_ev_send (%d)", ev); 
  else if (ev == at_radio_ev_close) sprintf(buf, "at_radio_ev_close (%d)", ev); 
  else if (ev == at_match_event) sprintf(buf, "at_match_event (%d)", ev); 

  else sprintf(buf, "unknown)(%d)", ev); 
  return buf;
}
#endif /* AT_RADIO_DEBUG */
/*---------------------------------------------------------------------------*/
void
enqueue_event(process_event_t ev, void *data) {
  int index;

#ifdef AT_RADIO_DEBUG
  printf("AT-RADIO: Enqueue event %s\n", eventstr(ev));
#endif /* AT_RADIO_DEBUG */
  if (at_radio_nevents >= AT_RADIO_MAX_NEVENTS) {
    printf("AT-RADIO: event queue full: %d\n", ev);
    return;
  }
  index = (at_radio_firstevent+at_radio_nevents) % AT_RADIO_MAX_NEVENTS;
  at_radio_event_queue[index].ev = ev; at_radio_event_queue[index].data = data;
  at_radio_nevents++;
}
/*---------------------------------------------------------------------------*/
struct at_radio_event *
dequeue_event() {
  struct at_radio_event *at_radio_event;
  if (at_radio_nevents == 0)
    return NULL;
  at_radio_event = &at_radio_event_queue[at_radio_firstevent];
  at_radio_nevents--;
  at_radio_firstevent = (at_radio_firstevent + 1) % AT_RADIO_MAX_NEVENTS;
#ifdef AT_RADIO_DEBUG
  printf("AT-RADIO: Dequeue event %s\n", eventstr(at_radio_event->ev));
#endif /* AT_RADIO_DEBUG */
  return at_radio_event;
}
/*---------------------------------------------------------------------------*/
PROCESS(at_radio, "AT-RADIO module");
/*---------------------------------------------------------------------------*/
void
at_radio_init() {
  int i;
  at_radio_module_init();

  for (i = 0; i < AT_RADIO_MAX_CONNECTION; i++) { 
    AT_RADIO_CONNECTION_RELEASE(&at_radio_connections[i]);
  }
  memset(&at_radio_statistics, 0, sizeof(at_radio_statistics));
  status.state = AT_RADIO_STATE_NONE;
  process_start(&at_radio, NULL);
}
/*---------------------------------------------------------------------------*/
struct at_radio_connection *
alloc_at_radio_connection() {
  int i;
  struct at_radio_connection *at_radioconn;
  for (i = 0; i < AT_RADIO_MAX_CONNECTION; i++) {
    at_radioconn = &at_radio_connections[i];
    if (!AT_RADIO_CONNECTION_RESERVED(at_radioconn)) {
        AT_RADIO_CONNECTION_RESERVE(at_radioconn);
        return at_radioconn;
    }
  }
  printf("AT-RADIO: Out of connections\n");
  return NULL;
}
/*---------------------------------------------------------------------------*/
static void
free_at_radio_connection(struct at_radio_connection *at_radioconn) {
  AT_RADIO_CONNECTION_RELEASE(at_radioconn);
  return;
}
/*---------------------------------------------------------------------------*/
struct at_radio_connection *
find_at_radio_connection(char connectionid) {
  int i;
  struct at_radio_connection *at_radioconn;

  for (i = 0; i < AT_RADIO_MAX_CONNECTION; i++) {
    at_radioconn = &at_radio_connections[i];
    if (at_radioconn->connectionid == connectionid) {
      return at_radioconn;
    }
  }
  printf("AT-RADIO: No such connection: %d\n", connectionid);
  return NULL;
}
/*---------------------------------------------------------------------------*/
#ifdef AT_RADIO_DEBUG
static char *atradio_event_str( at_radio_conn_event_t event) {
  switch (event) {
  case AT_RADIO_CONN_CONNECTED: return("AT_RADIO_CONN_CONNECTED");
  case AT_RADIO_CONN_SOCKET_CLOSED: return("AT_RADIO_CONN_SOCKET_CLOSED"); 
  case AT_RADIO_CONN_SOCKET_TIMEDOUT: return("AT_RADIO_CONN_SOCKET_TIMEDOUT"); 
  case AT_RADIO_CONN_ABORTED: return("AT_RADIO_CONN_ABORTED"); 
  case AT_RADIO_CONN_DATA_SENT: return("AT_RADIO_CONN_DATA_SENT"); 
  default: return("Unknown ");
  }
}
#endif /* AT_RADIO_DEBUG */
/*---------------------------------------------------------------------------*/
void
at_radio_call_event(struct at_radio_connection *at_radioconn, at_radio_conn_event_t event)
{
  if(at_radioconn != NULL && at_radioconn->event_callback != NULL) {
#ifdef AT_RADIO_DEBUG
    printf("AT-RADIO: callback %s\n", atradio_event_str(event));
#endif /* AT_RADIO_DEBUG */
    at_radioconn->event_callback(at_radioconn->callback_arg, event);
  }
#ifdef AT_RADIO_DEBUG
  else
    printf("No radio callback for event %d\n", event);
#endif /* AT_RADIO_DEBUG */
}
/*---------------------------------------------------------------------------*/
int
at_radio_set_context(struct at_radio_context *gcontext, char *pdptype, char *apn) {
    
  gcontext->active = 0;
  if (strcmp(pdptype, "IP") == 0 || strcmp(pdptype, "IPV6") == 0) {
    gcontext->pdptype = pdptype;
  }
  else
    return -1;
  strncpy(gcontext->apn, apn, AT_RADIO_MAX_APN_LEN);
  gcontext->apn[AT_RADIO_MAX_APN_LEN] = '\0';
  return 0;
}
/*---------------------------------------------------------------------------*/
int
at_radio_register(struct at_radio_connection *gconn,
              void *callback_arg,
              at_radio_data_callback_t input_callback,
              at_radio_event_callback_t event_callback) {

  gconn->callback_arg = callback_arg;
  gconn->input_callback = input_callback;
  gconn->event_callback = event_callback;  
  return 0;
}
/*---------------------------------------------------------------------------*/
int
at_radio_unregister(struct at_radio_connection *gconn) {

  free_at_radio_connection(gconn);
  return 0;
}
/*---------------------------------------------------------------------------*/
struct at_radio_connection *
at_radio_connection(struct at_radio_connection *at_radioconn,
                    const char *proto, const uip_ipaddr_t *ipaddr, uint16_t port) {

  if (at_radioconn == NULL) {
    return NULL;
  }
  at_radioconn->context = &at_radio_context;
  at_radioconn->proto = proto;
  memcpy(&at_radioconn->ipaddr, ipaddr, sizeof(uip_ipaddr_t));
  at_radioconn->port = port;
  enqueue_event(at_radio_ev_connection, at_radioconn);
  return at_radioconn;
}
/*---------------------------------------------------------------------------*/
void
at_radio_send(struct at_radio_connection *at_radioconn) {
  enqueue_event(at_radio_ev_send, at_radioconn);
}
/*---------------------------------------------------------------------------*/
void
at_radio_close(struct at_radio_connection *at_radioconn) {
  enqueue_event(at_radio_ev_close, at_radioconn);
}
/*---------------------------------------------------------------------------*/
void
at_radio_datamode(struct at_radio_connection *at_radioconn) {
  enqueue_event(at_radio_ev_datamode, at_radioconn);
}
/*---------------------------------------------------------------------------*/
struct at_radio_status *
at_radio_status() {
  return &status;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(at_radio, ev, data) {
  static struct at_radio_event *at_radio_event;
  static struct at_radio_connection *at_radioconn;

  PROCESS_BEGIN();
  event_init();
  event_queue_init();
  
  again:
  if (status.state ==  AT_RADIO_STATE_NONE) {
    ATSPAWN(init_module);
    goto again;
  }
  if (status.state == AT_RADIO_STATE_IDLE) {
    ATSPAWN(apn_register);
    goto again;
  }
  if (status.state == AT_RADIO_STATE_REGISTERED) {
    ATSPAWN(read_csq);
    ATSPAWN(apn_activate);
    ATSPAWN(get_moduleinfo);
    ATSPAWN(get_ipconfig);
    if (status.state != AT_RADIO_STATE_ACTIVE)
      goto again;
    process_post(PROCESS_BROADCAST, at_radio_ev_init, NULL);
    goto again;
  }
  if (status.state == AT_RADIO_STATE_ACTIVE) {
    at_radio_event = dequeue_event();
    if (at_radio_event == NULL) {
      PROCESS_PAUSE();
      goto again;
    }

    ATSPAWN(read_csq);

    at_radioconn = (struct at_radio_connection *) at_radio_event->data;
    if (at_radio_event->ev == at_radio_ev_connection) {
      ATSPAWN(at_radio_connect_pt, at_radioconn);
    } /* ev == at_radio_ev_connection */
    else if (at_radio_event->ev == at_radio_ev_send) {
      ATSPAWN(at_radio_send_pt, at_radioconn);
    } /* ev == at_radio_ev_send */
    else if (at_radio_event->ev == at_radio_ev_close) {
      ATSPAWN(at_radio_close_pt, at_radioconn);
    } /* ev == at_radio_ev_close */
    else if (at_radio_event->ev == at_radio_ev_datamode) {
      ATSPAWN(at_radio_datamode_pt, at_radioconn);
    }
#ifdef AT_RADIO_DEBUG
    else {
      printf("A6AT AT_RADIO Unknown event %d\n", at_radio_event->ev);
    }
#endif /* AT_RADIO_DEBUG */
  }
  goto again;
  PROCESS_END();
}
