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
#include "gprs-a6-arch.h"
#include "i2c.h"
#include "dev/leds.h"
#include "dev/sc16is/sc16is.h"
#include "sc16is-common.h"
#include "at-radio.h"
#include "at-wait.h"

#ifdef HW_FLOW_NONE
#if HW_FLOW_NONE
#error "HW_FLOW_NONE cannot be set"
#endif
#endif /* HW_FLOW_NONE */

#define APN GPRS_CONF_APN
#define PDPTYPE "IP"
#define AT_RADIO_CONNID 0

struct at_radio_context at_radio_context;

static void
wait_init();
/*---------------------------------------------------------------------------*/
PROCESS(a6_reader, "A6 UART input process");
/* LED debugging process */
PROCESS(yled, "Yellow LED");
static clock_time_t yled_interval = CLOCK_SECOND/2;

/*---------------------------------------------------------------------------*/
void
at_radio_module_init() {
  at_radio_set_context(&at_radio_context, PDPTYPE, APN);
  wait_init();
  leds_init();
  process_start(&a6_reader, NULL);
  process_start(&yled, NULL);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(wait_ciprcv_callback(struct pt *pt, struct at_wait *at, int c));
static
PT_THREAD(wait_tcpclosed_callback(struct pt *pt, struct at_wait *at, int c));
static
PT_THREAD(wait_gpsrd_callback(struct pt *pt, struct at_wait *at, int c));

static
struct at_wait wait_ciprcv = {"+CIPRCV:", wait_ciprcv_callback};
static
struct at_wait wait_ok = {"OK", wait_readline_pt};
static
struct at_wait wait_error = {"ERROR", NULL};
static
struct at_wait wait_connectok = {"CONNECT OK", NULL};
static
struct at_wait wait_cmeerror = {"+CME ERROR:", wait_readline_pt};
static
struct at_wait wait_commandnoresponse = {"COMMAND NO RESPONSE!", NULL};
static
struct at_wait wait_sendprompt = {">", NULL};
static
struct at_wait wait_tcpclosed = {"+TCPCLOSED:", wait_tcpclosed_callback};
static
struct at_wait wait_gpsrd = {"$GPRMC,", wait_gpsrd_callback};

/*
 * CIPRCV:len,<data>
 */
static
PT_THREAD(wait_ciprcv_callback(struct pt *pt, struct at_wait *at, int c)) {
  static uint8_t rcvdata[AT_RADIO_MAX_RECV_LEN];
  static int rcvpos;
  static uint16_t nbytes;
  struct at_radio_connection *at_radioconn;
  
  PT_BEGIN(pt);

  /* Consume the colon ':' */
  PT_YIELD(pt); 
  /* Get length as a decimal number followed by a comma ','
   */
  nbytes = 0;
  while (c != ',') {
    if (!isdigit(c)) {
      /* Error: bad len */
      at_radio_statistics.at_errors += 1;
      restart_at(&wait_ciprcv); /* restart */
      PT_EXIT(pt);
    }
    nbytes = nbytes*10 + (c - '0');
    PT_YIELD(pt);
  }

  /* Consume the comma ',' in input */
  PT_YIELD(pt);

  if (nbytes > AT_RADIO_MAX_RECV_LEN)
    nbytes = AT_RADIO_MAX_RECV_LEN;
  rcvpos = 0;
  while (rcvpos < nbytes) {
    rcvdata[rcvpos++] = (uint8_t) c;
    PT_YIELD(pt);
  }

  restart_at(&wait_ciprcv); /* restart */
  at_radioconn = find_at_radio_connection(AT_RADIO_CONNID);
  if (at_radioconn) {
    at_radioconn->input_callback(at_radioconn, at_radioconn->callback_arg, rcvdata, nbytes);
  }
  PT_END(pt);
}

/* 
 * Callback for matching +TCPCLOSED keyword 
 */
static
PT_THREAD(wait_tcpclosed_callback(struct pt *pt, struct at_wait *at, int c)) {
  struct at_radio_connection *at_radioconn;
  static struct pt rlpt;

  PT_BEGIN(pt);
  atwait_record_on();
  PT_INIT(&rlpt);
  while (wait_readline_pt(&rlpt, at, c) < PT_EXITED) {
    PT_YIELD(pt);
  }
  atwait_record_off();

  restart_at(&wait_tcpclosed); /* restart */
  at_radioconn = find_at_radio_connection(AT_RADIO_CONNID);
  if (at_radioconn) {
    at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_CLOSED);
  }
  PT_END(pt);
}

static
PT_THREAD(wait_gpsrd_callback(struct pt *pt, struct at_wait *at, int c)) {

  char *valid;
  /* 
     unsigned int year, mon, day, hour, min, sec; 
     char  foo[6], c[1];
  */
  char *p, *time, *s1, *s2, *date;

  /* 
     A7 NMEA sentence
     PGSV,3,2,11,20,52,092,23,10,33,169,,08,23,291,,30,11,344,*7B*
     $GPGSV,3,3,11,07,17,318,,26,22,189,,21,55,086,*4B*
     $GPRMC,112547.000,A,5951.05612,N,01736.86381,E,0.00,0.00,300182,,,A*6D*
     $GPVTG,0.00,T,,
     M,0.00,N,0.00,K,A*3D*
  */

  static struct pt rlpt;

  PT_BEGIN(pt);
  atwait_record_on();
  PT_INIT(&rlpt);

  while (wait_readline_pt(&rlpt, at, c) < PT_EXITED) {
    PT_YIELD(pt);
  }
  atwait_record_off();

  printf("GPS=%s\n", at_line);

  const char *delim = " \t\r,";
  time = strtok((char *)&at_line[0], (const char *) delim);

  if(time) printf("time %s\n", time);
  
  valid = strtok(NULL, delim);
  if(valid) printf("valid %s\n", valid);
  
  p = strtok(NULL, delim);
  if(p) {
    status.lat = strtod(p, NULL);
    status.lat = status.lat/100.;
  }

  s1 = strtok(NULL, delim);
  if(p) printf("s1 %s\n", s1);

  p = strtok(NULL, delim);
  if(p) {
    status.longi = strtod(p, NULL);
    status.longi = status.longi/100.;
  }

  s2 = strtok(NULL, delim);
  if(p) printf("s2 %s\n", s2);

  p = strtok(NULL, delim);
  if(p) status.speed = strtod(p, NULL);
#define KNOT_TO_KMPH 1.852 
  status.speed = status.speed * KNOT_TO_KMPH;

  p = strtok(NULL, delim);
  if(p) status.course = strtod(p, NULL);

  date = strtok(NULL, delim);
  if(p) printf("date %s\n", date);

  if((!strcmp(valid, "A"))) {
    /* Fix */
    printf("valid=%s lon=%-lf lat=%-lf speed=%-6.2lf course=%-lf\n", 
	   valid, status.lat, status.longi, status.speed, status.course);
    /* year += 2000; */
  } 
  else {
    /* No fix */
    sc16is_gpio_set_bit(G_LED_RED);
    status.longi = -1;
    status.lat = -1;
    status.speed = -1;
    status.course = -1;
  }

#if 0
    /* Old. Decode $GPRMC sentense */
    res = sscanf(at_line,"%2d%2d%2d.%3c,%1c,%lf,%1c,%lf,%1c,%lf,%lf,%2d%2d%2d",
		 &hour, &min, &sec, foo, &valid, &lat, c, &longi, c, &speed, &course, &day, &mon, &year);
#endif    
      //nmew_time(year, mon, day, hour, min, sec );
    PT_END(pt);
}

static void
wait_init() {
  /* The following are to detect async events -- permanently active */
  atwait_start_atlist(1, &wait_ciprcv, &wait_tcpclosed, &wait_gpsrd, NULL);
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
    putchar(c);
}

PROCESS_THREAD(a6_reader, ev, data)
{
  static struct pt wait_pt;
  static int len;
  static uint8_t buf[200];

  PROCESS_BEGIN();

  PT_INIT(&wait_pt);
  
  while(1) {
    PROCESS_PAUSE();
    len = gprs_a6_rx(buf, sizeof(buf));
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
  return gprs_a6_tx(buf, len);
}
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
  PT_ATWAIT2(5, &wait_ok);
  atwait_record_off();

  if (at == NULL)
    at_radio_statistics.at_timeouts += 1;
  else {
    char *csq;
    csq = strstr(at_line, "+CSQ:") + strlen("+CSQ:");    
    status.rssi = atoi((char *) csq /*foundbuf*/);
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

  PT_ATSPAWN(gprs_a6_module_init, 115200);

  status.state = AT_RADIO_STATE_IDLE;

  PT_ATSTR2("ATI\r");
  PT_DELAY(5);
  status.state = AT_RADIO_STATE_IDLE;
  PT_DELAY(10);
  { static int i;
    for (i = 0; i < 10; i++) {
      PT_ATSTR2("ATI\r");
      PT_ATWAIT2(10, &wait_ok);
      if (at == &wait_ok)
        break;
    }
  }
      
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
      printf("scan creg fail %d\n", n);
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
  PT_ATWAIT2(20, &wait_ok, &wait_cmeerror);
  
  /* Then activate context */
  again:
  major_tries = 0;
  while (major_tries++ < 10 && status.state != AT_RADIO_STATE_ACTIVE) {
    static struct at_radio_context *gcontext;
    gcontext = &at_radio_context;
    /* Deactivate PDP context */
    sprintf(str, "AT+CSTT=\"%s\", \"\", \"\"\r", gcontext->apn); /* Start task and set APN */
    PT_ATSTR2(str);   
    PT_ATWAIT2(20, &wait_ok);
      
    PT_ATSTR2("AT+CGATT=1\r");
    PT_ATWAIT2(5, &wait_ok);
    PT_ATSTR2("AT+CIPMUX=0\r");
    PT_ATWAIT2(5, &wait_ok);
      
    minor_tries = 0;
    while (minor_tries++ < 10) {
      sprintf(str, "AT+CGDCONT=1,%s,%s\r", gcontext->pdptype, gcontext->apn); /* Set PDP (Packet Data Protocol) context */
      PT_ATSTR2(str);
      PT_ATWAIT2(5, &wait_ok);
      PT_ATSTR2("AT+CGACT=1,1\r");       /* Sometimes fails with +CME ERROR:148 -- seen when brought up initially */
      PT_ATWAIT2(20, &wait_ok,  &wait_cmeerror);
      if (at == &wait_ok) {
        break;
      }
      if (at == &wait_cmeerror) {
#ifdef AT_RADIO_DEBUG
        printf("CGACT failed with CME ERROR:%s\n", at_line);
#endif /* AT_RADIO_DEBUG */
        at_radio_statistics.at_errors += 1;
      }
      else {
        at_radio_statistics.at_timeouts += 1;
      }
      PT_DELAY(5);
    }

    if (minor_tries++ >= 10)
      continue;
    minor_tries = 0;
    while (minor_tries++ < 10) {
      PT_ATSTR2("AT+CIPSTATUS?\r"); 
      atwait_record_on();
      PT_ATWAIT2(60, &wait_ok);
      atwait_record_off();
      if (at == &wait_ok) {
        if (strstr((char *) at_line, "0,IP GPRSACT") != NULL) {
          gcontext->active = 1;
          status.state = AT_RADIO_STATE_ACTIVE;
          printf("ACTIVATED\n");
          break;
        }
        else {
          PT_DELAY(5);
        }
      }
    }
    if (minor_tries++ >= 10) {
      /* Failed to activate */
      PT_ATSTR2("AT+CIPSHUT\r");
      PT_ATWAIT2(10, &wait_ok);
      continue;
    }

  } /* Context activated */
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
    goto notfound;
  }
  int i;
  const char *delim = "\r\n";  

  char *p = strtok((char *)&at_line[0], (const char *) delim);
  if (p == NULL)
    goto notfound;

  for(i=0; i <  sizeof(at_line); i++) {
    p = strtok(NULL, delim);
    if (p == NULL) {
      goto notfound;
    }
    
    if(!strncmp(p, "A6", sizeof("A6")-1)) {
      status.module = AT_RADIO_MODULE_A6;

      /* 
         Workaround and fix and
         needs investigation. It seems like the A6 
         module is not compatible with UART sleep mode.
         Can be an A6 firmware issue.
      */
      sc16is_sleep_mode(0);
      break;
    }
    if(0 == strncmp(p, "A7", sizeof("A7"))) {
      status.module = AT_RADIO_MODULE_A7;
      break;
    }
  }
  if (status.module == AT_RADIO_MODULE_UNNKOWN)
    goto notfound;

#ifdef AT_RADIO_CONF_FORCE_A6
  status.module = AT_RADIO_MODULE_A6; /* To avoid GPS with A7 */
#endif
  printf("PT Module version %d\n", status.module);

  if(status.module == AT_RADIO_MODULE_A7) {
    PT_ATSTR2("AT+GPS=0\r");
    PT_ATWAIT2(10, &wait_ok);
    if (at == NULL) {
      printf("GPS not disabled\n");
    }
      
    PT_ATSTR2("AT+GPS=1\r");
    PT_ATWAIT2(10, &wait_ok);
    if (at == NULL) {
      printf("GPS not enabled\n");
    }
      
    PT_ATSTR2("AT+GPSRD=30\r");
    PT_ATWAIT2(10, &wait_gpsrd);
    if (at == NULL) {
      printf("GPSRD not enabled\n");
    }
  }

  
  PT_ATSTR2("AT+CNUM\r");
  PT_ATWAIT2(10, &wait_ok);
  PT_EXIT(pt);
  
 notfound:
  printf("module info not found ('%s')\n", at_line);
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
  /* Get IP address */
  PT_ATSTR2("AT+CIFSR\r");
  atwait_record_on();
  PT_ATWAIT2(10, &wait_ok);
  atwait_record_off();
  if (at == NULL) {
    at_radio_statistics.at_timeouts += 1;
  }
  else {
    /* Look for something like
     *     +CGCONTRDP: 1,5,"lpwa.telia.iot","10.81.168.254.255.255.255.0" 
     */
    int n;
    int b1, b2, b3, b4;
    char *str;
    //n = sscanf(at_line, "%*[^:]: %*d,%*d,\"%*[-.a-zA-Z0-9_]\",\"%d.%d.%d.%d", &b1, &b2, &b3, &b4);
    str = at_line;
    while (!isdigit(*str) && *str != '\0')
      str++;
    if (*str != '\0') {
      n = sscanf(str, "%d.%d.%d.%d", &b1, &b2, &b3, &b4);
      if (n == 4) {
#if NETSTACK_CONF_WITH_IPV6
        snprintf(status.ipaddr, sizeof(status.ipaddr), "::ffff:%d.%d.%d.%d", b1, b2, b3, b4);
#else
        snprintf(status.ipaddr, sizeof(status.ipaddr), "%d.%d.%d.%d", b1, b2, b3, b4);
#endif 
        PT_EXIT(pt);
      }
    }
  }
  printf("Could not get ipaddr from \"%s\"\n", at_line);
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
  yled_interval = CLOCK_SECOND*2;
  process_post(&yled, 3, &yled_interval);

  while (minor_tries++ < 10) {
    hip4 = (uint8_t *) &at_radioconn->ipaddr + sizeof(at_radioconn->ipaddr) - 4;
    sprintf(str, "AT+CIPSTART= \"%s\", %d.%d.%d.%d, %d\r",
            at_radioconn->proto, hip4[0], hip4[1], hip4[2], hip4[3], uip_ntohs(at_radioconn->port));    
    PT_ATSTR2(str);
    PT_ATWAIT2(60, &wait_connectok, &wait_cmeerror, &wait_commandnoresponse);
    if (at == &wait_connectok) {
      at_radio_statistics.connections += 1;
      at_radio_call_event(at_radioconn, AT_RADIO_CONN_CONNECTED);
      break;
    }
    else if (at == &wait_commandnoresponse) {
      /* Give it some more time. It happens that the connection succeeds seconds after COMMAND NO RESPONSE! */
      PT_ATWAIT2(15, &wait_connectok);
      if (at == &wait_connectok) {
        at_radio_statistics.connections += 1;
        at_radio_call_event(at_radioconn, AT_RADIO_CONN_CONNECTED);
        yled_interval = CLOCK_SECOND/2;
        process_post(&yled, 3, &yled_interval);
        break;
      }
    }

    /* If we ended up here, we failed to set up connection */
    if (at == NULL) {
      /* Timeout */
      at_radio_statistics.connfailed += 1;
      PT_ATSTR2("AT+CIPCLOSE\r");
      PT_ATWAIT2(5, &wait_ok);
      PT_ATSTR2("AT+CIPSHUT\r");
      PT_ATWAIT2(5, &wait_ok);
      status.state = AT_RADIO_STATE_NONE;
      at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_TIMEDOUT);
      break;
    }        
    else if (at == &wait_cmeerror) {
      /* COMMAND NO RESPONSE! timeout. Sometimes it take longer though and have seen COMMAND NON RESPONSE! followed by CONNECT OK */ 
      /* Seen +CME ERROR:53 */
      printf("CIPSTART failed\n");
      PT_ATSTR2("AT+CREG?\r");
      PT_ATWAIT2(2, &wait_ok);
      at_radio_statistics.at_errors += 1;
      PT_ATSTR2("AT+CIPCLOSE\r");
      PT_ATWAIT2(15, &wait_ok);//ATWAIT2(5, &wait_ok, &wait_cmeerror);
      PT_ATSTR2("AT+CIPSHUT\r");
      PT_ATWAIT2(15, &wait_ok);//ATWAIT2(5, &wait_ok,  &wait_cmeerror);

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

  ptr = at_radioconn->output_data_ptr;
  remain = at_radioconn->output_data_len;
#if 1
  PT_ATSTR2("ATE0\r\n");
  PT_ATWAIT2(5, &wait_ok);
  if (at == NULL)
    goto timeout;
#endif
  while (remain > 0) {
    char buf[20];
    len = (remain <= AT_RADIO_MAX_SEND_LEN ? remain : AT_RADIO_MAX_SEND_LEN);
    sprintf((char *) buf, "AT+CIPSEND=%d\r", len);
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
    PT_ATWAIT2(30, &wait_ok, &wait_commandnoresponse);
    if (at == NULL) {
      goto timeout;
    }        
    if (at == &wait_commandnoresponse) {
      goto disconnect;
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
  at_radio_call_event(at_radioconn, AT_RADIO_CONN_DATA_SENT);      
  PT_ATSTR2("ATE1\r\n");
  PT_ATWAIT2(5, &wait_ok);
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
  PT_BEGIN(pt);

  PT_ATSTR2("AT+CIPCLOSE\r");
  PT_ATWAIT2(15, &wait_ok, &wait_cmeerror);
  if (at == NULL) {
    at_radio_statistics.at_timeouts += 1;
  }
  at_radio_call_event(at_radioconn, AT_RADIO_CONN_SOCKET_CLOSED);
  PT_END(pt);
} 
/*---------------------------------------------------------------------------*/

static struct etimer yled_timer;

PROCESS_THREAD(yled, ev, data) 
{
  PROCESS_BEGIN();

  sc16is_gpio_set_bit(G_LED_YELLOW);

  while (1) { 
    PROCESS_WAIT_EVENT();
    if(etimer_expired(&yled_timer)) { 
      etimer_stop(&yled_timer); 
      sc16is_gpio_toggle_bit(G_LED_YELLOW);
      etimer_set(&yled_timer, yled_interval);
    }
    else if(ev == 1) {
      etimer_stop(&yled_timer); 
      sc16is_gpio_set_bit(G_LED_YELLOW);
    }
    else if(ev == 2) {
      etimer_stop(&yled_timer); 
      sc16is_gpio_clr_bit(G_LED_YELLOW);
    }
    else if(ev == 3) {
      etimer_stop(&yled_timer); 
      etimer_set(&yled_timer, yled_interval);
    }
  }
  PROCESS_END();
}
