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
#include "i2c.h"
#include "dev/leds.h"
#include "dev/sc16is/sc16is.h"
#include "sc16is-common.h"
#include "tcp-socket-gprs.h"
#include "gprs-a6.h"

#define APN GPRS_CONF_APN
#define PDPTYPE "IP"
//#define PDPTYPE "IPV6"

uint32_t baud;
uint8_t at[] = {'A', 'T', 0xd };

#define GPRS_MAX_CONNECTION 1
struct gprs_connection gprs_connections[GPRS_MAX_CONNECTION];
struct gprs_context gprs_context;

process_event_t sc16is_input_event;
process_event_t at_match_event;

static struct gprs_status status;

#define min(A, B) ((A) <= (B) ? (A) : (B))
/*---------------------------------------------------------------------------*/
PROCESS(sc16is_reader, "I2C UART input process");
PROCESS(a6at, "GPRS A6 module");


/* LED debugging process */
PROCESS(yled, "Yellow LED");
static clock_time_t yled_interval = CLOCK_SECOND/2;

/*---------------------------------------------------------------------------*/
void
gprs_init() {
  gprs_set_context(&gprs_context, PDPTYPE, APN);
  memset(&gprs_statistics, 0, sizeof(gprs_statistics));
  process_start(&sc16is_reader, NULL);
  process_start(&a6at, NULL);
  process_start(&yled, NULL);
}
/*---------------------------------------------------------------------------*/
struct gprs_connection *
alloc_gprs_connection() {
  return &gprs_connections[GPRS_MAX_CONNECTION-1];
}
/*---------------------------------------------------------------------------*/
static void
free_gprs_connection(struct gprs_connection *gprsconn) {
  return;
}
/*---------------------------------------------------------------------------*/
static struct gprs_connection *
find_gprs_connection() {
  return &gprs_connections[GPRS_MAX_CONNECTION-1];
}
/*---------------------------------------------------------------------------*/
static void
call_event(struct tcp_socket_gprs *s, tcp_socket_gprs_event_t event)
{
  if(s != NULL && s->event_callback != NULL) {
    s->event_callback(s, s->ptr, event);
  }
}
/*---------------------------------------------------------------------------*/

static void
gprs_call_event(struct gprs_connection *gprsconn, gprs_conn_event_t event)
{
  if(gprsconn != NULL && gprsconn->event_callback != NULL) {
    gprsconn->event_callback(gprsconn, gprsconn->callback_arg, event);
  }
}
/*---------------------------------------------------------------------------*/
int
gprs_set_context(struct gprs_context *gcontext, char *pdptype, char *apn) {
    
  gcontext->active = 0;
  if (strcmp(pdptype, "IP") == 0 || strcmp(pdptype, "IPV6") == 0) {
    gcontext->pdptype = pdptype;
  }
  else
    return -1;
  strncpy(gcontext->apn, apn, GPRS_MAX_APN_LEN);
  gcontext->apn[GPRS_MAX_APN_LEN] = '\0';
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Do we need this */
int
gprs_register(struct gprs_connection *gconn,
              void *callback_arg,
              void *callback,
              gprs_data_callback_t input_callback,
              gprs_event_callback_t event_callback) {

  gconn->socket = callback_arg;
  gconn->callback = callback;
  gconn->callback_arg = callback_arg;
  gconn->input_callback = input_callback;
  gconn->event_callback = event_callback;  
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Do we need this */
int
gprs_unregister(struct gprs_connection *gconn) {

  gconn->socket = NULL;
  gconn->callback_arg = NULL;
  return 0;
}
/*---------------------------------------------------------------------------*/
struct gprs_connection *
gprs_connection(struct gprs_connection *gprsconn,
                const char *proto, const char *ipaddr, uint16_t port,
                struct tcp_socket_gprs *socket) {

  if (gprsconn == NULL) {
    return NULL;
  }
  gprsconn->context = &gprs_context;
  gprsconn->proto = proto;
  gprsconn->socket = socket;
  gprsconn->ipaddr = ipaddr;
  gprsconn->port = port;
  if (process_post(&a6at, a6at_gprs_connection, gprsconn) != PROCESS_ERR_OK) {
    free_gprs_connection(gprsconn);
    return NULL;
  }
  else {
    return gprsconn;
  }
}
/*---------------------------------------------------------------------------*/
void
//gprs_send(struct tcp_socket_gprs *socket) {
gprs_send(struct gprs_connection *gprsconn) {
  (void) process_post(&a6at, a6at_gprs_send, gprsconn);
}
/*---------------------------------------------------------------------------*/
void
gprs_close(struct tcp_socket_gprs *socket) {
  (void) process_post(&a6at, a6at_gprs_close, socket->g_c);
}
/*---------------------------------------------------------------------------*/
struct gprs_status *
gprs_status() {
  return &status;
}
/*---------------------------------------------------------------------------*/

struct at_wait; /* forward declaration */
typedef char (* at_callback_t)(struct pt *, struct at_wait *, uint8_t
*data, int len, int *consumed);
struct at_wait {
  char *str;
  at_callback_t callback;
  int (* match)(struct at_wait *, uint8_t *, int);
  uint8_t pos;
};
 
static
PT_THREAD(wait_ciprcv_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed));
static
PT_THREAD(wait_simple_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed));
static
PT_THREAD(wait_readline_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed));
static
PT_THREAD(wait_readlines_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed));
static
PT_THREAD(wait_tcpclosed_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed));
static
PT_THREAD(wait_dotquad_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed));
static
PT_THREAD(wait_gpsrd_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed));

static int at_match_byte(struct at_wait *at, uint8_t *buf, int len);
static int at_match_dotquad(struct at_wait *at, uint8_t *buf, int len);

struct at_wait wait_ciprcv = {"+CIPRCV:", wait_ciprcv_callback, at_match_byte};
struct at_wait wait_ok = {"OK", wait_simple_callback, at_match_byte};
struct at_wait wait_cipstatus = {"+CIPSTATUS:", wait_readlines_callback, at_match_byte};
struct at_wait wait_creg = {"+CREG: ", wait_readline_callback, at_match_byte};
struct at_wait wait_connectok = {"CONNECT OK", wait_simple_callback, at_match_byte};
struct at_wait wait_cmeerror = {"+CME ERROR:", wait_readline_callback, at_match_byte};
struct at_wait wait_commandnoresponse = {"COMMAND NO RESPONSE!", wait_simple_callback, at_match_byte};
struct at_wait wait_sendprompt = {">", wait_simple_callback, at_match_byte};
struct at_wait wait_tcpclosed = {"+TCPCLOSED:", wait_tcpclosed_callback, at_match_byte};
struct at_wait wait_dotquad = {"..." /* not used */, wait_dotquad_callback, at_match_dotquad};
struct at_wait wait_csq = {"+CSQ:", wait_readline_callback, at_match_byte};
struct at_wait wait_gpsrd = {"$GPRMC,", wait_gpsrd_callback, at_match_byte};

struct at_wait wait_ati = {"Ai Thinker", wait_readlines_callback, at_match_byte};

static char atline[80];

static void
start_at(struct at_wait *at); /* forward declaration */

/*
 * Simple match -- matched input string (like "OK"). Post 
 * an event to signal the match.
 */
static
PT_THREAD(wait_simple_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed)) {
  PT_BEGIN(pt);
  process_post(&a6at, at_match_event, at);
  *consumed = 0;
  PT_END(pt);
}

/*
 * CIPRCV:len,<data>
 */
static
PT_THREAD(wait_ciprcv_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed)) {
  static uint8_t rcvdata[GPRS_MAX_RECV_LEN];
  static uint16_t rcvlen;
  static uint16_t nbytes;
  static int rcvpos;
  static int datapos = 0;
  struct gprs_connection *gprsconn;
  
  PT_BEGIN(pt);
  datapos = 0;
  nbytes = 0;
  /* Get length as a decimal number followed by a comma ','
   */
  while (datapos < len && data[datapos] != ',') {
    if (!isdigit(data[datapos]) || nbytes > GPRS_MAX_RECV_LEN) {
      /* Error: bad len */
      printf("ciprcv_callback: bad len\n");
      start_at(&wait_ciprcv); /* restart */
      *consumed = datapos;
      PT_EXIT(pt);
    }

    nbytes = nbytes*10 + (data[datapos++] - '0');
    if (datapos == len) {
      /* Out of data -- yield and wait for more */
      PT_YIELD(pt);
      datapos = 0;
    }
  }

  /* Consume the comma ',' in input */
  if (nbytes > 0 && ++datapos == len) {
    /* Out of data -- yield and wait for more */
    PT_YIELD(pt);
    datapos = 0;
  }
  if (nbytes > GPRS_MAX_RECV_LEN)
    nbytes = GPRS_MAX_RECV_LEN;
  rcvlen = nbytes;
  rcvpos = 0;
  while (nbytes > 0) {
    int ncopy = min(nbytes, len-datapos);
    memcpy(&rcvdata[rcvpos], &data[datapos], ncopy);
    nbytes -= ncopy;
    rcvpos += ncopy;
    datapos += ncopy;
    if (nbytes > 0) {
      /* Out of data -- yield and wait for more */
      PT_YIELD(pt);
      datapos = 0;
    }
  }

  *consumed = datapos;
  start_at(&wait_ciprcv); /* restart */
  gprsconn = find_gprs_connection();
  if (gprsconn) {
    gprsconn->input_callback(gprsconn, gprsconn->callback_arg, rcvdata, rcvlen);
  }
  PT_END(pt);
}

PT_THREAD(wait_readline_pt(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed)) {
  static int atpos;
  int done = 0;
  static int datapos;

  PT_BEGIN(pt);
  datapos = 0;
  atpos = 0; 
  while (done == 0) {
    while (datapos < len && done == 0) {
      if (data[datapos] == '\n' || data[datapos] == '\r') {
        done = 1;
      }
      else {
        if (atpos < sizeof(atline)-1)
          atline[atpos++] = (char ) data[datapos++];
        else
          datapos++;
      }
    }
    if (!done) {
      /* Reached end of input data but have not seen end of line. 
       * Yield here and wait for next invocation.
       */ 
      PT_YIELD(pt);
    }
  }
  /* done -- mark end of string */
  atline[atpos] = '\0';
  *consumed = datapos; /* How many bytes we consumed */
  PT_END(pt);
}

PT_THREAD(wait_readlines_pt(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed)) {
  static int atpos;
  int done = 0;
  static int datapos;

  PT_BEGIN(pt);
  datapos = 0;
  atpos = 0;
  while (done == 0) {
    while (datapos < len && done == 0) {
      if (data[datapos] == '\n' || data[datapos] == '\r') {
        data[datapos] = ' ';
      }
      else {
        atline[atpos++] = (char ) data[datapos++];
        if (atpos == sizeof(atline)-1)
          done = 1;
	if (data[datapos] == 0)
          done = 1;
      }
    }
    if (!done) {
      /* Reached end of input data but have not seen end of line. 
       * Yield here and wait for next invocation.
       */ 
      PT_YIELD(pt);
    }
  }
  /* done -- mark end of string */
  atline[atpos] = '\0';
  *consumed = datapos; /* How many bytes we consumed */
  PT_END(pt);
}

static
PT_THREAD(wait_tcpclosed_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed)) {
  char ret;
  struct gprs_connection *gprsconn;
  ret = wait_readline_pt(pt, at, data, len, consumed);
  if (ret == PT_ENDED) {
    start_at(&wait_tcpclosed); /* restart */
    gprsconn = find_gprs_connection();
    if (gprsconn && gprsconn->socket) {
      call_event(gprsconn->socket, TCP_SOCKET_CLOSED);
    }
  }
  return ret;
}

/*
 * Callback function to read the remainder of the line until
 * EOL, and the post an event to signal the match
 */
static
PT_THREAD(wait_readline_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed)) {
  char ret;
  ret = wait_readline_pt(pt, at, data, len, consumed);
  if (ret == PT_ENDED)
    process_post(&a6at, at_match_event, at);
  return ret;
}

/*
 * Callback function to read all lines until full buffer is full
 * and the post an event to signal the match
 */

static
PT_THREAD(wait_readlines_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed)) {
  char ret;
  ret = wait_readlines_pt(pt, at, data, len, consumed);
  if (ret == PT_ENDED)
    process_post(&a6at, at_match_event, at);
  return ret;
}

static
PT_THREAD(wait_gpsrd_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed)) {

  char ret, *valid;
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

  ret = wait_readlines_pt(pt, at, data, len, consumed);
  printf("GPS=%s\n", atline);

  const char *delim = " \t\r,";
  time = strtok((char *)&atline[0], (const char *) delim);

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
    sc16is_gpio_clr_bit(G_LED_RED);
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
    res = sscanf(atline,"%2d%2d%2d.%3c,%1c,%lf,%1c,%lf,%1c,%lf,%lf,%2d%2d%2d",
		 &hour, &min, &sec, foo, &valid, &lat, c, &longi, c, &speed, &course, &day, &mon, &year);
#endif    
      //nmew_time(year, mon, day, hour, min, sec );
  *consumed = 0;
  return ret;
}

/*
 * Callback for dotted quad matching. Match function already filled in dotquadstr. Copy to status.
 */
static char dotquadstr[sizeof("255.255.255.255")];
static
PT_THREAD(wait_dotquad_callback(struct pt *pt, struct at_wait *at, uint8_t *data, int len, int *consumed)) {
  PT_BEGIN(pt);
#if NETSTACK_CONF_WITH_IPV6
  snprintf(status.ipaddr, sizeof(status.ipaddr), "::ffff:%s", dotquadstr);
#else
  snprintf(status.ipaddr, sizeof(status.ipaddr), "%s", dotquadstr);
#endif 
  process_post(&a6at, at_match_event, at);
  *consumed = 0;
  PT_END(pt);
}

/*
 * Match functions. Return a negative number if current data string does
 * not match search pattern. If there is a match, return number of bytes that 
 * were consumed in last call.
 */

/* Match byte: check one byte at a time */
static int at_match_byte(struct at_wait *at, uint8_t *data, int len) {
  if (*data == at->str[at->pos]) {
    at->pos += 1;
    if (at->str[at->pos] == '\0') {
      return 1;
    }
  }
  else { 
   at->pos = 0;
  }
  return -1;
}

/* 
 * Match an IPv4 address in dotted quad notation 
 * Use at->pos to record how many chars have been recognized
 * so far and stored. Matching is rough: wait for newline,
 * and then check if the buffer appears to contain a dotted quad.
 */

static int at_match_dotquad(struct at_wait *at, uint8_t *data, int len) {
  int b1, b2, b3, b4;

  if (*data == '\r' || *data == '\n') {
    /* End of line -- have we collected a dotted quad in the buffer? */
    if (at->pos < sizeof(dotquadstr)) {
      dotquadstr[at->pos] = '\0';
      /* Rough check -- look for 3 dots and 4 integers */ 
      if (sscanf(dotquadstr, "%d.%d.%d.%d", &b1, &b2, &b3, &b4) == 4) {
        at->pos = 0; /* reset */
        return 1;
      }
      else {
        /* No dotted quad -- restart. */
        at->pos = 0;
      }
    }
  }
  else if (at->pos < sizeof(dotquadstr)) {
    /* One more char for the buffer */ 
      dotquadstr[at->pos++] = *data;    
  }
  else {
    /* too long -- restart. */
    at->pos = 0;
  }
  /* Not done yet */
  return -1;
}

#define MAXWAIT 6
struct at_wait *at_waitlist2[MAXWAIT];

uint8_t at_numwait;
uint8_t at_numwait_permanent;

static void
start_at(struct at_wait *at) {
  if (at_numwait >= MAXWAIT) {
    printf("Error starting %s: too many at_waits (%d)\n", at->str, at_numwait);
    {
      int i;
      for (i = 0; i < at_numwait; i++)
        printf("%d: %s\n", i, at_waitlist2[i]->str);
    }
    return;
  }
  at_waitlist2[at_numwait] = at;
  at_numwait++;
  at->pos = 0;
}

static void
stop_at(struct at_wait *at) {
  at->pos = 0;
}

static void
start_atlist(struct at_wait *at, ...) {
  va_list valist;
  
  va_start(valist, at);
  while (at) {
    printf(" '%s' ", at->str);
    start_at(at);
    at = va_arg(valist, struct at_wait *);
  }
}

static void
stop_atlist(struct at_wait *at, ...) {
  va_list valist;
  
  va_start(valist, at);
  while (at) {
    stop_at(at);
    at = va_arg(valist, struct at_wait *);
  }
  at_numwait = at_numwait_permanent;
}


static void
wait_init() {
  at_numwait = 0;
  /* The following are to detect async events -- permanently active */
  start_at(&wait_ciprcv);
  start_at(&wait_tcpclosed);  
  start_at(&wait_gpsrd);  
  at_numwait_permanent = at_numwait;
}

/*
 * Main loop for input matching. Take one byte at a time, and call each
 * active matching state machine. 
 * If one state mechine returns a non-negative value, it is a complete match. 
 * Then call the corresponding callback protothread with the remaining
 * input data.
 */

/* Flag for locking AT commands. Set to non-zero when trigger has been
 * matched, and callback function is active and consuming input. Don't 
 * issue new AT commands while in this state; it would garble the
 * the input.
 */
static uint8_t matching = 0; 

PT_THREAD(wait_fsm_pt(struct pt *pt, uint8_t *data, unsigned int len)) {
  static uint8_t i;
  static uint8_t datapos;
  static struct pt subpt;
  static struct at_wait *at;
  int match;
  int consumed;

  static int again;
  PT_BEGIN(pt);
  while (1) {
  again = 0;
  
again:
  matching = 0;
    if (len > 0) {
      for (datapos = 0; datapos < len; datapos++){
        for (i = 0; i < at_numwait; i++) {
          at = at_waitlist2[i];
          match = at->match(at, &data[datapos], len-datapos);
          if (match >= 0) {
            /* An async (permanent) event? Then mark that it is active right now
             * so that other events can be postponed 
             */
            if (i < at_numwait_permanent)
              matching = 1;
            if (again) {
              printf("Again -- ");
              again = 0;
            }
            datapos += match; /* Consume matched chars */
            /* Run callback protothread -- loop until it has finished (PT_ENDED or PT_EXITED) */
            PT_INIT(&subpt);
            while (at->callback(&subpt, at, &data[datapos],len-datapos, &consumed) == PT_YIELDED) {
              PT_YIELD(pt);
            }
            matching = 0;
            //process_poll(&sc16is_reader);
            if (consumed < len-datapos) {
              /* Not all input consumed. Update data pointer and length, 
               * and continue scanning without returning.
               * This is to take care of the case when an async event happens
               * while we are looking for a synchronous event.  
               */
              data += datapos+consumed;
              len -= datapos+consumed;
              again = 1;
              goto again;
            }
            PT_RESTART(pt);
          }
        }
      }
    }
    PT_YIELD(pt);
  }
  PT_END(pt);
}

static void
dumpstr(unsigned char *str) {
  unsigned char *s = str;

  printf("    ");
  while (*s) {
    if (*s == '\n')
      printf("\n    ");
    else if (*s >= ' ' && *s <= '~')
      printf("%c", *s);
    else
      printf("*");
    s++;
  }
  printf("\n");
}

int
module_init(uint32_t baud)
{
  if(i2c_probed & I2C_SC16IS) {

    sc16is_init();
    //sc16is_gpio_set_dir(G_RESET | G_PWR | G_U_5V_CTRL | G_SET | G_LED_YELLOW | G_LED_RED | G_GPIO7);
    sc16is_gpio_set_dir(G_RESET | G_PWR | G_U_5V_CTRL | G_LED_YELLOW | G_LED_RED | G_GPIO7);
    sc16is_gpio_set((G_LED_RED|G_LED_YELLOW));
    sc16is_uart_set_speed(baud);
    /* sc16is_arch_i2c_write_mem(I2C_SC16IS_ADDR, SC16IS_FCR, SC16IS_FCR_FIFO_BIT); */
    status.state = GPRS_STATE_IDLE;
    sc16is_tx((uint8_t *)"AT", sizeof("AT"-1));
    return 1;
  }
  return 0;
}



int len;
uint8_t buf[200];

PROCESS_THREAD(sc16is_reader, ev, data)
{
  static struct pt wait_pt;

  PROCESS_BEGIN();

  wait_init();
  PT_INIT(&wait_pt);
  
  while(1) {
    PROCESS_PAUSE();
    if( i2c_probed & I2C_SC16IS ) {
      len = sc16is_rx(buf, sizeof(buf));
      if (len) {
        buf[len] = '\0';
        dumpstr(buf);
        wait_fsm_pt(&wait_pt, buf, len);
      }
    }
  }
  PROCESS_END();
}

#if 0
static void
sendbuf_fun (unsigned char *buf, size_t len) {
  size_t remain = len;

  while (remain > 0) {
    remain -= sc16is_tx(&buf[len - remain], remain); 
  }
}
#endif

static size_t sendbuf_remain;
#define sendbuf(buf, len) { \
  sendbuf_remain = len; \
  while (sendbuf_remain > 0) { \
    if (matching) { \
      printf("MATCHING\n"); \
      PROCESS_WAIT_UNTIL(matching == 0); \
      printf("DONE MATCHING\n"); \
    } \
    sendbuf_remain -= sc16is_tx(&(buf)[len - sendbuf_remain], sendbuf_remain); \
    continue; \
  } \
}

static struct etimer et;
#define GPRS_EVENT(E) (E >= a6at_gprs_init && E <= at_match_event)

//#define ATSTR(str) sendbuf_fun(((unsigned char *) str), strlen(str))
//#define ATSTR(str) sendbuf(((unsigned char *) str), strlen(str))
#define ATSTR(str) {printf("-->%s\n", str); sendbuf(((unsigned char *) str), strlen(str))}
#define ATBUF(buf, len) sendbuf(buf, len)

#define ATWAIT2(SEC, ...)  {                    \
  at = NULL; \
  printf("---start_atlist:%d: (%d sec)", __LINE__, SEC);    \
  start_atlist(__VA_ARGS__, NULL);            \
  printf("\n"); \
  etimer_set(&et, (SEC)*CLOCK_SECOND);                         \
  while (1) { \
    PROCESS_WAIT_EVENT();                           \
    if(etimer_expired(&et)) { \
      printf("---timeout:%d\n", __LINE__);                 \
      stop_atlist(__VA_ARGS__, NULL);                      \
      break; \
    } \
    else if(ev == at_match_event) { \
      etimer_stop(&et); \
      at = (struct at_wait *) data;                \
      printf("---got:%d '%s'\n", __LINE__, at->str);     \
      stop_atlist(__VA_ARGS__, NULL);             \
      break; \
    } \
    else { \
      if (GPRS_EVENT(ev)) { \
        /* This event is for us, but we can't do it now. \
         * Put it on the event queue for later. \
         */ \
          printf("---enq:%d\n", __LINE__);     \
          enqueue_event(ev, data); \
      } \
    } \
  } \
  }

#define CLOCKDELAY(ticks) {\
  printf("%d: start clock etimer %u, clock_time() = %lu\n", __LINE__, ticks, clock_time()); \
  etimer_set(&et, (ticks));                                             \
  while (1) { \
    PROCESS_WAIT_EVENT();                           \
    if(etimer_expired(&et)) { \
      printf("%d: timeout: clock_time() = %lu\n", __LINE__, clock_time()); \
      break; \
    } \
    else { \
      if (GPRS_EVENT(ev)) { \
        /* This is a event for us, but we can't do it now. \
         * Put it on the event queue for later. \
         */ \
          enqueue_event(ev, data); \
      } \
    } \
  } \
  }

#define DELAY(SEC) {\
  printf("%d: start etimer %lu, clock_time() = %lu\n", __LINE__, (clock_time_t) SEC*CLOCK_SECOND, clock_time()); \
  CLOCKDELAY((SEC)*CLOCK_SECOND);                                 \
  }

static void
event_init() {
  a6at_gprs_init = process_alloc_event();
  a6at_gprs_connection = process_alloc_event();
  a6at_gprs_send = process_alloc_event();
  a6at_gprs_close = process_alloc_event();  
  
  at_match_event = process_alloc_event();
  sc16is_input_event = process_alloc_event();
}

#define GPRS_MAX_NEVENTS 8
struct gprs_event {
  process_event_t ev;
  void *data;
} gprs_event_queue[GPRS_MAX_NEVENTS];
static int gprs_nevents;
static int gprs_firstevent;

static void
event_queue_init() {
  gprs_nevents = 0;
  gprs_firstevent = 0;
}

static void
enqueue_event(process_event_t ev, void *data) {
  int index;
  if (gprs_nevents >= GPRS_MAX_NEVENTS) {
    printf("Fatal error: GPRS event queue full adding ev %d\n", ev);
    return;
  }
  index = (gprs_firstevent+gprs_nevents) % GPRS_MAX_NEVENTS;
  gprs_event_queue[index].ev = ev; gprs_event_queue[index].data = data;
  gprs_nevents++;
}

static struct gprs_event *
dequeue_event() {
  struct gprs_event *gprs_event;
  if (gprs_nevents == 0)
    return NULL;
  gprs_event = &gprs_event_queue[gprs_firstevent];
  gprs_nevents--;
  gprs_firstevent = (gprs_firstevent + 1) % GPRS_MAX_NEVENTS;
  return gprs_event;
}

static struct tcp_socket_gprs *socket;
static struct gprs_connection *gprsconn;

int
in_atline(char *tok)
{
  int i;
  const char *delim = " \t\r";
  char *p = strtok((char *)&atline[0], (const char *) delim);
  printf("atline=%s", atline);
  while (p) {
    printf("tok=%s p=%s\n", tok, p);
    if(!strcmp(p, tok)) {
      return 1;
    }
    p = strtok(NULL, delim);
  }
  return 0;
}

PROCESS_THREAD(a6at, ev, data) {
  //unsigned char *res;
  struct at_wait *at;
  static uint8_t minor_tries, major_tries;
  char str[80];

  PROCESS_BEGIN();
  leds_init();
  event_init();
  event_queue_init();
  
 again:
#ifdef NBIOT   
  module_init(9600);
#else
  module_init(115200);
#endif    
  {
    uint8_t s;

    set_board_5v(0); /* Power cycle the board */
    DELAY(4);
    set_board_5v(1);
    DELAY(2);
    s = sc16is_gpio_get();
    printf("LOOP GPIO=0x%02x\n", sc16is_gpio_get());
    clr_bit(&s, G_PWR);
    set_bit(&s, G_RESET);

    set_bit(&s, G_LED_RED); /*OFF */
    set_bit(&s, G_LED_YELLOW);
    sc16is_gpio_set(s);
    DELAY(2);
    clr_bit(&s, G_RESET);
    sc16is_gpio_set(s);
    /* start */
    DELAY(2);
    printf("Start Radio\n");
    s = sc16is_gpio_get();
    set_bit(&s, G_PWR);
    sc16is_gpio_set(s);

    ATSTR("AT\r");
    DELAY(5);
    status.module = GPRS_MODULE_UNNKOWN;
    ATSTR("ATI\r");
    ATWAIT2(10, &wait_ati);
    if (at == NULL) {
      printf("No module version found\n");
    }
    else {
      int i;
      const char *delim = " \t\r,";
      char *p = strtok((char *)&atline[0], (const char *) delim);
      for(i=0; i <  sizeof(atline); i++) {
	p = strtok(NULL, delim);
	if(!strcmp(p, "A6")) {
	  status.module = GPRS_MODULE_A6;

	  /* 
	     Workaround and fix and
	     needs investigation. It seems like the A6 
	     module is not compatible with UART sleep mode.
	     Can be an A6 firmware issue.
	  */
	  sc16is_sleep_mode(0);
	  break;
	}
	if(!strcmp(p, "A7")) {
	  status.module = GPRS_MODULE_A7;
	  break;
	}
      }
    }

#ifdef GPRS_CONF_FORCE_A6
    status.module = GPRS_MODULE_A6; /* To avoid GPS with A7 */
#endif
    printf("Module version %d\n", status.module);

    if(status.module == GPRS_MODULE_A7) {
      ATSTR("AT+GPS=0\r");
      ATWAIT2(10, &wait_ok);
      if (at == NULL) {
	printf("GPS not disabled\n");
      }
      
      ATSTR("AT+GPS=1\r");
      ATWAIT2(10, &wait_ok);
      if (at == NULL) {
	printf("GPS not enabled\n");
      }
      
      ATSTR("AT+GPSRD=30\r");
      ATWAIT2(10, &wait_gpsrd);
      if (at == NULL) {
	printf("GPSRD not enabled\n");
      }
    }
  }

#ifdef NBIOT
  ATSTR("AT+CPIN?\r");
  ATWAIT2(10, &wait_ok);
  ATSTR("AT+IPR?\r");
  ATWAIT2(10, &wait_ok);
  ATSTR("AT+GSV\r");
  ATWAIT2(10, &wait_ok);
  ATSTR("AT+CBAND=?\r");
  ATWAIT2(10, &wait_ok);
  //ATSTR("AT+CBAND=\"8\"\r");
  //ATWAIT2(10, &wait_ok);
  ATSTR("AT+CBAND?\r");
  ATWAIT2(10, &wait_ok);
#endif
  
  /* Wait for registration status to become 1 (local registration)
   * or 5 (roaming) or 10 (roaming, non-preferred)
   */
  major_tries = 0;
  while (major_tries++ < 10) {
    static uint8_t creg;

    ATSTR("AT+CREG?\r");
    ATWAIT2(10, &wait_creg);
    if (at == NULL)
      continue;
    creg = atoi((char *) atline /*foundbuf*/);
    printf("creg atoi(\"%s\") -> %d\n", atline, creg);
    if (creg == 1 || creg == 5 || creg == 10) /* Wait for registration */
      status.state = GPRS_STATE_REGISTERED;
      break; 
  }
  if (major_tries >= 10) {
#ifdef GPRS_DEBUG
    printf("GPRS registration timeout\n");
#endif /* GPRS_DEBUG */
    gprs_statistics.resets += 1;
    goto again;
  }
  
  /* Then activate context */
  major_tries = 0;
  while (major_tries++ < 10 && status.state != GPRS_STATE_ACTIVE)
  {
    static struct gprs_context *gcontext;

    gcontext = &gprs_context;
    /* Deactivate PDP context */
    sprintf(str, "AT+CSTT=\"%s\", \"\", \"\"\r", gcontext->apn); /* Start task and set APN */
    ATSTR(str);   
    ATWAIT2(20, &wait_ok);
    ATSTR("AT+CGATT=1\r");
    ATWAIT2(5, &wait_ok);
    ATSTR("AT+CIPMUX=0\r");
    ATWAIT2(5, &wait_ok);
    minor_tries = 0;
    while (minor_tries++ < 10) {
      sprintf(str, "AT+CGDCONT=1,%s,%s\r", gcontext->pdptype, gcontext->apn); /* Set PDP (Packet Data Protocol) context */
      ATSTR(str);
      ATWAIT2(5, &wait_ok);
      ATSTR("AT+CGACT=1,1\r");       /* Sometimes fails with +CME ERROR:148 -- seen when brought up initially, then it seems to work */
      ATWAIT2(20, &wait_ok,  &wait_cmeerror);
      if (at == &wait_ok) {
        break;
      }
      if (at == &wait_cmeerror) {
#ifdef GPRS_DEBUG
        printf("CGACT failed with CME ERROR:%s\n", atline);
#endif /* GPRS_DEBUG */
        gprs_statistics.at_errors += 1;
      }
      else {
        gprs_statistics.at_timeouts += 1;
      }
      DELAY(5);
    }
    if (minor_tries++ >= 10)
      continue;
      
    minor_tries = 0;
    while (minor_tries++ < 10) {
      ATSTR("AT+CIPSTATUS?\r"); 
      ATWAIT2(60, &wait_cipstatus);
      if (at == &wait_cipstatus) {
	printf("atline=%s\n", atline);
	if ((strncmp((char *) atline, "0,IP GPRSACT", strlen("0,IP GPRSACT")) == 0) || (in_atline("0,IP"))) {
	  gcontext->active = 1;
	  status.state = GPRS_STATE_ACTIVE;
	  break;
	}
	else {
          DELAY(5);
	}
      }
    }
    if (minor_tries++ >= 10) {
      /* Failed to activate */
      ATSTR("AT+CIPSHUT\r");
      ATWAIT2(10, &wait_ok);
      continue;
    }
  } /* Context activated */
  if (major_tries >= 10) {
    gprs_statistics.resets += 1;
    goto again;
  }

  /* Get IP address */
  ATSTR("AT+CIFSR\r");
  ATWAIT2(2, &wait_dotquad);
  if (at == NULL)  {
    gprs_statistics.at_timeouts += 1;
  }

#ifdef GPRS_DEBUG
  printf("GPRS initialised\n");  
#endif /* GPRS_DEBUG */
  process_post(PROCESS_BROADCAST, a6at_gprs_init, NULL);

  if(status.module == GPRS_MODULE_A7) {
    ATSTR("AT+AGPS=1\r");
    ATWAIT2(25, &wait_ok);
    if (at == NULL) {
      printf("AGPS not enabled\n");
    }
    ATSTR("AT+GPSRD=30\r");
    ATWAIT2(10, &wait_gpsrd);
    if (at == NULL) {
      printf("GPSRD not enabled\n");
    }
  }

  /* Main loop. Wait for GPRS command and execute them */
  while(1) {
    static struct gprs_event *gprs_event;
  nextcommand:
    gprs_event = dequeue_event();
    if (gprs_event == NULL) {
      PROCESS_WAIT_EVENT();
      if (GPRS_EVENT(ev))
          enqueue_event(ev, data);
      goto nextcommand;
    }

    if (gprs_event->ev == a6at_gprs_connection) {
#ifdef GPRS_DEBUG
      printf("A6AT GPRS Connection\n");
#endif /* GPRS_DEBUG */


      yled_interval = CLOCK_SECOND*2;
      process_post(&yled, 3, &yled_interval);

      gprsconn = (struct gprs_connection *) gprs_event->data;
      minor_tries = 0;
      while (minor_tries++ < 10) {
        printf("Here is connection %s %s:%d\n", gprsconn->proto, gprsconn->ipaddr, uip_htons(gprsconn->port));
        sprintf(str, "AT+CIPSTART=\"%s\",\"%s\",%d\r", gprsconn->proto, gprsconn->ipaddr, uip_ntohs(gprsconn->port));
        ATSTR(str);
        ATWAIT2(60, &wait_connectok, &wait_cmeerror, &wait_commandnoresponse);
        if (at == &wait_connectok) {
          gprs_statistics.connections += 1;
          call_event(gprsconn->socket, TCP_SOCKET_CONNECTED);
	  yled_interval = CLOCK_SECOND/4;
	  process_post(&yled, 3, &yled_interval);
          goto nextcommand;
        }
        else if (at == &wait_commandnoresponse) {
          /* Give it some more time. It happens that the connection succeeds seconds after COMMAND NO RESPONSE! */
          ATWAIT2(15, &wait_connectok);
          if (at == &wait_connectok) {
            gprs_statistics.connections += 1;
            call_event(gprsconn->socket, TCP_SOCKET_CONNECTED);
	    yled_interval = CLOCK_SECOND/2;
	    process_post(&yled, 3, &yled_interval);
            goto nextcommand;
          }
        }
        /* If we ended up here, we failed to set up connection */
        if (at == NULL) {
          /* Timeout */
          gprs_statistics.connfailed += 1;
          ATSTR("AT+CIPCLOSE\r");
          ATWAIT2(5, &wait_ok);
          ATSTR("AT+CIPSHUT\r");
          ATWAIT2(5, &wait_ok);
          call_event(gprsconn->socket, TCP_SOCKET_TIMEDOUT);
          goto nextcommand;
        }        
        else if (at == &wait_cmeerror) {
          /* COMMAND NO RESPONSE! timeout. Sometimes it take longer though and have seen COMMAND NON RESPONSE! followed by CONNECT OK */ 
          /* Seen +CME ERROR:53 */
          printf("CIPSTART failed\n");
          ATSTR("AT+CREG?\r");
          ATWAIT2(2, &wait_ok);
          gprs_statistics.at_errors += 1;
          ATSTR("AT+CIPCLOSE\r");
          ATWAIT2(15, &wait_ok);//ATWAIT2(5, &wait_ok, &wait_cmeerror);
          ATSTR("AT+CIPSHUT\r");
          ATWAIT2(15, &wait_ok);//ATWAIT2(5, &wait_ok,  &wait_cmeerror);

	  /* Test to cure deadlock when closing/shutting down  --ro */
	  if (minor_tries++ > 10) {
	    gprs_statistics.connfailed += 1;
	    goto again;
	  }
	  else {
	    continue;
	  }
        }
      } /* minor_tries */
      if (minor_tries >= 10) {
        gprs_statistics.connfailed += 1;
        call_event(gprsconn->socket, TCP_SOCKET_TIMEDOUT);
        goto nextcommand;
      }
    } /* ev == a6at_gprs_connection */
    else if (gprs_event->ev == a6at_gprs_send) {
      static uint16_t remain;
      static uint16_t len;
      static uint8_t *ptr;
#ifdef GPRS_DEBUG
      printf("A6AT GPRS Send\n");
#endif /* GPRS_DEBUG */
      gprsconn = (struct gprs_connection *) gprs_event->data;
      ptr = gprsconn->output_data_ptr;
      //socket = gprsconn->socket;
      //remain = socket->output_data_len;
      remain = gprsconn->output_data_len;
      ATSTR("ATE0\r\n");
      ATWAIT2(5, &wait_ok);
      while (remain > 0) {
        len = (remain <= GPRS_MAX_SEND_LEN ? remain : GPRS_MAX_SEND_LEN);
        printf("Send %d bytes @0x%x\n", len, (unsigned) &ptr[gprsconn->output_data_len-remain]);
        sprintf((char *) buf, "AT+CIPSEND=%d\r", len);
        ATSTR((char *) buf); /* sometimes CME ERROR:516 */
        ATWAIT2(5, &wait_sendprompt, &wait_cmeerror);
        if (at == NULL) {
          gprs_statistics.at_timeouts += 1;
          ATSTR("ATE1\r\n");
          ATWAIT2(5, &wait_ok);
          goto failed;
        }
        else if (at == &wait_cmeerror) {
          gprs_statistics.at_errors += 1;
          ATSTR("ATE1\r\n");
          ATWAIT2(5, &wait_ok);
          goto failed;
        }
        ATBUF(&ptr[gprsconn->output_data_len-remain], len);
        ATWAIT2(30, &wait_ok, &wait_commandnoresponse);
        if (at == NULL || at == &wait_commandnoresponse) {
          gprs_statistics.at_timeouts += 1;
          goto failed;
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
      ATSTR("ATE1\r\n");
      ATWAIT2(5, &wait_ok);

      //call_event(socket, TCP_SOCKET_DATA_SENT);
      gprs_call_event(gprsconn, GPRS_CONN_DATA_SENT);      
    } /* ev == a6at_gprs_send */
    else if (gprs_event->ev == a6at_gprs_close) {

#ifdef GPRS_DEBUG
      printf("A6AT GPRS Close\n");
#endif /* GPRS_DEBUG */
      gprsconn = (struct gprs_connection *) gprs_event->data;
      socket = gprsconn->socket;

      ATSTR("AT+CIPCLOSE\r");
      ATWAIT2(15, &wait_ok, &wait_cmeerror);
      if (at == &wait_ok) {
        call_event(socket, TCP_SOCKET_CLOSED); 
      }
      else {
        gprs_statistics.at_timeouts += 1;
        call_event(socket, TCP_SOCKET_CLOSED);
      }
    } /* ev == a6at_gprs_close */
#ifdef GPRS_DEBUG
    else {
      printf("A6AT GPRS Unknown event %d\n", gprs_event->ev);
    }
#endif /* GPRS_DEBUG */

    ATSTR("AT+CSQ\r");
    ATWAIT2(5, &wait_csq);
    if (at == NULL)
      continue;
    status.rssi = atoi((char *) atline /*foundbuf*/);
    printf("rssi atoi(\"%s\") -> %d\n", atline, status.rssi);

    continue;
  failed:
    /* Timeout */
    ATSTR("AT+CIPCLOSE\r");
    ATWAIT2(5, &wait_ok, &wait_cmeerror);
    ATSTR("AT+CIPSHUT\r");
    ATWAIT2(5, &wait_ok, &wait_cmeerror);
    call_event(gprsconn->socket, TCP_SOCKET_TIMEDOUT);
  }
  PROCESS_END();
}

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
