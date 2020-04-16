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

#include "contiki.h"
#include "unit-test.h"
#include <string.h>
#include <stdio.h>
#include "senml-decode.h"
#include "jsonparse.h"

UNIT_TEST_REGISTER(read_label_null, "read null label");
UNIT_TEST_REGISTER(read_value_null, "read null value");
UNIT_TEST_REGISTER(read_one_label, "read one label");
UNIT_TEST_REGISTER(read_one_value_str, "read one str value");
UNIT_TEST_REGISTER(read_one_value_int, "read one int value");
UNIT_TEST_REGISTER(add_msg_and_read_label, "Add msg and read label");
UNIT_TEST_REGISTER(add_msg_and_read_value_str, "Add msg and read str value");
UNIT_TEST_REGISTER(add_msg_and_read_value_int, "Add msg and read int value");
UNIT_TEST_REGISTER(read_add_read_label, "Read, add, read label");
UNIT_TEST_REGISTER(read_add_read_value_str, "read, add, read str value");
UNIT_TEST_REGISTER(read_add_read_value_int," read, add, read int value");

int strcmpn(char * str1, char * str2, int n) {
  int i;
  for (i = 0; i < n; i++)
  {
    if(str1[i] - str2[i] != 0) return str1[i] - str2[i];
  }
  return 0;
}
UNIT_TEST(read_label_null){
  UNIT_TEST_BEGIN();
  init_json_decoder("[{}]");
  struct pair lv;
  read_next_token(&lv);
  UNIT_TEST_ASSERT(lv.label == NULL);
  UNIT_TEST_END();
}

UNIT_TEST(read_value_null){
  UNIT_TEST_BEGIN();
  init_json_decoder("[{}]");
  struct pair lv;
  read_next_token(&lv);
  UNIT_TEST_ASSERT(lv.value == NULL);
  UNIT_TEST_END();
}

UNIT_TEST(read_one_label){
  UNIT_TEST_BEGIN();
  init_json_decoder("[{\"bn\": \"urn:mac:testID\"}]");
  struct pair lv;
  read_next_token(&lv);
  UNIT_TEST_ASSERT(strcmp(lv.label, "bn") == 0);
  UNIT_TEST_END();
}

UNIT_TEST(read_one_value_str){
  UNIT_TEST_BEGIN();
  init_json_decoder("[{\"bn\": \"urn:mac:testID\"}]");
  struct pair lv;
  read_next_token(&lv);
  UNIT_TEST_ASSERT(strcmp(lv.value, "urn:mac:testID") == 0);
  UNIT_TEST_END();
}

UNIT_TEST(read_one_value_int){
  UNIT_TEST_BEGIN();
  init_json_decoder("[{\"v\": 10}]");
  struct pair lv;
  read_next_token(&lv);
  UNIT_TEST_ASSERT(strcmp(lv.value, "10") == 0);
  UNIT_TEST_END();
}

UNIT_TEST(add_msg_and_read_label){
  UNIT_TEST_BEGIN();
  init_json_decoder("[{\"bn\": \"urn:mac:testID\"},");
  add_new_msg("{\"u\": \"dB\"},");
  struct pair lv;
  read_next_token(&lv);
  read_next_token(&lv);
  UNIT_TEST_ASSERT(strcmp(lv.label, "u") == 0);
  UNIT_TEST_END();
}

UNIT_TEST(add_msg_and_read_value_str){
  UNIT_TEST_BEGIN();
  init_json_decoder("[{\"bn\": \"urn:mac:testID\"},");
  add_new_msg("{\"u\": \"dB\"},");
  struct pair lv;
  read_next_token(&lv);
  read_next_token(&lv);

  UNIT_TEST_ASSERT(strcmp(lv.value, "dB") == 0);
  UNIT_TEST_END();
}

UNIT_TEST(add_msg_and_read_value_int){
  UNIT_TEST_BEGIN();
  init_json_decoder("[{\"v\": 10},");
  add_new_msg("{\"v\": 10},");
  struct pair lv;
  read_next_token(&lv);
  read_next_token(&lv);
  UNIT_TEST_ASSERT(strcmp(lv.value, "10") == 0);
  UNIT_TEST_END();
}

UNIT_TEST(read_add_read_label){
  UNIT_TEST_BEGIN();
  init_json_decoder("[{\"bn\": \"urn:mac:testID\"},");
  struct pair lv;
  read_next_token(&lv);
  add_new_msg("{\"v\": 10},");
  read_next_token(&lv);

  UNIT_TEST_ASSERT(strcmp(lv.label, "v") == 0);
  UNIT_TEST_END();
}

UNIT_TEST(read_add_read_value_str){
  UNIT_TEST_BEGIN();
  init_json_decoder("[{\"bn\": \"urn:mac:testID\"},");
  struct pair lv;
  read_next_token(&lv);
  add_new_msg("{\"u\": \"dB\"},");
  read_next_token(&lv);
  UNIT_TEST_ASSERT(strcmp(lv.value, "dB") == 0);
  UNIT_TEST_END();
}

UNIT_TEST(read_add_read_value_int){
  UNIT_TEST_BEGIN();
  init_json_decoder("[{\"bn\": \"urn:mac:testID\"},");
  struct pair lv;
  read_next_token(&lv);
  add_new_msg("{\"v\": 10},");
  read_next_token(&lv);
  UNIT_TEST_ASSERT(strcmp(lv.value, "10") == 0);
  UNIT_TEST_END();
}

PROCESS(unit_testing, "Unit Testing");

AUTOSTART_PROCESSES(&unit_testing);

PROCESS_THREAD(unit_testing, ev, data){
  PROCESS_BEGIN();
  UNIT_TEST_RUN(read_label_null);
  UNIT_TEST_RUN(read_value_null);
  UNIT_TEST_RUN(read_one_label);
  UNIT_TEST_RUN(read_one_value_str);
  UNIT_TEST_RUN(read_one_value_int);
  UNIT_TEST_RUN(add_msg_and_read_label);
  UNIT_TEST_RUN(add_msg_and_read_value_str);
  UNIT_TEST_RUN(add_msg_and_read_value_int);
  UNIT_TEST_RUN(read_add_read_label);
  UNIT_TEST_RUN(read_add_read_value_str);
  UNIT_TEST_RUN(read_add_read_value_int);
  PROCESS_END();
}
