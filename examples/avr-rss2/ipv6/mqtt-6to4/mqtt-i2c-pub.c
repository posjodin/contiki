/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 */

/**
 * \file
 *         i2c core functions
 * \author
 *         Robert Olsson <robert@radio-sensors.com>
 */

#include "i2c.h"
#include <stdio.h>
#include <string.h>

#define PUTFMT(...) { \
		len = snprintf(buf_ptr, remaining, __VA_ARGS__);	\
		if (len < 0 || len >= remaining) { \
			printf("Line %d: Buffer too short. Have %d, need %d + \\0\n", __LINE__, remaining, len); \
			return bufsize + len; \
		} \
		remaining -= len; \
		buf_ptr += len; \
	}

int
mqtt_i2c_pub(char *buf, int bufsize)  {
  int remaining = bufsize;
  char *buf_ptr = buf;
  int len;
  char *delim = "";

  PUTFMT("{\"n\":\"i2c\", \"vj\":[");

  if (i2c_probed & I2C_AT24MAC) {
    PUTFMT("%s\"AT24MAC\"", delim);
    delim = ",";
  }

  if (i2c_probed & I2C_SHT2X) {
    PUTFMT("%s\"SHT2X\"", delim);
    delim = ",";
  }
  if (i2c_probed & I2C_CO2SA) {
  
  PUTFMT("%s\"CO2SA\"", delim);
  delim = ",";
}
  if (i2c_probed & I2C_BME280) {
    PUTFMT("%s\"BME280\"", delim);
    delim = ",";
  }
  if (i2c_probed & I2C_PMS5003) {
  PUTFMT("%s\"PMS5003\"", delim);
  delim = ",";
}
  if (i2c_probed & I2C_SC16IS) {
  PUTFMT("%s\"SC16IS\"", delim);
  delim = ",";
}
  PUTFMT("]}");
  return buf_ptr - buf;
}
  
