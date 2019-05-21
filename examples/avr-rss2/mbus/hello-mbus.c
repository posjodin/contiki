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
 *         A simple mbus application on RSS2 mote
 */

#include "contiki.h"
#include "sys/etimer.h"
#include <stdio.h>
#include <string.h>
#include "adc.h"
#include "dev/leds.h"
#include "usart1.h"
#include "mbus_library/mbus.h"
#include "mbus_library/functions/mbus-serial-scan.h"

/*---------------------------------------------------------------------------*/
PROCESS(hello_mbus_process, "Hello mbus process");
AUTOSTART_PROCESSES(&hello_mbus_process);

static struct etimer et;

void
mbus_local_scan(void)
{
  uint8_t frame[5] = {0x10, 0x40, 0x43, 0x83, 0x16};

  int i = 0;
  for (i = 0; i < 5; i++) {
    usart1_tx(&frame[i], 1);
  }
}



/*static void
read_values(void)
{
  uint8_t buf[1];
  memset(buf, 0, sizeof(buf));

  int check = usart1_rx(buf, 1);
  if (check == 1) {
      printf("buf = %x\n", (uint8_t) buf[0]);
  }
}*/



/*---------------------------------------------------------------------------*/
PROCESS_THREAD(hello_mbus_process, ev, data)
{
  PROCESS_BEGIN();

  usart1_init();

  leds_init();
  leds_on(LEDS_RED);
  leds_on(LEDS_YELLOW);

  /*
   * Delay 1/2 sec
   */

  etimer_set(&et, CLOCK_SECOND);
  while(1) {
    PROCESS_YIELD();

    //mbus_local_scan();
    mbus_scan();
    etimer_reset(&et);
    //read_values();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
