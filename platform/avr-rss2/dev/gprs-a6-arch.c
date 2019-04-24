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

#include "contiki.h"
#include "sys/etimer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uip.h"
#include "i2c.h"
#include "dev/leds.h"
#include "dev/sc16is/sc16is.h"
#include "sc16is-common.h"
#include "at-wait.h"

PT_THREAD(gprs_a6_module_init(struct pt *pt, uint32_t baud)) {
  uint8_t s;
  static struct etimer et;

  PT_BEGIN(pt);

  if(i2c_probed & I2C_SC16IS) {

    sc16is_init();
    //sc16is_gpio_set_dir(G_RESET | G_PWR | G_U_5V_CTRL | G_SET | G_LED_YELLOW | G_LED_RED | G_GPIO7);
    sc16is_gpio_set_dir(G_RESET | G_PWR | G_U_5V_CTRL | G_LED_YELLOW | G_LED_RED | G_GPIO7);
    sc16is_gpio_set((G_LED_RED|G_LED_YELLOW));
    sc16is_uart_set_speed(baud);
    /* sc16is_arch_i2c_write_mem(I2C_SC16IS_ADDR, SC16IS_FCR, SC16IS_FCR_FIFO_BIT); */
    sc16is_tx((uint8_t *)"AT", sizeof("AT"-1));

  }

  set_board_5v(0); /* Power cycle the board */
  etimer_set(&et, 2*CLOCK_SECOND);
  while (!etimer_expired(&et)) {
    PT_YIELD(pt);
  }
  set_board_5v(1);
  etimer_set(&et, 2*CLOCK_SECOND);
  while (!etimer_expired(&et)) {
    PT_YIELD(pt);
  }
  s = sc16is_gpio_get();
  printf("LOOP GPIO=0x%02x\n", sc16is_gpio_get());
  clr_bit(&s, G_PWR);
  set_bit(&s, G_RESET);

  set_bit(&s, G_LED_RED); /*OFF */
  set_bit(&s, G_LED_YELLOW);
  sc16is_gpio_set(s);
  etimer_set(&et, 2*CLOCK_SECOND);
  while (!etimer_expired(&et)) {
    PT_YIELD(pt);
  }
  clr_bit(&s, G_RESET);
  sc16is_gpio_set(s);
  /* start */
  etimer_set(&et, 2*CLOCK_SECOND);
  while (!etimer_expired(&et)) {
    PT_YIELD(pt);
  }

  s = sc16is_gpio_get();
  set_bit(&s, G_PWR);
  sc16is_gpio_set(s);


  PT_END(pt);

}

size_t
gprs_a6_rx(uint8_t *buf, size_t len) {
  if( i2c_probed & I2C_SC16IS )
    return sc16is_rx(buf, len);
  else
    return 0;
}

size_t
gprs_a6_tx(uint8_t *buf, size_t len) {
  return sc16is_tx(buf, len);
}
