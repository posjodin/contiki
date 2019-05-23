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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "dev/rs232.h"
#include "ringbuf-mbus.h"

/*---------------------------------------------------------------------------*/

#define BUFSIZE 256
struct ringbuf16 rxbuf;
uint16_t rxbuf_data[BUFSIZE];
// struct ringbuf16 rxbuf2;
// uint8_t rxbuf_data2[16];
// int rxbuf2counter = 0;
uint8_t usart = RS232_PORT_1;


/*---------------------------------------------------------------------------*/

static int
usart_input_byte(unsigned char c)
{
  // if (kek < 128) {
  //   ringbuf_put(&rxbuf, c);
  // } else {
  //   if (kek < 144) {
  //     ringbuf_put(&rxbuf2, c);
  //   } else {
  //     ringbuf_put(&rxbuf, c);
  //     kek = 0;
  //   }
  // }


  ringbuf16_put(&rxbuf, c);
  //kek++;
  return 1;
}

// static int
// usart_input_byte(unsigned char c)
// {
//   static uint8_t overflow = 0; /* Buffer overflow: ignore until END */
//
//   if(!overflow) {
//     /* Add character */
//     kek++;
//     if(ringbuf_put(&rxbuf, c) == 0) {
//       printf("%d", kek);
//
//       /* Buffer overflow: ignore the rest of the line */
//       ringbuf_put(&rxbuf2, c);
//       rxbuf2counter++;
//       overflow = 1;
//     }
//   } else {
//     //printf("OVERFLOW!\n");
//     /* Buffer overflowed:
//      * Only (try to) add terminator characters, otherwise skip */
//      if (rxbuf2counter == 16) {
//        overflow = 0;
//        rxbuf2counter = 0;
//        ringbuf_put(&rxbuf, c);
//        return 1;
//      }
//
//      if(ringbuf_put(&rxbuf2, c) == 0) {
//
//        ringbuf_put(&rxbuf, c);
//        overflow = 0;
//        rxbuf2counter = 0;
//      }
//      else {
//        rxbuf2counter++;
//      }
//
//     // if(c == END && ringbuf_put(&rxbuf, c) != 0) {
//     //   overflow = 0;
//     // }
//     }
//   return 1;
// }

/*---------------------------------------------------------------------------*/
void
usart1_init() {
  ringbuf16_init(&rxbuf, rxbuf_data, sizeof(rxbuf_data));
  //ringbuf16_init(&rxbuf2, rxbuf_data2, sizeof(rxbuf_data2));
  rs232_set_input(usart, usart_input_byte);
}

/*---------------------------------------------------------------------------*/
size_t
usart1_rx(uint8_t *buf, size_t len) {
  int c;

  c = ringbuf16_get(&rxbuf);

  // if (c == -1) {
  //   c = ringbuf16_get(&rxbuf2);
  // }

  if (c == -1)
    return 0;
  *buf = (uint8_t) c;
  return 1;
}
/*---------------------------------------------------------------------------*/
size_t
usart1_tx(uint8_t *buf, size_t len) {
  uint8_t p = *buf;
  rs232_send(usart, p);
  return 1;
}
/*---------------------------------------------------------------------------*/
