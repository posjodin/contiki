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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "i2c.h"
#include "dev/leds.h"
#include "dev/sc16is/sc16is.h"
#include "tcp-socket-gprs.h"
#include "gprs-a6.h"

#define GP0 1
#define GP1 2
#define GP2 4
#define GP3 8
#define G_RESET GP0
#define G_SLEEP  GP1
#define G_PWR_KEY GP2

#define APN "4g.tele2.se"
#define PDPTYPE "IP"

uint32_t baud;
uint8_t at[] = {'A', 'T', 0xd };

#define GPRS_MAX_CONNECTION 1
struct gprs_connection gprs_connections[GPRS_MAX_CONNECTION];
struct gprs_context gprs_context;

process_event_t sc16is_input_event;
process_event_t at_match_event;

/*---------------------------------------------------------------------------*/
PROCESS(sc16is_reader, "I2C UART input process");
//PROCESS(sc16is_at, "I2C UART AT emitter");
PROCESS(gprs_init_pt, "GPRS Initializer");
PROCESS(a6at, "GPRS A6 module");
extern struct process testapp;
AUTOSTART_PROCESSES(&sc16is_reader, &a6at, &testapp); //&sc16is_at);
//AUTOSTART_PROCESSES(&sc16is_reader, &a6at);

/*---------------------------------------------------------------------------*/
static struct gprs_connection *alloc_gprs_connection() {
  return &gprs_connections[GPRS_MAX_CONNECTION-1];
}
/*---------------------------------------------------------------------------*/
static void
free_gprs_connection() {
  return;
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
gprs_init() {
  gprs_set_context(&gprs_context, "IP", APN);
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
  process_post(&a6at, a6at_gprs_activate, gcontext);
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Do we need this */
int
gprs_register(struct gprs_connection *gconn,
              struct tcp_socket_gprs *socket,
              void *callback) {

  gconn->socket = socket;
  gconn->callback = callback;
  return 0;
}
/*---------------------------------------------------------------------------*/
struct gprs_connection *
gprs_connection(const char *proto, const char *ipaddr, uint16_t port,
                struct tcp_socket_gprs *socket) {
  struct gprs_connection *gprsconn;
  if ((gprsconn = alloc_gprs_connection()) == NULL)
    return NULL;
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
gprs_send(struct tcp_socket_gprs *socket) {
  (void) process_post(&a6at, a6at_gprs_send, socket->g_c);
}
/*---------------------------------------------------------------------------*/
void
gprs_close(struct tcp_socket_gprs *socket) {
  (void) process_post(&a6at, a6at_gprs_close, socket->g_c);
}
/*---------------------------------------------------------------------------*/
int len;
uint8_t buf[200];

#define MAXWAIT 3
#define PERMANENT 0
static struct waitfor {
  unsigned char *str;
  uint8_t pos;
  uint8_t found;  
} waitfor[MAXWAIT];


uint8_t found;
static unsigned char foundbuf[200];
static int foundpos;
static unsigned char atbuf[256];
static uint16_t atpos;

static void startwait(char *str, ...) { //char *str, char *end) {
  va_list valist;
  char *s;
  uint8_t w;
  
  va_start(valist, str);
  s = str;
  w = PERMANENT;
  while (s && w < MAXWAIT) {
    printf(" %s", s);
    waitfor[w].str = (unsigned char *)s;
    waitfor[w].pos = 0;
    waitfor[w].found = 0;
    w++;
    foundpos = 0;
    s = va_arg(valist, char *);
  }

}


static void
stopwait() {
  waitfor[PERMANENT].str = NULL;
}

static unsigned char
*matchwait(unsigned char *bytes) {
  uint8_t pos;
  uint8_t w = 0;  
  for(w = 0; w < MAXWAIT && waitfor[w].str != NULL; w++) {
    if (!waitfor[w].found) {
      pos = 0;
      while (bytes[pos] != 0) {
        if (bytes[pos++] == waitfor[w].str[waitfor[w].pos++]) {
          if (waitfor[w].str[waitfor[w].pos] == 0) {
            /* Found it */
            uint8_t foundpos;
            waitfor[w].found = 1;
            foundpos = 0;
            while (bytes[pos] != 0 && bytes[pos] != '\n' && foundpos < sizeof(foundbuf))
              foundbuf[foundpos++] = bytes[pos++];
            foundbuf[foundpos] = 0;
            //printf("Found @%d \"%s\": remains %d: \"%s\"\n", w, waitfor[w].str, foundpos, foundbuf);
            //printf("Match \"%s\", found \"%s\": remains %d: \"%s\"\n", buf, waitfor[w].str, foundpos, foundbuf);
            return waitfor[w].str;
          }
        }
        else {
          waitfor[w].pos = 0;
        }
      }
    }
  }
  return NULL;
}
      
static void
dumpstr(unsigned char *str) {
  unsigned char *s = str;

  printf("    ");
  while (*s) {
    if (*s == '\n')
      printf("\n    ");
    else
      printf("%c", *s);
    s++;
  }
  printf("\n");
}

static uint8_t *match;
void module_init(void)
{
  if( i2c_probed & I2C_SC16IS ) {
    /* GPIO set output */
    sc16is_gpio_set_dir(G_RESET|G_PWR_KEY|G_SLEEP);

    sc16is_init();
    baud = 115200;
    sc16is_uart_set_speed(baud);
    //sc16is_arch_i2c_write_mem(I2C_SC16IS_ADDR, SC16IS_FCR, SC16IS_FCR_FIFO_BIT);
    sc16is_tx(at, sizeof(at));
  }
}



PROCESS_THREAD(sc16is_reader, ev, data)
{
  PROCESS_BEGIN();

#if 0
  sc16is_input_event = process_alloc_event();
  at_match_event = process_alloc_event();  

  module_init();
  leds_init();

  /* Fix baudrate  */
  sc16is_tx(at, sizeof(at));
#endif
  atpos = 0;
  memset(atbuf, 0, sizeof(atbuf));
  while(1) {
    PROCESS_PAUSE();
    if( i2c_probed & I2C_SC16IS ) {
      len = sc16is_rx(buf, sizeof(buf));
      if (len) {
        buf[len] = '\0';
        dumpstr(buf);
        //printf("????matchwait \"%s\"\n", buf);
        match = matchwait(buf);
        if (match != NULL) {
          stopwait();
          //printf("\n===stopwait:");
          //printf(" \"%s\"\n", match);
          /* Tell other processes there was a match */
          if (process_post(PROCESS_BROADCAST, at_match_event, match) == PROCESS_ERR_OK) {
            PROCESS_WAIT_EVENT_UNTIL(ev == at_match_event);
          }
        }
      }
    }
  }
  PROCESS_END();
}

static struct etimer et;

static void
sendbuf(unsigned char *buf, size_t len) {
  sc16is_tx(buf, len); 
}

#define ATSTR(str) sendbuf((unsigned char *) str, strlen(str))
#define ATBUF(buf, len) sendbuf(buf, len)

#define ATWAIT(delay, ...)            \
  res = NULL; \
  printf("---startwait:%d: delay=%d", __LINE__, delay);     \
  startwait(__VA_ARGS__, NULL);            \
  printf("\n");                       \
  etimer_set(&et, delay); \
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et) || \
                           ev == at_match_event); \
  /* printf("#####WAIYED\n"); */                  \
  if(etimer_expired(&et)) { \
    printf("---timeout:%d\n", __LINE__);                 \
    stopwait(); \
    res = NULL; \
  } \
  else if(ev == at_match_event) { \
    etimer_stop(&et); \
    res = (unsigned char *) data;                \
    printf("---got %s\n", res);\
  }

static char *strings[] = {
  "HEJ \375hopp\n",
  "strng\n",
  "this isd data indeed\n",
};

void fnus() {
  int i;

  printf("sizeof is %d\n", sizeof(strings));
  for (i = 0; i < sizeof(strings)/sizeof(char *); i++)
    printf("%s, len %d\n", strings[i], strlen(strings[i]));
}

//#define APN "online.telia.se"
#define APN "4g.tele2.se"
#define PDPTYPE "IP"
#define IPADDR  "130.237.22.219"
#define PORTNO 9999


PROCESS_THREAD(a6at, ev, data) {
  unsigned char *res;
  char str[80];
  static struct gprs_connection *gprsconn;
  static struct tcp_socket_gprs *socket;

  
  PROCESS_BEGIN();
  a6at_gprs_init = process_alloc_event();
  a6at_gprs_activate = process_alloc_event();  
  a6at_gprs_connection = process_alloc_event();
  a6at_gprs_send = process_alloc_event();
  a6at_gprs_close = process_alloc_event();  
  
  sc16is_input_event = process_alloc_event();
  at_match_event = process_alloc_event();  

  module_init();
  leds_init();

  gprs_init();
  /* Fix baudrate  */
  sc16is_tx(at, sizeof(at));

 again:
  if (0){

  /* Reset module */
  printf("Resetting... gpio == 0x%x\n", sc16is_gpio_get());
  sc16is_gpio_set(sc16is_gpio_get()|G_RESET); /* Reset on */
  printf("Initializing... gpio == 0x%x\n", sc16is_gpio_get());
  /* Power on */
  sc16is_gpio_set(sc16is_gpio_get()|G_PWR_KEY|G_SLEEP); /* Power key on, no sleep */
  printf("Probing2... gpio == 0x%x\n", sc16is_gpio_get());
  ATWAIT(CLOCK_SECOND*5, "KOKO");
  sc16is_gpio_set(sc16is_gpio_get()&~G_PWR_KEY); /* Toggle power key (power will remain on) */
  printf("Probing3... gpio == 0x%x\n", sc16is_gpio_get());
  ATWAIT(CLOCK_SECOND*30, "OK");
  goto again;
  }

  {
    printf("Resetting... gpio == 0x%x\n", sc16is_gpio_get());
    sc16is_gpio_set(sc16is_gpio_get()|G_RESET); /* Reset on */
    printf("Sleeping... gpio == 0x%x\n", sc16is_gpio_get());
    ATWAIT(CLOCK_SECOND*5, "KOKO");
    sc16is_gpio_set((sc16is_gpio_get()&~G_RESET)|G_PWR_KEY|G_SLEEP); /* Toggle reset, power key on, no sleep */
    printf("Probing2... gpio == 0x%x\n", sc16is_gpio_get());
    ATWAIT(CLOCK_SECOND*5, "KOKO");
    sc16is_gpio_set(sc16is_gpio_get()&~G_PWR_KEY); /* Toggle power key (power will remain on) */
    printf("Probing3... gpio == 0x%x\n", sc16is_gpio_get());
    ATSTR("AT\r"); ATWAIT(CLOCK_SECOND*10, "OK");
    ATWAIT(CLOCK_SECOND*10, "KOOK");
  }

  /* Wait for registration status to become 1 (local registration)
   * or 5 (roaming)
   */
  while (1) {
    static uint8_t creg;
    ATWAIT(CLOCK_SECOND*2, "KOKO");
    ATSTR("AT+CIPSTATUS?\r");
    ATWAIT(CLOCK_SECOND*10, "+CIPSTATUS:0,");
    ATSTR("AT+CREG?\r");
    ATWAIT(CLOCK_SECOND*10, "+CREG: 1,");
    if (res == NULL)
      continue;
    creg = atoi((char *) foundbuf);
    if (creg == 1 || creg == 5 || creg == 10) /* Wait for status == 1 (local registration) */
      break; /* Wait for status == 1 (local registration) */    
  }
  printf("Done---- init\n");
  process_post(PROCESS_BROADCAST, a6at_gprs_init, NULL);

  while(1) {
  nextcommand:
    PROCESS_WAIT_EVENT();
    if (ev == a6at_gprs_activate) {
      printf("GPRS Activate\n");
      static struct gprs_context *gcontext;

      gcontext = (struct gprs_context *) data;
      /* Deactivate PDP context */
      sprintf(str, "AT+CSTT=\"%s\", \"\", \"\"\r", gcontext->apn); /* Start task and set APN */
      ATSTR(str);   ATWAIT(CLOCK_SECOND*20, "OK");

      ATSTR("AT+CGATT=1\r"); ATWAIT(CLOCK_SECOND*5, "OK"); /* Service attach */

      ATSTR("AT+CIPMUX=0\r"); ATWAIT(CLOCK_SECOND*5, "OK"); /* Single IP connection */

      ATSTR("AT+CREG?\r");ATWAIT(CLOCK_SECOND, "OK");
  
      sprintf(str, "AT+CGDCONT=1,%s,%s\r", gcontext->pdptype, gcontext->apn); /* Set PDP (Packet Data Protocol) context */
      ATSTR(str);  ATWAIT(CLOCK_SECOND*5, "OK");
      ATSTR("AT+CREG?\r");ATWAIT(CLOCK_SECOND, "OK");
      ATSTR("AT+CGACT=1,1\r"); ATWAIT(CLOCK_SECOND*20, "OK"); /* Activate context */

      ATSTR("AT+CREG?\r"); ATWAIT(CLOCK_SECOND*2, "OK");
      
      while (1) {
        ATSTR("AT+CIPSTATUS?\r"); 
        ATWAIT(CLOCK_SECOND*60, "+CIPSTATUS:0,");
        if (res != NULL && strcmp((char *) res, "+CIPSTATUS:0,") == 0) {
          if (strncmp((char *) foundbuf, "IP GPRSACT", strlen("IP GPRSACT")) == 0) {
            printf("GPRS is active\n");
            gcontext->active = 1;
            //process_post(PROCESS_BROADCAST, a6at_gprs_active, NULL);
            break;
          }
        }
        printf("GPRS not active\n");
        printf("Got gpio 0x%x\n", sc16is_gpio_get());
        ATSTR("AT+CREG?\r"); ATWAIT(CLOCK_SECOND*2, "OK");
      }
    } /* ev == a6at_gprs_activate */
    else if (ev == a6at_gprs_connection) {
      gprsconn = (struct gprs_connection *) data;

    try:
      printf("Here is connection %s %s:%d\n", gprsconn->proto, gprsconn->ipaddr, uip_htons(gprsconn->port));

      ATSTR("AT+CREG?\r"); ATWAIT(CLOCK_SECOND*2, "OK");
      ATSTR("AT+CIPSTATUS?\r"); ATWAIT(CLOCK_SECOND*2, "OK");

      sprintf(str, "AT+CIPSTART= \"%s\", %s, %d\r", gprsconn->proto, gprsconn->ipaddr, uip_ntohs(gprsconn->port));
      ATSTR(str); ATWAIT(CLOCK_SECOND*60, "CONNECT OK", "CME ERROR", "COMMAND NO RESPONSE!");
      if (res != NULL && (strcmp("CONNECT OK", (char *) res) == 0)) {
        call_event(gprsconn->socket, TCP_SOCKET_CONNECTED);
        goto nextcommand;
      }
      if (res != NULL && (strcmp("COMMAND NO RESPONSE!", (char *) res) == 0)) {
        /* Give it some more time. It happens that the connection succeeds after COMMAND NO RESPONSE! */
        ATWAIT(CLOCK_SECOND*15, "CONNECT OK");
        if (res != NULL && (strcmp("CONNECT OK", (char *) res) == 0)) {
          call_event(gprsconn->socket, TCP_SOCKET_CONNECTED);
          goto nextcommand;
        }
      }
      /* If we end up here, we failed to set up connection */
      if (res == NULL) {
        /* Timeout */
          ATSTR("AT+CIPCLOSE\r");  ATWAIT(CLOCK_SECOND*5, "OK");
          ATSTR("AT+CIPSHUT\r");  ATWAIT(CLOCK_SECOND*5, "OK");
          call_event(gprsconn->socket, TCP_SOCKET_TIMEDOUT);
          goto nextcommand;
      }        
      else if (res == NULL || (0 == strcmp((char *) res, "CME ERROR"))) {
        /* COMMAND NO RESPONSE! timeout. Sometimes it take longer though and have seen COMMAND NON RESPONSE! followed by CONNECT OK */ 
        /* Seen +CME ERROR:53 */
        /* After CME ERROR:53, seen CIPSTATUS stuck at IP START */
        printf("CIPSTART failed\n");
        ATSTR("AT+CREG?\r"); ATWAIT(CLOCK_SECOND*2, "OK");    
        ATSTR("AT+CIPCLOSE\r");  ATWAIT(CLOCK_SECOND*5, "OK");
        ATSTR("AT+CIPSHUT\r");  ATWAIT(CLOCK_SECOND*5, "OK");
        goto try;
      }

      /* */;
    } /* ev == a6at_gprs_connection */
    else if (ev == a6at_gprs_send) {
      static uint16_t remain;
      static uint16_t len;

      gprsconn = (struct gprs_connection *) data;
      socket = gprsconn->socket;
      remain = socket->output_data_len;
      while (remain > 0) {
        
        len = (remain <= GPRS_MAX_SEND_LEN ? remain : GPRS_MAX_SEND_LEN);
        sprintf((char *) buf, "AT+CIPSEND=%d\r", len);
        ATSTR((char *) buf); /* sometimes CME ERROR:516 */
        ATWAIT(CLOCK_SECOND*2, ">");
        ATBUF(&socket->output_data_ptr[socket->output_data_len-remain], len);
#ifdef notdef
        sprintf((char *) buf, "AT+CIPSEND=%d,", len-1);
        ATSTR((char *) buf); /* sometimes CME ERROR:516 */
        ATBUF(&socket->output_data_ptr[socket->output_data_len-remain], len);
        ATSTR((char *) "\r"); /* sometimes CME ERROR:516 */
#endif 
        ATWAIT(CLOCK_SECOND*10, "OK");
        if (remain > len) {
          memcpy(&socket->output_data_ptr[0],
                 &socket->output_data_ptr[len],
                 remain - len);
        }
        socket->output_data_len -= len;
        remain -= len;
      }
      call_event(socket, TCP_SOCKET_DATA_SENT);
    } /* ev == a6at_gprs_send */
    else if (ev == a6at_gprs_close) {
      gprsconn = (struct gprs_connection *) data;
      socket = gprsconn->socket;

      ATSTR("AT+CIPCLOSE\r");  ATWAIT(CLOCK_SECOND*15, "OK");
      if (res == NULL) {
        call_event(socket, TCP_SOCKET_TIMEDOUT);
      }
      else {
        call_event(socket, TCP_SOCKET_CLOSED);
      }
    } /* ev == a6at_gprs_close */
  }
  PROCESS_END();
}

/* Events:
+TCPCLOSED,0
+CIPRCV:4,hej
*/


PROCESS_THREAD(gprs_init_pt, ev, data) {
  unsigned char *res;
  char str[80];

  PROCESS_BEGIN();
 again:
  do {
    printf("Probing... gpio == 0x%x\n", sc16is_gpio_get());
    sc16is_gpio_set(sc16is_gpio_get()|G_PWR_KEY|G_SLEEP); /* Power key on, no sleep */
    printf("Probing2... gpio == 0x%x\n", sc16is_gpio_get());
    ATWAIT(CLOCK_SECOND*5, "KOKO");
    sc16is_gpio_set(sc16is_gpio_get()&~G_PWR_KEY); /* Toggle power key (power will remain on) */
    printf("Probing3... gpio == 0x%x\n", sc16is_gpio_get());
    ATSTR("AT\r"); ATWAIT(CLOCK_SECOND*10, "OK");
  }
  while (res == NULL);

  {
    printf("Resetting... gpio == 0x%x\n", sc16is_gpio_get());
    sc16is_gpio_set(sc16is_gpio_get()|G_RESET); /* Reset on */
    printf("Sleeping... gpio == 0x%x\n", sc16is_gpio_get());
    ATWAIT(CLOCK_SECOND*5, "KOKO");
    sc16is_gpio_set((sc16is_gpio_get()&~G_RESET)|G_PWR_KEY|G_SLEEP); /* Toggle reset, power key on, no sleep */
    printf("Probing2... gpio == 0x%x\n", sc16is_gpio_get());
    ATWAIT(CLOCK_SECOND*5, "KOKO");
    sc16is_gpio_set(sc16is_gpio_get()&~G_PWR_KEY); /* Toggle power key (power will remain on) */
    printf("Probing3... gpio == 0x%x\n", sc16is_gpio_get());
    ATSTR("AT\r"); ATWAIT(CLOCK_SECOND*10, "OK");

  }

 do {
    ATWAIT(CLOCK_SECOND*2, "KOKO");
    ATSTR("AT+CIPSTATUS?\r");
    ATWAIT(CLOCK_SECOND*60, "+CIPSTATUS:0,");
    ATSTR("AT+CREG?\r");
    ATWAIT(CLOCK_SECOND*10, "+CREG: 1,");
  } while (atoi((char *) foundbuf) != 1 && atoi((char *) foundbuf) != 5); /* Wait for status == 1 (local registration) */


  if (1){
    ATSTR("AT+CIPSTATUS?\r");
    ATWAIT(CLOCK_SECOND*10, "+CIPSTATUS:0,");
    ATSTR("AT+CGACT=1,0\r");  ATWAIT(CLOCK_SECOND*15, "OK"); 
    ATSTR("AT+CGACT=0,0\r");  ATWAIT(CLOCK_SECOND*15, "OK");
    ATSTR("AT+CGACT=0\r");  ATWAIT(CLOCK_SECOND*15, "OK");     
    if ((res == NULL) || (strncmp((char *) foundbuf, "IP START", strlen("IP START")) == 0)) { /* IP CLOSE OK?*/
      printf("IP STATUS not OK\n");
      ATSTR("AT+CGACT=1,0\r");  ATWAIT(CLOCK_SECOND*15, "OK"); 
      ATSTR("AT+CGACT=0,0\r");  ATWAIT(CLOCK_SECOND*15, "OK");
      ATSTR("AT+CGACT=0\r");  ATWAIT(CLOCK_SECOND*15, "OK");     
      ATSTR("AT+CIPSHUT=0\r"); ATWAIT(CLOCK_SECOND*15, "OK"); 
      ATSTR("AT+CGATT=0\r"); ATWAIT(CLOCK_SECOND*15, "OK"); 
      ATSTR("AT+CIPSTATUS?\r"); ATWAIT(CLOCK_SECOND*10, "OK");
      ATSTR("AT+CREG?\r"); ATWAIT(CLOCK_SECOND*10, "+CREG: 1,");
    }  
  }
  if (0) {
    ATSTR("AT+CREG?\r");
    ATWAIT(CLOCK_SECOND*10, "+CREG: 1,");
    if (atoi((char *) foundbuf) == 5) {
      printf("Roaming mode\n");
      ATSTR("AT+CGATT=0\r");
      ATWAIT(CLOCK_SECOND*10, "OK"); /* Echo on */
      ATSTR("AT+CREG?\r");
      ATWAIT(CLOCK_SECOND*10, "KOKO");
      ATSTR("AT+CREG?\r");
      ATWAIT(CLOCK_SECOND*10, "KOKO");
    }
  }  
  ATSTR("ATE1\r"); ATWAIT(CLOCK_SECOND*10, "OK"); /* Echo on */

  sprintf(str, "AT+CSTT=\"%s\", \"\", \"\"\r", APN); /* Start task and set APN */
  ATSTR(str);   ATWAIT(CLOCK_SECOND*20, "OK");

  ATSTR("AT+CREG?\r");ATWAIT(CLOCK_SECOND, "OK");

  ATSTR("AT+CGATT=1\r"); ATWAIT(CLOCK_SECOND*5, "OK"); /* Service attach */

  ATSTR("AT+CIPMUX=0\r"); ATWAIT(CLOCK_SECOND*5, "OK"); /* Single IP connection */

  ATSTR("AT+CREG?\r");ATWAIT(CLOCK_SECOND, "OK");
  
  sprintf(str, "AT+CGDCONT=1,%s,%s\r", PDPTYPE, APN); /* Set PDP (Packet Data Protocol) context */
  ATSTR(str);  ATWAIT(CLOCK_SECOND*5, "OK");
  ATSTR("AT+CREG?\r");ATWAIT(CLOCK_SECOND, "OK");
  ATSTR("AT+CGACT=1,1\r"); ATWAIT(CLOCK_SECOND*20, "OK"); /* Activate context */

  ATSTR("AT+CREG?\r"); ATWAIT(CLOCK_SECOND*2, "OK");
  ATSTR("AT+CIPSTATUS?\r"); 
  ATWAIT(CLOCK_SECOND*60, "+CIPSTATUS:0,");
  if (res != NULL && strcmp((char *) res, "+CIPSTATUS:0,") == 0) {
    if (strncmp((char *) foundbuf, "IP GPRSACT", strlen("IP GPRSACT")) == 0) {
      printf("GPRS is active\n");
      PROCESS_EXIT();
    }
    else {

      printf("GPRS not active\n");
      printf("Got gpio 0x%x\n", sc16is_gpio_get());
      ATSTR("AT+CREG?\r"); ATWAIT(CLOCK_SECOND*2, "OK");
      goto again;
    }
  }
  else {
    printf("No CIPSTATUS\n");
    goto again;
  }

  PROCESS_END();
}

#if 0
static int err;
extern struct process testapp;
PROCESS_THREAD(sc16is_at, ev, data)
{
  unsigned char *res;
  char str[80];
  static struct gprs_context gprs_context;

  err = 0;
  PROCESS_BEGIN();
  printf("HERE IS SC16it_at\n");
  //module_init();
  printf("Wait firsr 10\n");
  etimer_set(&et, CLOCK_SECOND*10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  printf("Wait 10 %d\n", CLOCK_SECOND);
  ATWAIT(CLOCK_SECOND*10, "ook");
  
  process_start(&testapp, NULL);
  process_start(&a6at, NULL);
  printf("PROCESS EXITING\n");
  PROCESS_EXIT();
  PROCESS_WAIT_EVENT_UNTIL(ev == a6at_gprs_init);
  printf("Here is at proc\n");

  gprs_set_context(&gprs_context, PDPTYPE, APN);
  printf("Did set context\n");
  PROCESS_WAIT_UNTIL(gprs_context.active);
  printf("CONTEXT is ACTIVE\n");
  ATWAIT(CLOCK_SECOND*10, "ook");
  gprs_connection("TCP", IPADDR, PORTNO);
  
 again:
#if 0
  process_start(&gprs_init_pt, NULL);
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_EXITED && data == &gprs_init_pt);  
  printf("GPRS INITIED\n");
  ATWAIT(CLOCK_SECOND*30, "LOOK");
#endif
  printf("GPIO: 0x%x\n\n", sc16is_gpio_get());
  ATSTR("AT+CREG?\r"); ATWAIT(CLOCK_SECOND*2, "OK");
  ATSTR("AT+CIPSTATUS?\r"); ATWAIT(CLOCK_SECOND*2, "OK");
  ATWAIT(CLOCK_SECOND*5, "KOKO");

 moreconnect:
  sprintf(str, "AT+CIPSTART= \"TCP\", %s, %d\r", IPADDR, PORTNO);
  ATSTR(str); ATWAIT(CLOCK_SECOND*120, "OK", "CME ERROR");
  if (res == NULL || (0 == strcmp((char *) res, "CME ERROR"))) {
    /* Seen +CME ERROR:53 */
    /* After CME ERROR:53, seen CIPSTATUS stuck at IP START */
    printf("CIPSTART failed\n");
    ATSTR("AT+CREG?\r"); ATWAIT(CLOCK_SECOND*2, "OK");    
    ATSTR("AT+CIPCLOSE\r");  ATWAIT(CLOCK_SECOND*5, "OK");
    goto again;
  }
  
  {
    static int i;
    static unsigned char buf[32];

    err = 0;
    for (i = 0; i < sizeof(strings)/sizeof(char *); i++) {
      ATSTR("AT+CREG?\r");ATWAIT(CLOCK_SECOND*5, "OK");
      ATSTR("AT+CIPSTATUS?\r");  ATWAIT(CLOCK_SECOND*5, "OK");
      //ATSTR("AT+CGACT=1,1\r"); ATWAIT(CLOCK_SECOND*20, "OK");

      sprintf((char *) buf, "AT+CIPSEND=%d\r", strlen(strings[i]));
      ATSTR((char *) buf); /* sometimes CME ERROR:516 */
      ATWAIT(CLOCK_SECOND*2, ">");
      ATSTR((char *) strings[i]);
      /* sometimes CME ERROR:516 */
      /* Sometimes COMMAND NO RESPONSE -- timeout after c:a 20 sec */
      ATSTR("\r");
      ATWAIT(CLOCK_SECOND*5, "OK");
      if (err || (res == NULL) || (strcmp((char *)res, "OK") != 0)) {
        err = 1;
        printf("SEND ERROR --------\n");
        ATWAIT(CLOCK_SECOND*240, "OK");
        break;
      }
      ATSTR("AT+CREG?\r"); ATWAIT(CLOCK_SECOND*5, "OK");
      ATSTR("AT+CIPSTATUS?\r");  ATWAIT(CLOCK_SECOND*5, "OK");
    }
  }
  ATSTR("AT+CREG?\r"); ATWAIT(CLOCK_SECOND*5, "OK");
  ATSTR("AT+CIPSTATUS?\r");  ATWAIT(CLOCK_SECOND*5, "OK");

  ATSTR("AT+CIPCLOSE\r");  ATWAIT(CLOCK_SECOND*5, "OK"); /* Close connection */
  if (res == NULL) { /* close failed */
    printf("CLOSE FAILED SEND ERROR ----------\n");
    ATWAIT(CLOCK_SECOND*240, "OK");
    goto again;
  }
    
  //ATSTR("AT+CIPSHUT\r");  ATWAIT(CLOCK_SECOND*5, "OK"); /* Shutdown PDP context */

  ATSTR("AT+CREG?\r");ATWAIT(CLOCK_SECOND*5, "OK");
  ATSTR("AT+CIPSTATUS?\r");  ATWAIT(CLOCK_SECOND*5, "OK");
  ATSTR("AT+CCID\r");  ATWAIT(CLOCK_SECOND*5, "OK");

  ATSTR("AT+CEER\r");  ATWAIT(CLOCK_SECOND*5, "OK");
  ATSTR("AT+CSQ\r"); ATWAIT(CLOCK_SECOND*5, "OK");

  ATSTR("AT+CIFSR\r");  ATWAIT(CLOCK_SECOND*5, "OK");

  ATSTR("AT+CGDCONT=?\r"); ATWAIT(CLOCK_SECOND*5, "OK");
  ATSTR("AT+CGDCONT?\r"); ATWAIT(CLOCK_SECOND*5, "OK");  
  ATWAIT(CLOCK_SECOND*20, "KOKO"); /* delay */
  goto moreconnect;

  PROCESS_END();
}
#endif