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
#include <stdarg.h>
#include <stdio.h>
#include "label.h"
#include "senml-formatter.h"

struct senml_formatter formatter;

void
senml_init(struct senml_formatter frmttr)
{
  formatter = frmttr;
}
int
senml_start_pack(char *buf_ptr, int buf_len)
{
  return formatter.start_pack(buf_ptr, buf_len);
}
int
senml_end_pack(char *buf_ptr, int buf_len)
{
  return formatter.end_pack(buf_ptr, buf_len);
}
int
senml_add_record(char *buf_ptr, int buf_len, Label label, ...)
{
  int len = 0;
  va_list args;
  va_start(args, label);
  len += formatter.start_record(buf_ptr, buf_len);
  while(label != END) {
    switch(label) {
    case BASE_NAME:
    case BASE_UNIT:
    case UNIT:
    case NAME:
    case STRING_VALUE:
    case DATA_VALUE:
    {
      char *str = va_arg(args, char *);
      len += formatter.append_str_field(&buf_ptr[len], buf_len - len, label, str);
      break;
    }
    case BASE_TIME:
    case BASE_VALUE:
    case BASE_SUM:
    case VALUE:
    case SUM:
    case TIME:
    case UPDATE_TIME:
    {
      float dbl = va_arg(args, double);
      len += formatter.append_float_field(&buf_ptr[len], buf_len - len, label, dbl);
      break;
    }
    case BOOLEAN_VALUE:
    {
      int b = va_arg(args, int);
      len += formatter.append_bool_field(&buf_ptr[len], buf_len - len, label, b);
      break;
    }
    case BASE_VERSION:
    {
      int i = va_arg(args, int);
      len += formatter.append_int_field(&buf_ptr[len], buf_len - len, label, i);
      break;
    }
    default:
      break;
    }

    label = va_arg(args, Label);
  }
  len += formatter.end_record(&buf_ptr[len], buf_len - len);
  return len;
}
