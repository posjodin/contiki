/*
 * Copyright (c) 2017, Robert Olsson 
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
 * Author  : Robert Olsson 
 * roolss@kth.se & robert@radio-sensors.com
 * Created : 2017-04-22
 */

/**
 * \file
 *         A simple AES128 crypto emmgine test for Atmel radios
 */

#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "sys/etimer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rf230bb.h"

#define AES256 1
extern   int __test();

PROCESS(aes_process, "AES crypto process");
PROCESS(aes_hw_process, "AES HW crypto process");
AUTOSTART_PROCESSES(&aes_process);

PROCESS_THREAD(aes_process, ev, data)
{
  PROCESS_BEGIN();
  /* AES HW engine on */
  NETSTACK_RADIO.on();

  __test();

  process_start(&aes_hw_process, NULL);
  PROCESS_END();
}

unsigned char aes_p[128];
unsigned char aes_c[128];
unsigned char aes_s[128];
unsigned char tmp[16];
uint8_t i, i1;
int res;

// 128bit key
 uint8_t key[16] =        { (uint8_t) 0x2b, (uint8_t) 0x7e, (uint8_t) 0x15, (uint8_t) 0x16, (uint8_t) 0x28, (uint8_t) 0xae, (uint8_t) 0xd2, (uint8_t) 0xa6, (uint8_t) 0xab, (uint8_t) 0xf7, (uint8_t) 0x15, (uint8_t) 0x88, (uint8_t) 0x09, (uint8_t) 0xcf, (uint8_t) 0x4f, (uint8_t) 0x3c };
    // 512bit text
    uint8_t plain_text[64] = { (uint8_t) 0x6b, (uint8_t) 0xc1, (uint8_t) 0xbe, (uint8_t) 0xe2, (uint8_t) 0x2e, (uint8_t) 0x40, (uint8_t) 0x9f, (uint8_t) 0x96, (uint8_t) 0xe9, (uint8_t) 0x3d, (uint8_t) 0x7e, (uint8_t) 0x11, (uint8_t) 0x73, (uint8_t) 0x93, (uint8_t) 0x17, (uint8_t) 0x2a,
                               (uint8_t) 0xae, (uint8_t) 0x2d, (uint8_t) 0x8a, (uint8_t) 0x57, (uint8_t) 0x1e, (uint8_t) 0x03, (uint8_t) 0xac, (uint8_t) 0x9c, (uint8_t) 0x9e, (uint8_t) 0xb7, (uint8_t) 0x6f, (uint8_t) 0xac, (uint8_t) 0x45, (uint8_t) 0xaf, (uint8_t) 0x8e, (uint8_t) 0x51,
                               (uint8_t) 0x30, (uint8_t) 0xc8, (uint8_t) 0x1c, (uint8_t) 0x46, (uint8_t) 0xa3, (uint8_t) 0x5c, (uint8_t) 0xe4, (uint8_t) 0x11, (uint8_t) 0xe5, (uint8_t) 0xfb, (uint8_t) 0xc1, (uint8_t) 0x19, (uint8_t) 0x1a, (uint8_t) 0x0a, (uint8_t) 0x52, (uint8_t) 0xef,
                               (uint8_t) 0xf6, (uint8_t) 0x9f, (uint8_t) 0x24, (uint8_t) 0x45, (uint8_t) 0xdf, (uint8_t) 0x4f, (uint8_t) 0x9b, (uint8_t) 0x17, (uint8_t) 0xad, (uint8_t) 0x2b, (uint8_t) 0x41, (uint8_t) 0x7b, (uint8_t) 0xe6, (uint8_t) 0x6c, (uint8_t) 0x37, (uint8_t) 0x10 };


PROCESS_THREAD(aes_hw_process, ev, data)
{
  PROCESS_BEGIN();

  /* AES engine on */
  NETSTACK_RADIO.on();

  //strcpy((char *)aes_s, "Teststring______");
  memset(aes_s, 0, sizeof(aes_s));

  printf(" AES-128 HW EBC Crypted\n");
  
#if PRINT_INIT_VECTOR
  for(i = 0; i < 16; i++) {
    printf("%02x", aes_s[i]);
  }
  printf(" Uncrypted \n");
#endif

  printf("ciphertext:\n");
  for (i1 = 0; i1 < 4; ++i1) {
    res = rf230_aes_encrypt_ebc(key, plain_text + (i1 * 16), aes_c);
    if(!res) {
      printf("ERR encryption\n");
      exit(0);
    }
    for(i = 0; i < 16; i++) {
      printf("%02x", aes_c[i]);
    }
    printf("\n");
  }

  PROCESS_END();
}
