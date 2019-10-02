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
#include "dev/watchdog.h"
#include "uip.h"
#include "at-radio.h"
#include "at-wait.h"

#include "nbiot-sim7020-arch.h"
#include "rss2.h"

#define APN GPRS_CONF_APN
#define PDPTYPE "IP"
//#define PDPTYPE "IPV6"

static void
wait_init();

/* Power save mode */
/* Activate power save mode on module */
#define POWER_SAVE_MODULE 
#ifdef POWER_SAVE_MODULE
uint8_t powersaving = 0;
#endif /* POWER_SAVE_MODULE */
struct at_radio_context at_radio_context;

#define MAXATTEMPTS 6

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
static PT_THREAD(wait_csonmi_callback(struct pt *pt, struct at_wait *at, int c));
static PT_THREAD(wait_csoerr_callback(struct pt *pt, struct at_wait *at, int c));
#ifdef POWER_SAVE_MODULE
static PT_THREAD(wait_cpsmstatus_callback(struct pt *pt, struct at_wait *at, int c));
#endif /* POWER_SAVE_MODULE */

static struct at_wait wait_csonmi = {"+CSONMI:", wait_csonmi_callback};
static struct at_wait wait_ok = {"OK", wait_readline_pt};
static struct at_wait wait_connect = {"CONNECT", wait_readline_pt};
static struct at_wait wait_error = {"ERROR", NULL};
static struct at_wait wait_sendprompt = {">", NULL};
static struct at_wait wait_csoerr = {"+CSOERR:", wait_csoerr_callback};
static struct at_wait wait_csq = {"+CSQ:", wait_readline_pt};
static struct at_wait wait_dataaccept = {"DATA ACCEPT: ", wait_readline_pt};
#ifdef POWER_SAVE_MODULE
static struct at_wait wait_cpsmstatus = {"+CPSMSTATUS: ", wait_cpsmstatus_callback};
#endif /* POWER_SAVE_MODULE */

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
    at_radio_statistics.at_errors++;
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
    at_radio_statistics.at_errors++;
    PT_EXIT(pt);
  }
  at_radioconn = find_at_radio_connection(csock);
  if (at_radioconn) {
    at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_CLOSED);
  }
  PT_END(pt);
}

#ifdef POWER_SAVE_MODULE
/* 
 * Callback for matching +CPSMSTATUS keyword 
 */
static
PT_THREAD(wait_cpsmstatus_callback(struct pt *pt, struct at_wait *at, int c)) {
  static struct pt rlpt;
  
  PT_BEGIN(pt);
  atwait_record_on();
  PT_INIT(&rlpt);
  while (wait_readline_pt(&rlpt, at, c) < PT_EXITED) {
    PT_YIELD(pt);
  }
  atwait_record_off();
  restart_at(&wait_cpsmstatus); /* restart */
  if (strstr(at_line, "EXIT PSM") != NULL) {
    powersaving = 0;
  }
  else if (strstr(at_line, "ENTER PSM") != NULL) {
    powersaving = 1;
  }
  else {
    printf("Unknown: %s\n", at_line);
  }
  PT_END(pt);
}
#endif /* POWER_SAVE_MODULE */

static void
wait_init() {
  /* The following are to detect async events -- permanently active */
#ifdef POWER_SAVE_MODULE
  atwait_start_atlist(1, &wait_csonmi, &wait_csoerr, &wait_cpsmstatus, NULL);
#else
  atwait_start_atlist(1, &wait_csonmi, &wait_csoerr, NULL);
#endif /* POWER_SAVE_MODULE */  
}

static void
printbol() {
  printf("%6lu: ", clock_seconds());
}

static void
dumpchar(int c) {
  static char atbol = 1; /* at beginning of line */
  
  if (atbol) {
    printbol();
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

/*
 * PWR_1 == PE7 Programmable power pin Vcc via P-FET
 */

void
toggle_pwrkey(uint16_t ms)
{
  uint16_t i;

  DDRE |= (1 << PWR_1);

  PORTE &= ~(1 << PWR_1);
  for(i = 0; i < ms; i++) {
    watchdog_periodic();
    clock_delay_usec(1000);
  }
  PORTE |= (1 << PWR_1);
}

void sim7020_power_on() {
  toggle_pwrkey(800);
}

void sim7020_power_off() {
  toggle_pwrkey(2000);
}

int
sim7020_rssi_to_dbm(int rssi)
{
  int lb = -48;
  /* Link budget approximation from RSSI */
  if(rssi >=0 && rssi <= 30)
    lb = -110 + 2 * rssi;
  else if(rssi == 99)   /* NO VALUE */
    lb = -999;
  return lb;
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
    status.rssi = atoi((char *) csq);
    PT_ATWAIT2(5, &wait_ok);
  }
  PT_END(pt);
}

/*---------------------------------------------------------------------------*/
/* module_restart
 * Force restart of module. Notify all active sockets and power off.
 */ 
static void module_restart() {
  status.state = AT_RADIO_STATE_NONE;
  at_radio_statistics.resets += 1;
  at_radio_reset();
  sim7020_power_off();
}
/*---------------------------------------------------------------------------*/
/* init_module
 * Protothread to initialize mobile data radio unit.
 */
PT_THREAD(init_module(struct pt *pt)) {
  struct at_wait *at;
  static unsigned long start_seconds = 0;

  PT_BEGIN(pt);
  start_seconds = clock_seconds();
  sim7020_power_on(); /* Start radio module */
  PT_ATSTR2("AT+CRESET\r");
  PT_ATWAIT2(10, &wait_ok);
  while  (clock_seconds() - start_seconds < AT_RADIO_INIT_TIMEOUT) {
    /* Module active? */
    PT_ATSTR2("AT\r");
    PT_ATWAIT2(10, &wait_ok);
    if (at == NULL) {
      at_radio_statistics.at_timeouts += 1;
      continue;
    }
    /* Disable power save mode for now */
    PT_ATSTR2("AT+CPSMS=0\r");
    PT_ATWAIT2(10, &wait_ok);
#ifdef POWER_SAVE_MODULE
    powersaving = 0;
#endif /* POWER_SAVE_MODULE */  

    /* Limit bands to speed up roaming */
    /* WIP needs a generic solution */
    PT_ATSTR2("AT+CBAND=20\r");
    PT_ATWAIT2(10, &wait_ok);

    PT_ATSTR2("AT+CSORCVFLAG?\r");
    PT_ATWAIT2(20, &wait_ok);

#ifdef SIM7020_RECVHEX
    /* Receive data as hex string */
    PT_ATSTR2("AT+CSORCVFLAG=0\r");
#else  
    /* Receive binary data */
    PT_ATSTR2("AT+CSORCVFLAG=1\r"); 
#endif /* SIM7020_RECVHEX */
    PT_ATWAIT2(10, &wait_ok);
    if (at == NULL) {
      at_radio_statistics.at_timeouts += 1;
      continue;
    }
    /* Module initialization done */
    status.state = AT_RADIO_STATE_IDLE;
    PT_EXIT(pt);
  }
  /* End up here if it took too long */
  module_restart();
  PT_END(pt);
}

/*---------------------------------------------------------------------------*/
/* apn_register
 * Protothread to register with APN. Wait until status confirms
 * we are registered, then update state.
 */
PT_THREAD(apn_register(struct pt *pt)) {
  struct at_wait *at;
  static unsigned long start_seconds;

  PT_BEGIN(pt);
  start_seconds = clock_seconds();
  while (clock_seconds() - start_seconds < AT_RADIO_APN_REGISTER_TIMEOUT) {
    static uint8_t creg;

    PT_ATSTR2("AT+CREG?\r");
    atwait_record_on();
    PT_ATWAIT2(10, &wait_ok);
    atwait_record_off();
    if (at == NULL) {
      at_radio_statistics.at_timeouts += 1;
      continue;
    }
    int n;
    /* AT+CREG?
     *  +CREG: 0,0*
     */
    if (1 != (n = sscanf(at_line, "%*[^:]: %*d,%hhd", &creg))) {
      at_radio_statistics.at_errors += 1;
      continue;
    }
    if (creg == 1 || creg == 5 || creg == 10) {/* Wait for registration */
      status.state = AT_RADIO_STATE_REGISTERED;
      PT_EXIT(pt);
    }
    /* Registration failed. Delay and try again */
    PT_DELAY(AT_RADIO_APN_REGISTER_REATTEMPT);
  }
  /* End up here if it took too long */
  module_restart();
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
  static struct at_radio_context *gcontext;
  static unsigned long start_seconds;

  PT_BEGIN(pt);
  start_seconds = clock_seconds();
  gcontext = &at_radio_context;

  while (clock_seconds() - start_seconds < AT_RADIO_APN_ATTACH_TIMEOUT) {
    /* Attach */
    sprintf(str, "AT+CSTT=\"%s\",\"\",\"\"\r", gcontext->apn); /* Start task and set APN */
    PT_ATSTR2(str);   
    PT_ATWAIT2(10, &wait_ok, &wait_error);
    if (at == &wait_ok)
      break;
    else if (at == NULL)
      at_radio_statistics.at_timeouts++;
    /* Registration failed. Delay and try again */
    PT_DELAY(AT_RADIO_APN_ATTACH_REATTEMPT);
  }

  /* Bring up wireless */
  while ( clock_seconds() - start_seconds < AT_RADIO_APN_ATTACH_TIMEOUT) {
    PT_ATSTR2("AT+CIICR\r");
    PT_ATWAIT2(600, &wait_ok, &wait_error);
    if (at == &wait_ok) {
      /* Enable power save mode */
      PT_ATSTR2("AT+CPSMS=1\r");
      PT_ATWAIT2(120, &wait_ok, &wait_error);
      /* Recover socket config when module exits PSM mode */
      PT_ATSTR2("AT+RETENTION=1\r");
      PT_ATWAIT2(10, &wait_ok);
      PT_ATSTR2("AT+CPSMS?\r");
      PT_ATWAIT2(120, &wait_ok, &wait_error);

      gcontext->active = 1;
      status.state = AT_RADIO_STATE_ACTIVE;
      PT_EXIT(pt);
    }
    PT_DELAY(AT_RADIO_APN_ATTACH_REATTEMPT);
  }
  /* End up here if too many errors, or it took too long time */
  PT_ATSTR2("AT+CIPSHUT\r");
  PT_ATWAIT2(10, &wait_ok);
  module_restart();
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
    printf("No IP address\n");
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
    }
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
  char str[80];
  uint8_t *hip4;
  static unsigned long start_seconds;
  static uint8_t attempts;

  PT_BEGIN(pt);
  start_seconds = clock_seconds();
  attempts = 0;
  while (attempts++ < MAXATTEMPTS && clock_seconds() - start_seconds < AT_RADIO_CONNECT_TIMEOUT) {
	
#ifdef POWER_SAVE_MODULE
    /* If in PSM, wakeup.
     * Also, if no response, assume module is in PSM and try wakeup 
     */ 
    if (powersaving || attempts > 2) {
      sim7020_power_on();
    }
    /* Probe module */
    PT_ATSTR2("AT\r");
    PT_ATWAIT2(10, &wait_ok);
    if (at == &wait_ok) {
      powersaving = 0;
    }
    else {
      continue;
    }

#ifndef SIM7020_RECVHEX
    {
      /* RCVFLAG gets reset after PSM, so set it again. */
      PT_ATSTR2("AT+CSORCVFLAG=1\r");
      PT_ATWAIT2(20, &wait_ok);
      if (at == NULL) {
        /* No response, make another attempt */
        continue;
      }
    }
#endif /* not SIM7020_RECVHEX */
#endif /* POWER_SAVE_MODULE */  

    hip4 = (uint8_t *) &at_radioconn->ipaddr + sizeof(at_radioconn->ipaddr) - 4;
    PT_ATSTR2("AT+CSOC=1,1,1\r"); /* IPv4, TCP, IP */
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
      PT_EXIT(pt);
    }
    /* If we end up here, we failed to set up connection */
    at_radio_statistics.connfailed += 1;
    sprintf(str, "AT+CSOCL=%d\r", at_radioconn->connectionid);
    PT_ATSTR2(str);
    PT_ATWAIT2(10, &wait_ok, &wait_error);        
    PT_DELAY(AT_RADIO_CONNECT_REATTEMPT);
  } /* minor_tries */
  /* End up here if too many errors, or it took too long time */
#ifdef AT_RADIO_DEBUG
  printf("%d: connect reset: %lu (%d attempts)\n", __LINE__, clock_seconds() - start_seconds, attempts);
#endif /* AT_RADIO_DEBUG */
  module_restart();
  at_radioconn->connectionid = AT_RADIO_CONNECTIONID_NONE;
  at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_TIMEDOUT);
  break;
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
  static uint8_t attempts;
  
  PT_BEGIN(pt);
#ifdef AT_RADIO_DEBUG
  printf("A6AT AT_RADIO Send @%lu sec\n", clock_seconds());
#endif /* AT_RADIO_DEBUG */

#ifdef POWER_SAVE_MODULE
  attempts = 0;
  while (attempts++ < MAXATTEMPTS) {

    /* If in PSM, wakeup.
     * Also, if probed module but no response, assume module is
     * in PSM and try wakeup 
     */ 
    if (powersaving || attempts > 2) {
      sim7020_power_on();
    }
    /* Probe module */
    PT_ATSTR2("AT\r");
    PT_ATWAIT2(30, &wait_ok);
    if (at == NULL) {
      at_radio_statistics.at_timeouts++;
    }
    else {
      powersaving = 0;
      break;
    }
  }
  if (attempts > MAXATTEMPTS) {
    module_restart();
    PT_EXIT(pt);
  }
#endif /* POWER_SAVE_MODULE */  

  ptr = at_radioconn->output_data_ptr;
  remain = at_radioconn->output_data_len;

  while (remain > 0) {
    static char buf[40];

    len = (remain <= AT_RADIO_MAX_SEND_LEN ? remain : AT_RADIO_MAX_SEND_LEN);
    sprintf((char *) buf, "AT+CSODSEND=%d,%d\r", at_radioconn->connectionid, len);
    PT_ATSTR2((char *) buf); 
    PT_ATWAIT2(5, &wait_ok, &wait_sendprompt, &wait_error);
    if (at != &wait_sendprompt) {
      goto disconnect;
    }
#ifdef AT_RADIO_DEBUG
    {
      int i;
      printf("\n");
      for (i = 0; i < len; i++)
        printf("%02x ", ptr[at_radioconn->output_data_len-remain+i]);
      printf("\n");
    }
#endif /* AT_RADIO_DEBUG */
    PT_ATBUF2(&ptr[at_radioconn->output_data_len-remain], len);
    PT_ATWAIT2(30, &wait_ok, &wait_error, &wait_dataaccept);
    /* Should goto disconnect here ? */
    if (at == NULL || at == &wait_error) {
      goto disconnect;
#if 0
      if (remain > len) {
        memcpy(&socket->output_data_ptr[0],
               &socket->output_data_ptr[len],
               remain - len);
      }
      socket->output_data_len -= len;
#endif
    }
    remain -= len;
  }
  at_radio_call_event(at_radioconn, AT_RADIO_CONN_DATA_SENT);      
#if 0
  PT_ATSTR2("ATE1\r");
  PT_ATWAIT2(5, &wait_ok);
#endif
  PT_EXIT(pt);

 disconnect:
  if (at == NULL)
    at_radio_statistics.at_timeouts += 1;
  else
    at_radio_statistics.at_errors += 1;
  {
    char str[32];
    sprintf(str, "AT+CSOCL=%d\r", at_radioconn->connectionid);
    PT_ATSTR2(str);
  }
  PT_ATWAIT2(10, &wait_ok, &wait_error);        

  at_radioconn->connectionid = AT_RADIO_CONNECTIONID_NONE;
  at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_CLOSED);
  PT_EXIT(pt);

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

  if (0 == verify_at_radio_connection(at_radioconn)) {
    printf("close: invalid radioconn @0x%x\n", (unsigned) at_radioconn);
    PT_EXIT(pt);
  }

  if (at_radioconn->connectionid != AT_RADIO_CONNECTIONID_NONE) {
    snprintf(str, sizeof(str), "AT+CSOCL=%d\r", at_radioconn->connectionid);
    PT_ATSTR2(str);
    PT_ATWAIT2(15, &wait_ok, &wait_error);
    if (at == NULL) {
      at_radio_statistics.at_timeouts += 1;
    }
    at_radioconn->connectionid = AT_RADIO_CONNECTIONID_NONE;
    //at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_CLOSED);
  }
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
