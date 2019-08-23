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

#include "nbiot-sim7020-arch.h"

#define APN GPRS_CONF_APN
#define PDPTYPE "IP"
//#define PDPTYPE "IPV6"

#define RAWIP
struct at_radio_context at_radio_context;

static void
wait_init();

PROCESS(sim7020_reader, "Sim7020 UART input process");

/*---------------------------------------------------------------------------*/
void
at_radio_module_init() {

  at_radio_set_context(&at_radio_context, PDPTYPE, APN);
  nbiot_sim7020_init();
  wait_init();
  process_start(&sim7020_reader, NULL);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(wait_csonmi_callback(struct pt *pt, struct at_wait *at, int c));
static
PT_THREAD(wait_csoerr_callback(struct pt *pt, struct at_wait *at, int c));

static
struct at_wait wait_csonmi = {"+CSONMI:", wait_csonmi_callback};
static
struct at_wait wait_ok = {"OK", wait_readline_pt};
static
struct at_wait wait_connect = {"CONNECT", wait_readline_pt};
static
struct at_wait wait_error = {"ERROR", NULL};
static
struct at_wait wait_sendprompt = {">", NULL};
static
struct at_wait wait_csoerr = {"+CSOERR:", wait_csoerr_callback};
static
struct at_wait wait_csq = {"+CSQ:", wait_readline_pt};
static
struct at_wait wait_dataaccept = {"DATA ACCEPT: ", wait_readline_pt};

/*
 * +CSONMI: sock,len,<data>
 * hex: +CSONMI: 0,8,20020000 * 
 * bin: +CSONMI: 0,4,**** * 
 */
static
PT_THREAD(wait_csonmi_callback(struct pt *pt, struct at_wait *at, int c)) {
  static uint8_t rcvdata[AT_RADIO_MAX_RECV_LEN];
  static uint16_t rcvlen;
  static short int nbytes;
  static uint8_t csock;
  static int rcvpos;

  struct at_radio_connection *at_radioconn;
  
  PT_BEGIN(pt);
  atwait_record_on();
  while (c != ',') {
    PT_YIELD(pt);
  }
  PT_YIELD(pt);
  while (c != ',') {
    PT_YIELD(pt);
  }
  atwait_record_off();  
  PT_YIELD(pt);
  int nm;
  if ((nm = sscanf(at_line, " %hhd,%hd,", &csock, &nbytes)) != 2) {
    printf("csoerr scan error: %d", nm);
    goto done;
  }

#ifdef SIM7020_RECVHEX
  /* Data is encoded as hex string, so
   * data length is half the string length */ 
  rcvlen = nbytes >> 1;
  if (rcvlen > AT_RADIO_MAX_RECV_LEN)
    rcvlen = AT_RADIO_MAX_RECV_LEN;

  rcvpos = 0;
  /* Get one hex byte at a time */
  static char hexstr[3];
  while (1) {
    hexstr[0] = c;
    nbytes--;
    PT_YIELD(pt);
    hexstr[1] = c;
    nbytes--;
    hexstr[2] = 0;
    if (rcvpos < AT_RADIO_MAX_RECV_LEN) {
      rcvdata[rcvpos++] = (uint8_t) strtoul(hexstr, NULL, 16);
    }
    if (nbytes == 0)
      break;
    PT_YIELD(pt);
  }
#else
  /* Data is binary */
  rcvlen = nbytes;
  if (rcvlen > AT_RADIO_MAX_RECV_LEN)
    rcvlen = AT_RADIO_MAX_RECV_LEN;

  rcvpos = 0;
  /* Get one byte at a time, but don't store more than rcvlen */
  while (nbytes-- > 0) {
    if (rcvpos < rcvlen) {
      rcvdata[rcvpos++] = (uint8_t) c;
    }
    if (nbytes > 0)
      PT_YIELD(pt);
  }
#endif /* SIM7020_RECVHEX */

 done:
  restart_at(&wait_csonmi); /* restart */
  at_radioconn = find_at_radio_connection(csock);
  if (at_radioconn) {
    at_radioconn->input_callback(at_radioconn, at_radioconn->callback_arg, rcvdata, rcvlen);
  }
  PT_END(pt);
}


/* 
 * Callback for matching +CSOERR keyword 
 */
static
PT_THREAD(wait_csoerr_callback(struct pt *pt, struct at_wait *at, int c)) {
  struct at_radio_connection *at_radioconn;
  static struct pt rlpt;
  uint8_t csock, cerr;
  
  PT_BEGIN(pt);
  atwait_record_on();
  PT_INIT(&rlpt);
  while (wait_readline_pt(&rlpt, at, c) < PT_EXITED) {
    PT_YIELD(pt);
  }
  atwait_record_off();
  restart_at(&wait_csoerr); /* restart */
  if (2 != sscanf(at_line, "%hhd,%hhd", &csock, &cerr)) {
    printf("csoerr: csock fail\n");
    PT_EXIT(pt);
  }
  at_radioconn = find_at_radio_connection(csock);
  if (at_radioconn) {
    at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_CLOSED);
  }
  PT_END(pt);
}

static void
wait_init() {
  /* The following are to detect async events -- permanently active */
  atwait_start_atlist(1, &wait_csonmi, &wait_csoerr, NULL);
}

static void
dumpchar(int c) {
  static char atbol = 1; /* at beginning of line */
  
  if (atbol) {
    printf("    ");
    atbol = 0;
  }
  if (c == '\n') {
    putchar('\n');
    atbol = 1;
  }
  else if (c >= ' ' && c <= '~')
    putchar(c);
  else
    putchar('*');
}

 
PROCESS_THREAD(sim7020_reader, ev, data)
{
  static struct pt wait_pt;
  static int len;
  static uint8_t buf[200];

  PROCESS_BEGIN();

  PT_INIT(&wait_pt);
  
  while(1) {
    PROCESS_PAUSE();
    len = nbiot_sim7020_rx(buf, sizeof(buf));
    if (len) {
      static int i;
      uint8_t c;
      for (i = 0; i < len; i++) {
        c = buf[i];
	dumpchar(c);
	wait_fsm_pt(&wait_pt, c);
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/* at_radio_sendbuf
 * Send byte buffer to radio module
 * Return no of bytes sent
 */
size_t
at_radio_sendbuf(uint8_t *buf, size_t len) {
  return nbiot_sim7020_tx(buf, len);
}
/*---------------------------------------------------------------------------*/
#ifdef AT_PPP
void ppp_arch_putchar(uint8_t c) {
  while (1 != at_radio_sendbuf(&c, 1))
    ;
}

uint8_t ppp_arch_getchar(uint8_t *c) {
  return nbiot_sim7020_rx(c, 1);
}
#endif /* AT_PPP */
/*---------------------------------------------------------------------------*/
/* read_csq
 * Protothread to read rssi/csq with AT commands. Store result
 * in status struct
 */
PT_THREAD(read_csq(struct pt *pt)) {
  struct at_wait *at;

  PT_BEGIN(pt);
  PT_ATSTR2("AT+CSQ\r");
  atwait_record_on();
  PT_ATWAIT2(5, &wait_csq);
  atwait_record_off();

  if (at == NULL)
    at_radio_statistics.at_timeouts += 1;
  else {
    char *csq;
    csq = strstr(at_line, "+CSQ:") + strlen("+CSQ:");    
    status.rssi = atoi((char *) csq /*foundbuf*/);
    printf("Got CSQ: %d\n", status.rssi);
    PT_ATWAIT2(5, &wait_ok);
  }
  PT_END(pt);
}

/*---------------------------------------------------------------------------*/
/* init_module
 * Protothread to initialize mobile data radio unit.
 */
PT_THREAD(init_module(struct pt *pt)) {
  struct at_wait *at;

  PT_BEGIN(pt);

  PT_ATSTR2("AT+CRESET\r");
  PT_ATWAIT2(10, &wait_ok);
 again:
  PT_ATSTR2("AT+CPIN?\r");
  PT_ATWAIT2(10, &wait_ok);
  if (at == NULL)
  goto again;

  /* Disable power save mode for now */
  PT_ATSTR2("AT+CPSMS=0\r"); 
  PT_ATWAIT2(10, &wait_ok);

  PT_ATSTR2("AT+CSORCVFLAG?\r");
  PT_ATWAIT2(10, &wait_ok);

  /* Receive data in hex */
#ifdef SIM7020_RECVHEX
  /* Receive data as hex string */
  PT_ATSTR2("AT+CSORCVFLAG=0\r");
#else  
  /* Receive binary data */
  PT_ATSTR2("AT+CSORCVFLAG=1\r"); 
#endif /* SIM7020_RECVHEX */
  PT_ATWAIT2(10, &wait_ok);

  if (at == NULL) {
    status.state = AT_RADIO_STATE_NONE;
    at_radio_statistics.at_timeouts += 1;
  }
  else 
    status.state = AT_RADIO_STATE_IDLE;

  PT_ATSTR2("AT+CGCONTRDP\r");
  PT_ATWAIT2(10, &wait_ok);
  PT_ATSTR2("AT+CENG?\r");
  PT_ATWAIT2(10, &wait_ok);

  PT_DELAY(5);
  PT_END(pt);
}

/*---------------------------------------------------------------------------*/
/* apn_register
 * Protothread to register with APN. Wait until status confirms
 * we are registered, then update state.
 */
PT_THREAD(apn_register(struct pt *pt)) {
  struct at_wait *at;

  PT_BEGIN(pt);

  static uint8_t waiting;
  waiting = 0;
  while (waiting < AT_RADIO_APN_REGISTER_TIMEOUT) {
    static uint8_t creg;

    PT_ATSTR2("AT+CREG?\r");
    atwait_record_on();
    PT_ATWAIT2(10, &wait_ok);
    atwait_record_off();
    if (at == NULL)
      goto timeout;
    int n;
    /* AT+CREG?
     *  +CREG: 0,0*
     */
    if (1 != (n = sscanf(at_line, "%*[^:]: %*d,%hhd", &creg))) {
      at_radio_statistics.at_errors += 1;
      break;
    }
    if (creg == 1 || creg == 5 || creg == 10) {/* Wait for registration */
      status.state = AT_RADIO_STATE_REGISTERED;
      PT_EXIT(pt);
    }
    /* Registration failed. Delay and try again */
    PT_DELAY(AT_RADIO_APN_REGISTER_REATTEMPT);
    waiting += AT_RADIO_APN_REGISTER_REATTEMPT;
  }
  /* Timeout expired without registering */
  status.state = AT_RADIO_STATE_NONE;
  at_radio_statistics.resets += 1;
  PT_EXIT(pt);
 timeout:
  at_radio_statistics.at_timeouts += 1;
  status.state = AT_RADIO_STATE_NONE;
  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
/* apn_activate
 * Protothread to activate context. Wait until status confirms
 * we are activated, then update state.
 */
PT_THREAD(apn_activate(struct pt *pt)) {
  struct at_wait *at;
  static char str[80];
  static int major_tries, minor_tries;
  
  PT_BEGIN(pt);

  PT_ATSTR2("AT+CIMI?\r");   
  PT_ATWAIT2(20, &wait_ok, &wait_error);
  
  /* Then activate context */
  again:
  major_tries = 0;
  while (major_tries++ < 10 && status.state != AT_RADIO_STATE_ACTIVE) {
    static struct at_radio_context *gcontext;

    /* Attach */
    PT_ATSTR2("AT+CGATT\r");
    PT_ATWAIT2(10, &wait_ok, &wait_error);
    if (at == NULL)
      continue;

    PT_ATSTR2("AT+CSTT?\r");   
    PT_ATWAIT2(5, &wait_ok);

    gcontext = &at_radio_context;
    minor_tries = 0;
    while (minor_tries++ < 10) {
      //sprintf(str, "AT+CSGACT=1,1,%s\r", gcontext->apn); /* Start task and set APN */
      /* Start start, set APN */
      sprintf(str, "AT+CSTT=\"%s\",\"\",\"\"\r", gcontext->apn); /* Start task and set APN */
      PT_ATSTR2(str);   
      PT_ATWAIT2(10, &wait_ok, &wait_error);
      if (at == NULL)
        continue;
      PT_ATSTR2("AT+CIICR\r");
      PT_ATWAIT2(85, &wait_ok, &wait_error);
      if (at == NULL)
        continue;
      if (at == &wait_error)
        continue;
      gcontext->active = 1;
      status.state = AT_RADIO_STATE_ACTIVE;
      break;
    }
    if (minor_tries++ >= 10) {
      /* Failed to activate */
      PT_ATSTR2("AT+CIPSHUT\r");
      PT_ATWAIT2(10, &wait_ok);
      continue;
    }
  }
  /* Context activated */
  PT_DELAY(2);
  if (major_tries >= 10) {
    at_radio_statistics.resets += 1;
    goto again;
  }
  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
/*
 * get_moduleinfo
 *
 * Protothread to read module info using AT commands.
 * For now, identify module type.
 */
PT_THREAD(get_moduleinfo(struct pt *pt)) {
  struct at_wait *at;

  PT_BEGIN(pt);
  status.module = AT_RADIO_MODULE_UNNKOWN;
  PT_ATSTR2("ATI\r");
  atwait_record_on();
  PT_ATWAIT2(10, &wait_ok);
  atwait_record_off();
  if (at == NULL) {
    printf("No module version found\n");
  }
  else {
    char *ver;
    ver = strstr(at_line, "SIM7020E ");
    if (ver != NULL) {
      status.module = AT_RADIO_MODULE_SIM7020E;
    }
    else {
      printf("No module version in '%s'\n", at_line);
    }
  }
  PT_ATSTR2("AT+GSV\r");
  PT_ATWAIT2(10, &wait_ok);
  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
/*
 * get_ipconfig
 *
 * Protothread to get IP configuration using AT commands:
 * - Get own IP address
 */
PT_THREAD(get_ipconfig(struct pt *pt)) {
  struct at_wait *at;
  
  PT_BEGIN(pt);
  PT_ATSTR2("AT+CGCONTRDP\r");
  atwait_record_on();
  PT_ATWAIT2(10, &wait_ok);
  atwait_record_off();
  if (at == NULL) {
    at_radio_statistics.at_timeouts += 1;
    printf("No IP address found\n");
  }
  else {
    /* Look for something like
     *     +CGCONTRDP: 1,5,"lpwa.telia.iot","10.81.168.254.255.255.255.0" 
     */
    int n;
    int b1, b2, b3, b4;
    n = sscanf(at_line, "%*[^:]: %*d,%*d,\"%*[-.a-zA-Z0-9_]\",\"%d.%d.%d.%d", &b1, &b2, &b3, &b4);
    if (n == 4) {
      uint8_t *ip_p;
      memset(&status.uipaddr, 0, sizeof(status.uipaddr));
      ip_p = &((uint8_t *) &status.uipaddr)[sizeof(status.uipaddr) - 4];
      ip_p[0] = b1; ip_p[1] = b2; ip_p[2] = b3; ip_p[3] = b4;
#if NETSTACK_CONF_WITH_IPV6
      snprintf(status.ipaddr, sizeof(status.ipaddr), "::ffff:%d.%d.%d.%d", b1, b2, b3, b4);
#else
      snprintf(status.ipaddr, sizeof(status.ipaddr), "%d.%d.%d.%d", b1, b2, b3, b4);
#endif 

      uint8_t *ip4addr, *ip4mask;
      ip4addr = (uint8_t *) &status.ip4addr;       ip4mask = (uint8_t *) &status.ip4mask; 
      n = sscanf(at_line, "%*[^:]: %*d,%*d,\"%*[-.a-zA-Z0-9_]\",\"%hhd.%hhd.%hhd.%hhd.%hhd.%hhd.%hhd.%hhd",
                 &ip4addr[0], &ip4addr[1], &ip4addr[2], &ip4addr[3],
                 &ip4mask[0], &ip4mask[1], &ip4mask[2], &ip4mask[3]);
      printf("Scanned %d address byte\n", n);
    }
    else
      printf("Could not get ipaddr (%d)\n", n);
  }
  PT_END(pt);
}
/*---------------------------------------------------------------------------*/
/*
 * connect
 *
 * Protothread to set up TCP connection with AT commands
 */
PT_THREAD(at_radio_connect_pt(struct pt *pt, struct at_radio_connection * at_radioconn)) {
  static struct at_wait *at;
  static int minor_tries;
  char str[80];
  uint8_t *hip4;

  PT_BEGIN(pt);
  minor_tries = 0;
  while (minor_tries++ < 10) {
	
    hip4 = (uint8_t *) &at_radioconn->ipaddr + sizeof(at_radioconn->ipaddr) - 4;
    //#undef RAWIP
    PT_ATSTR2("AT+CSOC=1,1,1\r");
    atwait_record_on();
    PT_ATWAIT2(10, &wait_ok, &wait_error);
    atwait_record_off();
    uint8_t sockid;
    if (at != &wait_ok || 1 != sscanf(at_line, "%*[^:]: %hhd", &sockid)) {
      continue;
    }
    at_radioconn->connectionid = sockid;
    
    hip4 = (uint8_t *) &at_radioconn->ipaddr + sizeof(at_radioconn->ipaddr) - 4;
    //sprintf(str, "AT+CSOCON=%d,%d,\"%d.%d.%d.%d\"\r",
    //        at_radioconn->connectionid, uip_ntohs(at_radioconn->port), hip4[0], hip4[1], hip4[2], hip4[3]);
    sprintf(str, "AT+CSOCON=%d,%d,%d.%d.%d.%d\r",
            at_radioconn->connectionid, uip_ntohs(at_radioconn->port), hip4[0], hip4[1], hip4[2], hip4[3]);
    PT_ATSTR2(str);
    PT_ATWAIT2(60, &wait_ok, &wait_error);        
    if (at == &wait_ok) {
      at_radio_statistics.connections += 1;
      at_radio_call_event(at_radioconn, AT_RADIO_CONN_CONNECTED);
      break;
    }
    /* If we ended up here, we failed to set up connection */
    if (at == NULL) {
      /* Timeout */
      at_radio_statistics.connfailed += 1;
      status.state = AT_RADIO_STATE_NONE;
      sprintf(str, "AT+CSOCL=%d\r", at_radioconn->connectionid);
      PT_ATSTR2(str);
      PT_ATWAIT2(10, &wait_ok, &wait_error);        
      at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_TIMEDOUT);
      break;
    }        
    else if (at == &wait_error) {
      /* COMMAND NO RESPONSE! timeout. Sometimes it take longer though and have seen COMMAND NON RESPONSE! followed by CONNECT OK */ 
      /* Seen +CME ERROR:53 */
      PT_ATSTR2("AT+CREG?\r");
      PT_ATWAIT2(2, &wait_ok);
      at_radio_statistics.at_errors += 1;
      sprintf(str, "AT+CSOCL=%d\r", at_radioconn->connectionid);
      PT_ATSTR2(str);
      PT_ATWAIT2(15, &wait_ok);//ATWAIT2(5, &wait_ok, &wait_cmeerror);

      /* Test to cure deadlock when closing/shutting down  --ro */
      if (minor_tries++ > 10) {
        at_radio_statistics.connfailed += 1;
        break;
      }
      else {
        continue;
      }
    }
  } /* minor_tries */
  if (minor_tries >= 10) {
    at_radio_statistics.connfailed += 1;
    at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_TIMEDOUT);
    break;
  }
  PT_END(pt);
}

/*---------------------------------------------------------------------------*/
/*
 * send
 *
 * Protothread to send data with AT commands
 */
PT_THREAD(at_radio_send_pt(struct pt *pt, struct at_radio_connection * at_radioconn)) {
  static struct at_wait *at;
  static uint16_t remain;
  static uint16_t len;
  static uint8_t *ptr;

  PT_BEGIN(pt);
#ifdef AT_RADIO_DEBUG
  printf("A6AT AT_RADIO Send\n");
#endif /* AT_RADIO_DEBUG */

  ptr = at_radioconn->output_data_ptr;
  //socket = at_radioconn->socket;
  //remain = socket->output_data_len;
  remain = at_radioconn->output_data_len;
#if 0
  PT_ATSTR2("ATE0\r\n");
  PT_ATWAIT2(5, &wait_ok);
  if (at == NULL)
    goto timeout;
#endif
  while (remain > 0) {
    static char buf[40];

    len = (remain <= AT_RADIO_MAX_SEND_LEN ? remain : AT_RADIO_MAX_SEND_LEN);
    sprintf((char *) buf, "AT+CSODSEND=%d,%d\r", at_radioconn->connectionid, len);
    PT_ATSTR2((char *) buf); /* sometimes CME ERROR:516 */
    PT_ATWAIT2(5, &wait_ok, &wait_sendprompt, &wait_error);
    if (at == NULL) {
      PT_ATSTR2("ATE1\r\n");
      PT_ATWAIT2(5, &wait_ok);
      goto timeout;
    }
    else if (at == &wait_error) {
      goto disconnect;
    }
    PT_ATBUF2(&ptr[at_radioconn->output_data_len-remain], len);
    {
      int i;
      printf("\n");
      for (i = 0; i < len; i++)
        printf("%02x ", ptr[at_radioconn->output_data_len-remain+i]);
      printf("\n");
    }
    PT_ATWAIT2(30, &wait_ok, &wait_error, &wait_dataaccept);
    if (at == NULL || at == &wait_error) {
      at_radio_statistics.at_timeouts += 1;
      PT_EXIT(pt);
    }        
#if 0
    if (remain > len) {
      memcpy(&socket->output_data_ptr[0],
             &socket->output_data_ptr[len],
             remain - len);
    }
    socket->output_data_len -= len;
#endif
    remain -= len;
  }
  //call_event(socket, AT_RADIO_CONN_DATA_SENT);
  at_radio_call_event(at_radioconn, AT_RADIO_CONN_DATA_SENT);      
#if 0
  PT_ATSTR2("ATE1\r\n");
  PT_ATWAIT2(5, &wait_ok);
#endif
  PT_EXIT(pt);

 disconnect:
  at_radio_statistics.at_errors += 1;
  at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_CLOSED);
  PT_EXIT(pt);

 timeout:
  at_radio_statistics.at_timeouts += 1;
  at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_TIMEDOUT);
  status.state = AT_RADIO_STATE_NONE;

  PT_END(pt);
} 
/*---------------------------------------------------------------------------*/
/*
 * close
 *
 * Protothread to close TCP connection with AT commands
 */
PT_THREAD(at_radio_close_pt(struct pt *pt, struct at_radio_connection * at_radioconn)) {
  static struct at_wait *at;
  char str[20];
  PT_BEGIN(pt);

  snprintf(str, sizeof(str), "AT+CSOCL=%d\r", at_radioconn->connectionid);
  PT_ATSTR2(str);
  PT_ATWAIT2(15, &wait_ok, &wait_error);
  if (at == NULL) {
    at_radio_statistics.at_timeouts += 1;
  }
  at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_CLOSED);
  PT_END(pt);
} 
/*---------------------------------------------------------------------------*/
/*
 * datamode
 *
 * Protothread to put radio in data (transparent) mode 
 */

PT_THREAD(at_radio_datamode_pt(struct pt *pt, struct at_radio_connection * at_radioconn)) {

  static struct at_wait *at;
  PT_BEGIN(pt);
  
  while (1) {
    PT_ATSTR2("atdt*99#\r");
    PT_ATWAIT2(10, &wait_ok, &wait_error, &wait_connect);
    if (at == &wait_connect) {
      goto good;
    }
    else if (at == NULL) {
      printf("No response");
      goto good;
    }
    if (at == &wait_error) {
      PT_ATSTR2("AT+CEER=1\r");
      PT_ATWAIT2(10, &wait_ok, &wait_error);
    }
  }
 good:
  /* Disable uart receiver -- application accesses it directly */
  
  printf("Disable sim7020_reader\n");
  process_exit(&sim7020_reader);
  nbiot_sim7020_init(); /* Reset uart */
  PT_END(pt);
} 
/*---------------------------------------------------------------------------*/
