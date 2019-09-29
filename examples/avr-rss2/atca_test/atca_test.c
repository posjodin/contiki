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
 *         A simple application showing sensor reading on RSS2 mote
 */

#include "contiki.h"
#include "sys/etimer.h"
#include <stdio.h>
#include <string.h>
#include "adc.h"
#include "i2c.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include "atca_command.h"
#include "atca_devtypes.h"

PROCESS(read_atca_process, "Read atca process");
AUTOSTART_PROCESSES(&read_atca_process);

static struct etimer et;

//ATCA_STATUS atca_execute_command(ATCAPacket* packet, ATCADevice device);
ATCA_STATUS atca_execute_command(ATCAPacket* packet, uint8_t addr);

ATCAPacket packet;
ATCACommand ca_cmd;
ATCA_STATUS atca_status;
ATCADeviceType device_type = ATECC608A;


static void wake(uint8_t addr)
{
  int i;
  
  i2c_start(addr | I2C_WRITE);

  /* 60 us */
  for(i=0; i < 20; i++) {
    i2c_write(0);
  }
  i2c_stop();

  /* 1500 us */
  clock_delay_usec(1500);
}

ATCA_STATUS atca_execute_command(ATCAPacket* p, uint8_t addr)
{
  uint8_t i = 0;
  uint8_t bytes;
  uint8_t buf[36];

  wake(addr);
  
  i2c_start(addr | I2C_WRITE);
  i2c_write(0x03);
  i2c_write(p->txsize);
  i2c_write(p->opcode);
  i2c_write(p->param1);
  i2c_write(p->param2 & 0xFF);
  i2c_write((p->param2)>>8 & 0xFF);
  for(i=0; i < p->txsize-5; i++) {
    i2c_write(p->data[i]);
  }
    for(i = 0; i < 10; i++) {
    clock_delay_usec(1000);
  }

  bytes = 2;
    
  i2c_start(addr | I2C_READ);
  for(i = 0; i < bytes; i++) {
    if(i == bytes - 1) {
      buf[i] = i2c_readNak();
    } else {
      buf[i] = i2c_readAck();
    }
    if(i == 0)
      bytes = buf[0];
  }
  i2c_stop();

  atca_status = atCheckCrc(buf);

  if(atca_status == ATCA_SUCCESS) {
    for(i=0; i < bytes; i++)
      printf("%02x", buf[i]);
  }
  return ATCA_SUCCESS;
}  

void dump(ATCAPacket *p)
{
  int i;
  printf("_reserved=%d\n", p->_reserved);
  printf("txsize=%d\n", p->txsize);
  printf("opcode=0x%02x\n", p->opcode);
  printf("param1=0x%02x\n", p->param1);
  printf("param2=0x%04x\n", p->param2);
  for(i=0; i < p->txsize-5; i++) {
    printf("data[%-d]=0x%02x\n", i, p->data[i]);
  }
  //data[192]
}  

static void
read_crypt(void)
{
  if( I2C_ATECC608A ) {

    memset(&packet, 0x00, sizeof(packet));
    // build an info command

    //packet.param1 = INFO_MODE_REVISION;
    //packet.param2 = 0;
    //atca_status = atInfo(ca_cmd, &packet);
    //atca_status = atCounter(ca_cmd, &packet);

    //packet.param2 = SELFTEST_MODE_ALL;
    //atca_status = atSelfTest(ca_cmd, &packet);
    atca_status = atRandom(ca_cmd, &packet);
     if(atca_status != ATCA_SUCCESS)
       printf("ERROR\n");
     dump(&packet);

     atca_execute_command(&packet, I2C_ATECC608A_ADDR);
  }
  printf("\n\n");
}

PROCESS_THREAD(read_atca_process, ev, data)
{
  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);
  
  leds_init(); 
  leds_on(LEDS_RED);
  leds_on(LEDS_YELLOW);

  /* 
   * Delay 5 sec 
   * Gives a chance to trigger some pulses
   */

  etimer_set(&et, CLOCK_SECOND * 5);
  
  while(1) {
    PROCESS_YIELD();
    //PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    if (ev == sensors_event && data == &button_sensor) {
      leds_on(LEDS_YELLOW);
      printf("Button pressed\n");
    }

    read_crypt();
    etimer_reset(&et);
  }

  PROCESS_END();
}
