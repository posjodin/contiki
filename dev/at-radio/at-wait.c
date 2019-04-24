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

#include "at-wait.h"

static uint8_t at_numwait;
static uint8_t at_numwait_permanent;

void
atwait_init() {
  at_match_event = process_alloc_event();
  at_numwait = 0;
  at_numwait_permanent = 0;
  atwait_matching = 0;
}

/*
 * FSM protothread to read characters until end of line
 */ 
PT_THREAD(wait_readline_pt(struct pt *pt, struct at_wait *at, int c)) {

  PT_BEGIN(pt);
  while (c != '\n' && c != '\r') {
    PT_YIELD(pt);
  }
  PT_END(pt);
}

/*
 * FSM protothread to read all lines until full buffer is full
 */

PT_THREAD(wait_readlines_pt(struct pt *pt, struct at_wait *at, int c)) {
  static int atpos;

  PT_BEGIN(pt);
  atpos = 0;
  while (1) {
    if (c == '\n' || c == '\r') {
      /* Do nothing???*/;
    }
    else {
      at_line[atpos++] = (char ) c;
      if (atpos == sizeof(at_line)-1)
        break;
      if (c == 0)
        break;
    }
    PT_YIELD(pt);
  }
  /* done -- mark end of string */
  at_line[atpos] = '\0';
  PT_END(pt);
}

size_t at_radio_sendbuf(uint8_t *, size_t);
PT_THREAD(at_sendbuf(struct pt *pt, unsigned char *buf, size_t len)) {
  size_t sendbuf_remain = len;
  PT_BEGIN(pt);
  sendbuf_remain = len;
  while (sendbuf_remain > 0) { 
    while (atwait_matching) {
      PT_YIELD(pt);
    }
    sendbuf_remain -= at_radio_sendbuf(&(buf)[len - sendbuf_remain], sendbuf_remain);
  }
  PT_END(pt);
}

void
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

void
restart_at(struct at_wait *at) {
  at->pos = 0;
}

static void
vstart_atlist(va_list valist) {
  struct at_wait *at;
  at = va_arg(valist, struct at_wait *);
  while (at) {
    printf(" '%s' ", at->str);
    start_at(at);
    at = va_arg(valist, struct at_wait *);
  }
}

void
atwait_start_atlist(uint8_t permanent, ...) {
  va_list valist;
  
  va_start(valist, permanent);
  vstart_atlist(valist);
  if (permanent)
    at_numwait_permanent = at_numwait;
}

static uint8_t recording = 0;
static unsigned int recordpos;
void
atwait_record_on() {
  recording = 1;
  recordpos = 0;
}

void
atwait_record_off() {
  recording = 0;
  at_line[recordpos] = '\0';
}

/*
 * Main loop for input matching. Take one byte at a time, and call each
 * active matching state machine. 
 * If one state mechine returns a non-negative value, it is a complete match. 
 * Then call the corresponding callback protothread with the remaining
 * input data.
 */

PT_THREAD(wait_fsm_pt(struct pt *pt, int c)) {
  static uint8_t i;
  static struct pt subpt;
  static struct at_wait *at;
    
  if (recording) {
    if (recordpos < AT_LINE_SIZE) {
      at_line[recordpos++] = c;
    }
    else {
      printf("at_line full while recording\n");
    }
    at_line[recordpos] = 0;
  }
  PT_BEGIN(pt);

  while (1) {
    atwait_matching = 0;
    for (i = 0; i < at_numwait; i++) {
      at = at_waitlist2[i];
      if (c != at->str[at->pos]) {
        at->pos = 0;
      }
      else if (at->str[++at->pos] == '\0') {
        /* An async (permanent) event? Then mark that it is active right now
         * so that other AT commands can be postponed. Issuing new AT commands 
         * while in this state would garble the input. 
         */
        if (i < at_numwait_permanent)
          atwait_matching = 1;
        if (at->callback != NULL) {
          /* callback function to consume last matching char */

          /* Run callback protothread -- loop until it has finished (PT_ENDED or PT_EXITED) */
          PT_INIT(&subpt);
          while (at->callback(&subpt, at, c) < PT_EXITED) {
            PT_YIELD(pt);
          }
        }
        if (i >= at_numwait_permanent)
          /* We have a match for a dynamic (non-permanent) event. Set
           * at_wait_match to non-NULL to signal the match
           */
          at_wait_match = at;

        atwait_matching = 0;
      }
    }
    /* Yield and wait for next input */
    PT_YIELD(pt);
  }
  PT_END(pt);
}

PT_THREAD(atwait(int lineno, struct pt *pt, struct at_wait **atp, int seconds, ...)) {
//PT_THREAD(atwait(struct pt *pt, process_event_t ev,
//                 process_data_t data, struct at_wait **atp, int seconds, ...)) {
  va_list valist;
  static struct etimer et;
  
  PT_BEGIN(pt);
  at_wait_match = (struct at_wait *) 0;
  
  printf("---start_atlist:%d: ", lineno);
  va_start(valist, seconds);
  vstart_atlist(valist);
  printf("\n"); 

  etimer_set(&et, (seconds)*CLOCK_SECOND);
  while (1) {
    if(etimer_expired(&et)) {
      printf("---timeout:%d\n", lineno);
      *atp = NULL;
      break; 
    } 
    else if (at_wait_match != NULL) {
      etimer_stop(&et); 
      *atp = at_wait_match;
      printf("\n---got:%d '%s'\n", lineno, (*atp)->str); 
      break;
    }      
    PT_YIELD(pt);
  } 

  /* remove dynamic events, and keep permanent */
  at_numwait = at_numwait_permanent;

  PT_END(pt);
}
