/*
 * Copyright (c) 2015, Hasso-Plattner-Institut.
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
 */
/**
 * \addtogroup avr-aes-128
 * @{
 *
 * \file
 *         Implementation of the AES-128 driver for the AVR SoC
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 * \ported to AVR 
 *         Robert Olsson <roolss@kth.se>
 */
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "dev/avr-aes-128.h"
#include "radio/rf230bb/rf230bb.h"
#include <stdint.h>
#include <stdio.h>

/*---------------------------------------------------------------------------*/
#define MODULE_NAME     "avr-aes-128"

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
static uint8_t
avr_crypto_enable(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static uint8_t
avr_crypto_disable(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static uint8_t
enable_crypto(void)
{
  uint8_t enabled = 1; //CRYPTO_IS_ENABLED();
  if(!enabled) {
    avr_crypto_enable();
  }
  return enabled;
}
/*---------------------------------------------------------------------------*/
static void
restore_crypto(uint8_t enabled)
{
  if(!enabled) {
    avr_crypto_disable();
  }
}
/*---------------------------------------------------------------------------*/
static void
set_key(const uint8_t *key)
{
  uint8_t crypto_enabled;

  crypto_enabled = enable_crypto();
  rf230_aes_write_key((unsigned char*) key);
  restore_crypto(crypto_enabled);
}
/*---------------------------------------------------------------------------*/

uint8_t tmp[16];

static void
encrypt(uint8_t *plaintext_and_result)
{
  uint8_t crypto_enabled;
  int8_t res;

  crypto_enabled = enable_crypto();
  res = rf230_aes_encrypt_ecb(NULL, plaintext_and_result, tmp);
  if(!res) {
    PRINTF("HW ecb encrypt error\n");
  }
  memcpy(plaintext_and_result, tmp, 16);
  restore_crypto(crypto_enabled);
}
/*---------------------------------------------------------------------------*/
const struct aes_128_driver avr_aes_128_driver = {
  set_key,
  encrypt
};

/** @} */
