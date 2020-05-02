/*
 * Copyright (c) 2019, 
 * Anton Bothin,
 * Erik Flink,
 * Nelly Friman,
 * Jacob Klasmark,
 * Valter Lundegårdh, 
 * Isak Olsson,
 * Andreas Sjödin,
 * Carina Wickström.
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "label.h"
#include "senml-formatter.h"

/* translated SenML labels to CBOR labels according to RFC8428 page 19 */
static const unsigned char label_cbor[] = {
  0x21,
  0x22,
  0x23,
  0x24,
  0x25,
  0x20,
  0x00,
  0x01,
  0x02,
  0x03,
  0x04,
  0x08,
  0x05,
  0x06,
  0x07
};
/* 0x9F stands for start of indefinite length array */
int
start_pack_cbor(char *buffer, int buffer_len)
{
  return snprintf(buffer, buffer_len, "%c", 0x9F);
}
/* 0xFF stands for "break" indefinite length array */
int
end_pack_cbor(char *buffer, int buffer_len)
{
  return snprintf(buffer, buffer_len, "%c", 0xFF);
}
/* 0xBF stands for start indefinite length map */
int
start_record_cbor(char *buffer, int buffer_len)
{
  return snprintf(buffer, buffer_len, "%c", 0xBF);
}
/* 0xFF stands for "break" indefinite length map */
int
end_record_cbor(char *buffer, int buffer_len)
{
  return snprintf(buffer, buffer_len, "%c", 0xFF);
}
/* max int value or length of string is 65535. Can be extended if needed. */
static int
data_type_cbor_convert(char *buffer, int buffer_len, unsigned char type, int value)
{
  if(value < 24) {
    return snprintf(buffer, buffer_len, "%c", type | (unsigned char)value);
  } else if(value < 256) {
    return snprintf(buffer, buffer_len, "%c%c", type | 0x18, (unsigned char)value);
  } else if(value < 65536) {
    return snprintf(buffer, buffer_len, "%c%c%c", type | 0x19, (uint8_t)(value >> 8) & 0xFF, (uint8_t)value & 0xFF);
  }
  return 0;
}
int
append_str_field_cbor(char *buffer, int buffer_len, Label label, char *value)
{
  uint8_t len = 0;

  len += snprintf(&buffer[len], buffer_len - len, "%c", label_cbor[label]);

  int number_of_chars = 0;
  while(*value != '\0') {
    number_of_chars++;
    value++;
  }
  /* 0x60 for text */
  len += data_type_cbor_convert(&buffer[len], buffer_len - len, 0x60, number_of_chars);

  value = value - number_of_chars;
  len += snprintf(&buffer[len], buffer_len - len, "%s", value);

  return len;
}
int
append_int_field_cbor(char *buffer, int buffer_len, Label label, int value)
{
  uint8_t len = 0;

  len += snprintf(&buffer[len], buffer_len - len, "%c", label_cbor[label]);
  /* 0x00 for positive/unsigned int */
  len += data_type_cbor_convert(&buffer[len], buffer_len - len, 0x00, value);

  return len;
}
int
append_dbl_field_cbor(char *buffer, int buffer_len, Label label, float value)
{
  uint8_t len = 0;

  /* 0xFA for start of float value */
  len += snprintf(&buffer[len], buffer_len - len, "%c%c", label_cbor[label], 0xFA);

  union {
    float f_val;
    uint32_t u_val;
  } u32;

  u32.f_val = value;

  int i;
  for(i = 0; i < 4; i++) {
    len += snprintf(&buffer[len], buffer_len - len, "%c", (uint8_t)((u32.u_val >> 8 * (3 - i)) & 0xFF));
  }
  return len;
}
int
append_bool_field_cbor(char *buffer, int buffer_len, Label label, int value)
{
  if(value == 0) {
    return snprintf(buffer, buffer_len, "%c%c", label_cbor[label], 0xF4);
  } else {
    return snprintf(buffer, buffer_len, "%c%c", label_cbor[label], 0xF5);
  }
}
const struct senml_formatter senml_cbor_formatter = {
  start_record_cbor,
  end_record_cbor,
  start_pack_cbor,
  end_pack_cbor,
  append_str_field_cbor,
  append_dbl_field_cbor,
  append_bool_field_cbor,
  append_int_field_cbor
};
