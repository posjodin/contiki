/*
* Copyright (c) 2019, Copyright Albert Asratyan, Mandar Joshi
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

/**
 * \file
 *         Contiki-OS M-Bus functionality for platform avr-rss2
 * \authors
 *          Albert Asratyan asratyan@kth.se
 *          Mandar Joshi mandarj@kth.se
 *
 */

void mbus_arch_init(void);
int usart_input_byte(unsigned char c);
size_t mbus_arch_read_byte(uint16_t *buf);
size_t mbus_arch_send_byte(uint8_t *buf);

struct ringbuf16 {
  uint16_t *data;
  uint16_t mask;

  /* XXX these must be 8-bit quantities to avoid race conditions. */
  uint8_t put_ptr, get_ptr;
};

void    ringbuf16_init(struct ringbuf16 *r, uint16_t *a,
		     uint16_t size_power_of_two);

int     ringbuf16_put(struct ringbuf16 *r, uint8_t c);

int     ringbuf16_get(struct ringbuf16 *r);

int     ringbuf16_size(struct ringbuf16 *r);

int     ringbuf16_elements(struct ringbuf16 *r);
