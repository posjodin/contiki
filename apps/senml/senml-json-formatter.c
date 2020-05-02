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
#include "label.h"
#include <stdio.h>
#include "senml-formatter.h"

static const char *label_strings[] = {
  "bn",
  "bt",
  "bu",
  "bv",
  "bs",
  "bver",
  "n",
  "u",
  "v",
  "vs",
  "vb",
  "vd",
  "s",
  "t",
  "ut"
};

int
start_pack_json(char *buf_ptr, int remaining)
{
  return snprintf(buf_ptr, remaining, "[");
}
int
end_pack_json(char *buf_ptr, int remaining)
{
  buf_ptr -= sizeof(char);

  if(buf_ptr[0] == ',') {
    return snprintf(buf_ptr, remaining, "]") - 1;
  } else {
    buf_ptr += sizeof(char);
    return snprintf(buf_ptr, remaining, "]");
  }
}
int
start_record_json(char *buf_ptr, int remaining)
{
  return snprintf(buf_ptr, remaining, "{");
}
int
end_record_json(char *buf_ptr, int remaining)
{
  buf_ptr -= sizeof(char);
  return snprintf(buf_ptr, remaining, "},") - 1;
}
int
append_str_field_json(char *buf_ptr, int remaining, Label label, char *str)
{
  return snprintf(buf_ptr, remaining, "\"%s\":\"%s\",", label_strings[label], str);
}
int
append_dbl_field_json(char *buf_ptr, int remaining, Label label, float flt)
{
  return snprintf(buf_ptr, remaining, "\"%s\":%g,", label_strings[label], flt);
}
int
append_bool_field_json(char *buf_ptr, int remaining, Label label, int b)
{
  if(b) {

    return snprintf(buf_ptr, remaining, "\"%s\":true,", label_strings[label]);
  } else {
    return snprintf(buf_ptr, remaining, "\"%s\":false,", label_strings[label]);
  }
}
int
append_int_field_json(char *buf_ptr, int remaining, Label label, int i)
{
  return snprintf(buf_ptr, remaining, "\"%s\":%d,", label_strings[label], i);
}
const struct senml_formatter senml_json_formatter = {
  start_record_json,
  end_record_json,
  start_pack_json,
  end_pack_json,
  append_str_field_json,
  append_dbl_field_json,
  append_bool_field_json,
  append_int_field_json
};