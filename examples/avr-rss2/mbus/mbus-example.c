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
 * Created : 2019-05-30
 */

/**
 * \file
 *         A simple mbus application on RSS2 mote
 * \authors
 *          Albert Asratyan asratyan@kth.se
 *          Mandar Joshi mandarj@kth.se
 */

#include "contiki.h"
#include "sys/etimer.h"
#include <stdio.h>
#include <string.h>
#include "adc.h"
#include "dev/leds.h"
#include "dev/mbus/mbus-arch.h"
// #include "dev/mbus-arch.h"
#include "dev/watchdog.h"
#include "mbus.h"
#include "mbus-supported-devices.h"


/*---------------------------------------------------------------------------*/
PROCESS(mbus_process, "mbus process");
AUTOSTART_PROCESSES(&mbus_process);

static struct etimer et;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mbus_process, ev, data)
{
  PROCESS_BEGIN();

  // Initialize the UART1 port.
  mbus_arch_init();

  leds_init();
  leds_on(LEDS_RED);
  leds_on(LEDS_YELLOW);

  /*
   * Delay 5 sec
   */

  etimer_set(&et, CLOCK_SECOND*5);
  while(1) {
    PROCESS_YIELD();

    leds_toggle(LEDS_YELLOW);

    /* Comment in/out all of the code in each of the section to test it. */

    //------------

    mbus_scan_primary_at_address(67);

    //------------

    //mbus_scan_primary_all();

    //-------------

    /*
    //create an empty array that will hold the frame data
    uint16_t data[MBUS_FRAME_SIZE_KAMSTRUP_2101];
    memset(data, 0, sizeof(data));
    //request the data with the empty array and expected frame size as arguments
    mbus_request_data_at_primary_address(67, data, MBUS_FRAME_SIZE_KAMSTRUP_2101);
    printf("\n");

    //print out the received hex data
    for (int i = 0; i < MBUS_FRAME_SIZE_KAMSTRUP_2101; i++) {
      printf("%0X ", data[i]);
      if ((i+1) % 32 == 0) {
        printf("\n");
      }
    }
    printf("\n");

    watchdog_periodic();

    //create a new array that will hold parsed data, specific to the flowIQ 2101
    char text_data[37][32];
    for (int i = 0; i < 37; i++) {
      memset(text_data[i], 0, sizeof(text_data[i]));
    }

    //use the empty array as an argument that will be filled by the function
    mbus_parse_data_kamstrup_2101(data, text_data);

    //print out the parsed data
    for (int i = 0; i < 37; i++)
    {
      printf("%s\n", text_data[i]);
    }
    */

    //-------------

    /*
    mbus_switch_baudrate(67, 2400);
    */

    //-------------

    /*
    mbus_scan_primary_at_address(67);
    mbus_set_primary_address(67, 100);
    mbus_scan_primary_at_address(67);
    mbus_scan_primary_at_address(100);
    mbus_set_primary_address(100, 67);
    mbus_scan_primary_at_address(67);
    */

    //-------------


    etimer_reset(&et);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
