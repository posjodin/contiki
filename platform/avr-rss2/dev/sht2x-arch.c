/*
 * Copyright (c) 2016, Zolertia
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
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "i2c.h"
#include "dev/sht2x/sht2x-arch.h"
/*---------------------------------------------------------------------------*/
void
sht2x_arch_i2c_init(void)
{
  /* Does nothing */
}
/*---------------------------------------------------------------------------*/
void
sht2x_arch_i2c_write_mem(uint8_t addr, uint8_t reg, uint8_t value)
{
  i2c_write_mem(addr, reg, value);
}
/*---------------------------------------------------------------------------*/
void
sht2x_arch_i2c_read_mem(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t bytes)
{
  i2c_read_mem(addr, reg, buf, bytes);
}

uint8_t
sht2x_arch_i2c_start(uint8_t addr)
{
  return i2c_start(addr);
}

void
sht2x_arch_i2c_start_wait(uint8_t addr)
{
  i2c_start_wait(addr);
}

void
sht2x_arch_i2c_stop(void)
{
  return i2c_stop();
}

void
sht2x_arch_i2c_write(uint8_t value)
{
  i2c_write(value);
}

uint8_t
sht2x_arch_i2c_readAck(void)
{
  return i2c_readAck();
}

uint8_t
sht2x_arch_i2c_readNak(void)
{
  return i2c_readNak();
}
