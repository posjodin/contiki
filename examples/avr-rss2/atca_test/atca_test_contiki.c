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
#include "atca_device.h"
#include "atca_devtypes.h"
#include "atca_i2c.h"
#include "atca_basic.h"


PROCESS(read_atca_process, "Read atca process");
AUTOSTART_PROCESSES(&read_atca_process);

static struct etimer et;

//ATCA_STATUS atca_execute_command(ATCAPacket* packet, ATCADevice device);
ATCA_STATUS atca_execute_command(ATCAPacket* packet, uint8_t addr);

ATCAPacket packet;
ATCACommand ca_cmd;
ATCA_STATUS atca_status;
ATCADevice _gDevice;

uint8_t id[9];
uint8_t buf[32];

extern uint8_t test_ecc608_configdata[];

void
init_once(void)
{
  //test_assert_config_is_unlocked();
    //status = atcab_write_config_zone(test_ecc608_configdata);
    atca_status = atcab_write_bytes_zone(ATCA_ZONE_CONFIG, 0, 20, &test_ecc608_configdata[20], 128 - 20);
    if (atca_status != ATCA_SUCCESS)
       printf("ID ERROR\n");
}

void
lock(void)
{  
  atca_status = atcab_lock(LOCK_ZONE_NO_CRC | LOCK_ZONE_CONFIG, 0);
  if (atca_status != ATCA_SUCCESS)
    printf("Lock ERROR\n");
}


void
read_id(void)
{
  int i;
  atca_status = atcab_read_serial_number(id);
  if(atca_status != ATCA_SUCCESS)
       printf("ID ERROR\n");

  printf("ID=");
  for(i=0; i < sizeof(id); i++)
    printf("%02x", id[i]);

  printf("\n");
}

void
read_zone(void)
{
  int i, j;
  int is_locked;

  for(i=0; i < 2; i++) {
    atca_status = atcab_is_locked(i, &is_locked);
    if(atca_status != ATCA_SUCCESS)
      printf("SLOT READ ERROR\n");
    else

      if(i == LOCK_ZONE_CONFIG) 
	printf("Config Zone is_lock=%0d\n", is_locked);
      if(i == LOCK_ZONE_DATA) 
	printf("Config Zone is_lock=%0d\n", is_locked);
  }

  for(i=0; i < 16; i++) {
    atca_status = atcab_is_slot_locked(i, &is_locked);
    if(atca_status != ATCA_SUCCESS)
      printf("SLOT READ ERROR\n");
    else
      printf("slot%0d is_lock=%0d\n", i, is_locked);
  }
  
  for(j=0; j < 4; j++) {
  //atca_status = atcab_read_zone(uint8_t zone, uint16_t slot, uint8_t block, uint8_t offset, uint8_t *data, uint8_t len)
    atca_status = atcab_read_zone(ATCA_ZONE_CONFIG, 0, j, 0, buf, 32);
    if(atca_status != ATCA_SUCCESS)
      printf("READ ERROR\n");
    
    printf("\n%3d: ", j*32);
  
    for(i=0; i < sizeof(buf); i++) {
      if(! (i % 16) )
	printf("\n");
      printf("%02x ", buf[i]);
    }
  }
}

static void
test_command(void)
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
    packet.param1 = 0x04; // a random private key is generated and stored in slot keyID
    //packet.param2 = GENKEY_PRIVATE_TO_TEMPKEY;
    packet.param2 = 0; // keyID;
    atca_status = atGenKey(ca_cmd, &packet);
    //atca_status = atRandom(ca_cmd, &packet);

    //atca_status = atLock(ca_cmd, &packet);
    if(atca_status != ATCA_SUCCESS)
       printf("ERROR\n");
     atca_command_dump(&packet);
     atca_status = atca_execute_command(&packet, I2C_ATECC608A_ADDR);
     if(atca_status != ATCA_SUCCESS)
       printf("ERROR comnmand status=%02X\n", atca_status);

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
  //init_once();
  //lock();
  
  while(1) {
    PROCESS_YIELD();
    //PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    if (ev == sensors_event && data == &button_sensor) {
      leds_on(LEDS_YELLOW);
      printf("Button pressed\n");
      
    }

    //read_id();
    //read_zone();
    //test_command();
    atca_cmd_unit_test_genkey();
    atca_cmd_basic_test_genkey();

    printf("PUBLIC KEY\n");
    atca_cmd_basic_test_get_pubkey();
    etimer_reset(&et);
  }

  PROCESS_END();
}
