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
 */
#include "contiki.h"
#include "contiki-lib.h"
#include <stdio.h>
#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "dev/serial-line.h"
#include <dev/watchdog.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include "sys/etimer.h"
#include "mcu_sleep.h"

static int power_save = 0;

PROCESS(mcu_sleep_process, "Sleeps the mcu");

extern uint32_t mcu_sleep;

//AUTOSTART_PROCESSES(&mcu_sleep_process);

/* Sleep process is just put MCU in sleep so we minimiza MCU impact */
PROCESS_THREAD(mcu_sleep_process, ev, data)
{
  PROCESS_BEGIN();
#ifdef CONTIKI_TARGET_AVR_RSS2  
  while(1) {
    watchdog_periodic();
    if(power_save) 
      set_sleep_mode(SLEEP_MODE_PWR_SAVE);
    else
      set_sleep_mode(SLEEP_MODE_IDLE);
    cli();
    sleep_enable();
    sei();  
    sleep_cpu();
    sleep_disable();
#ifdef MCU_SLEEP_STATS 
    mcu_sleep++;
#endif    
    PROCESS_PAUSE();
  }
#endif  
  PROCESS_END();
}
