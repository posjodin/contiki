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
 * Created : 2018-04-08
 */

/**
 * \file
 *         Commons for sc16is I2C UART & GPIO
 *         Example uses avr-rss2 platform
 *
 */

#include "contiki.h"
#include "sys/etimer.h"
#include <stdio.h>
#include <string.h>
#include "i2c.h"
#include "dev/leds.h"
#include "dev/sc16is/sc16is.h"
#include "sc16is-common.h"

void
toggle_bit(uint8_t *s, uint8_t bit)
{
  if(*s & bit) {
    *s &= ~bit;
  } else {
    *s |= bit;
  }
}
void
set_bit(uint8_t *s, uint8_t bit)
{
  *s |= bit;
}
void
clr_bit(uint8_t *s, uint8_t bit)
{
  *s &= ~bit;
}
void
set_board_5v(uint8_t on)
{
  uint8_t s = sc16is_gpio_get();
  uint8_t d = sc16is_gpio_get_dir();

  printf("set_5V on=%d\n", on);
  printf("GPIO=0x%02x\n", s);
  printf("GPIO_DIR=0x%02x\n", d);

  if(on) {
    set_bit(&d, G_U_5V_CTRL);
    clr_bit(&s, G_U_5V_CTRL);
  } else {
    clr_bit(&d, G_U_5V_CTRL);
  }
  sc16is_gpio_set_dir(d);
  sc16is_gpio_set(s);
}
