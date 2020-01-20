/*
 * Copyright (c) 2015, Copyright Robert Olsson / Radio Sensors AB  
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
 * Author  : Robert Olsson robert@radio-sensors.com
 * Created : 2015-11-22
 */

/**
 * \file
 *         A simple LIDAR/RADAR application on RSS2 mote
 */

#include "contiki.h"
#include "sys/etimer.h"
#include <stdio.h>
#include "adc.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include "usart1.h"
/*---------------------------------------------------------------------------*/
PROCESS(hello_xdar_process, "Hello xdar process");
AUTOSTART_PROCESSES(&hello_xdar_process);

#define DEBUG 0

static struct etimer et, ez;

int ext_trigger = 1;

unsigned char
csum(unsigned char *in, unsigned char len) 
{
  int i;
  unsigned char cs = 0;
  for(i=0; i<len; i++) {
    cs += in[i];
  }
  return cs;
}

static void
xdar_config(void)
{
  uint8_t buf[16];
  int i;
  
  /* Reset 42 57 02 00 FF FF FF FF */
  buf[0] = 0x42;
  buf[1] = 0x57;
  buf[2] = 0x02;
  buf[3] = 0x00;
  buf[4] = 0xFF;
  buf[5] = 0xFF;
  buf[6] = 0xFF;
  buf[7] = 0xFF;
  i = usart1_tx(buf, 8);
  clock_delay_usec(10000);
  
  /* Config open 42 57 02 00 00 00 01 02 */
  if(1)  {
    buf[0] = 0x42;
    buf[1] = 0x57;
    buf[2] = 0x02;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x01;
    buf[7] = 0x02;
    i = usart1_tx(buf, 8);
    clock_delay_usec(10000);
  }

  /* Range limit disabled 42 57 02 00 00 00 00 19 */
  if(0)  {
    buf[0] = 0x42;
    buf[1] = 0x57;
    buf[2] = 0x02;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x19;
    i = usart1_tx(buf, 8);
    clock_delay_usec(10000);
  }

  /*  42 57 02 00 00 00 00 40 External trigger */
  if(ext_trigger)  {
    buf[0] = 0x42;
    buf[1] = 0x57;
    buf[2] = 0x02;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x40;
    i = usart1_tx(buf, 8);
    clock_delay_usec(10000);
  }
  /* Config close 42 57 02 01 00 00 00 02,*/
  if(1)  {
    buf[0] = 0x42;
    buf[1] = 0x57;
    buf[2] = 0x02;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x00;
    buf[7] = 0x02;
    i = usart1_tx(buf, 8);
    clock_delay_usec(10000);
  }
}

static void
trigger(void)
{
  uint8_t buf[9];
  int i;

  buf[0] = 0x42;
  buf[1] = 0x57;
  buf[2] = 0x02;
  buf[3] = 0x00;
  buf[4] = 0x00;
  buf[5] = 0x00;
  buf[6] = 0x00;
  buf[7] = 0x41;
  i = usart1_tx(buf, 8);
  clock_delay_usec(10000);
}

static void
read_values(void)
{
  uint8_t crc, mode, buf[9];
  uint16_t signal = 0, dist = 0;
  int i;

  if(ext_trigger) {

#if DEBUG
    printf("Trigger\n");
#endif

    trigger();
  }
  
  for(i=0; i < 9; i++) {
    if( usart1_rx(&buf[i], 1) != 1)
      return;

    if(i == 0 && buf[i] != 0x59)
      return;
    if(i == 1 && buf[i] != 0x59)
      return;
  }
  
  dist = (uint16_t) buf[2] + ((uint16_t) buf[3]) * 256;
  signal = (uint16_t) buf[4] + ((uint16_t) buf[5]) * 256;
  mode = buf[6];
  crc = buf[8];

  if(crc != csum((uint8_t *)&buf, 8))
    return;
    
  printf("Dist=%-5u Signal=%-5u Mode=%-2u ext_trigger=%-d\n",
	 dist, signal, mode, ext_trigger);
}


PROCESS_THREAD(hello_xdar_process, ev, data)
{
  PROCESS_BEGIN();

  SENSORS_ACTIVATE(button_sensor);
  
  leds_init(); 
  leds_on(LEDS_RED);
  leds_on(LEDS_YELLOW);

  usart1_init();
  xdar_config();

  etimer_set(&et, CLOCK_SECOND/100);
  while(1) {
    //PROCESS_YIELD();
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    if (ev == sensors_event && data == &button_sensor) {
      leds_on(LEDS_YELLOW);
      printf("Button pressed\n");
      ext_trigger = ! ext_trigger;
    }

#if DEBUG
    printf("Timer done\n");
#endif

    read_values();
    etimer_reset(&et);
    if(ext_trigger){
      etimer_set(&et, CLOCK_SECOND);
    }
    else {
      etimer_set(&et, CLOCK_SECOND/100);
    }
  }
  PROCESS_END();
}
