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
#include "senml-json.h"
#include "senml-cbor.h"
#include <string.h>
#include <stdio.h>

#define BUFFER_SIZE 1024

UNIT_TEST_REGISTER(json_empty_string, "JSON: Adding 0 records should return \"[]\"");
UNIT_TEST_REGISTER(json_noise_sensor, "JSON: Noise sensor example");
UNIT_TEST_REGISTER(json_many_parameters, "JSON: Many parameters");
UNIT_TEST_REGISTER(json_multiple_records, "JSON: Multiple records");
UNIT_TEST_REGISTER(cbor_empty_string, "CBOR: Adding 0 records should return \"[]\"");
UNIT_TEST_REGISTER(cbor_noise_sensor, "CBOR: Noise sensor example");
UNIT_TEST_REGISTER(cbor_many_parameters, "CBOR: Many parameters");
UNIT_TEST_REGISTER(cbor_multiple_records, "CBOR: Multiple records");

static int
strcmpn(char *str1, char *str2, int n)
{
  int i;
  for(i = 0; i < n; i++) {
    if(str1[i] - str2[i] != 0) {
      return str1[i] - str2[i];
    }
  }
  return 0;
}
UNIT_TEST(json_empty_string){
  char buffer_pointer[BUFFER_SIZE];
  int len = 0;
  UNIT_TEST_BEGIN();

  SENML_INIT_JSON();
  len += SENML_START_PACK(buffer_pointer + len, BUFFER_SIZE - len);
  len += SENML_END_PACK(buffer_pointer + len, BUFFER_SIZE - len);

  UNIT_TEST_ASSERT(strcmp(buffer_pointer, "[]") == 0);

  UNIT_TEST_END();
}

UNIT_TEST(json_noise_sensor){
  char buffer_pointer[BUFFER_SIZE];
  int len = 0;
  UNIT_TEST_BEGIN();
  SENML_INIT_JSON();
  len += SENML_START_PACK(buffer_pointer + len, BUFFER_SIZE - len);
  len += SENML_ADD_RECORD(buffer_pointer + len, BUFFER_SIZE - len, BASE_NAME, "urn:dev:mac:fcc23d0000003790", UNIT, "dB", BOOLEAN_VALUE, 0, VALUE, 50.00);
  len += SENML_END_PACK(buffer_pointer + len, BUFFER_SIZE - len);
  UNIT_TEST_ASSERT(strcmp(buffer_pointer, "[{\"bn\":\"urn:dev:mac:fcc23d0000003790\",\"u\":\"dB\",\"vb\":false,\"v\":50}]") == 0);

  UNIT_TEST_END();
}

UNIT_TEST(json_many_parameters){
  char buffer_pointer[BUFFER_SIZE];
  int len = 0;
  UNIT_TEST_BEGIN();

  SENML_INIT_JSON();
  len += SENML_START_PACK(buffer_pointer + len, BUFFER_SIZE - len);
  len += SENML_ADD_RECORD(buffer_pointer + len, BUFFER_SIZE - len, BASE_NAME, "urn:dev:mac:fcc23d0000003790", BASE_TIME, 123456789.00, BASE_UNIT, "Volt", BASE_VERSION, 2, VALUE, -1.00, SUM, 0.00, TIME, 643.00, UPDATE_TIME, 0.00);
  len += SENML_END_PACK(buffer_pointer + len, BUFFER_SIZE - len);
  UNIT_TEST_ASSERT(strcmp(buffer_pointer, "[{\"bn\":\"urn:dev:mac:fcc23d0000003790\",\"bt\":1.23457e+08,\"bu\":\"Volt\",\"bver\":2,\"v\":-1,\"s\":0,\"t\":643,\"ut\":0}]") == 0);

  UNIT_TEST_END();
}

UNIT_TEST(json_multiple_records){
  char buffer_pointer[BUFFER_SIZE];
  int len = 0;
  UNIT_TEST_BEGIN();

  SENML_INIT_JSON();
  len += SENML_START_PACK(buffer_pointer + len, BUFFER_SIZE - len);
  len += SENML_ADD_RECORD(buffer_pointer + len, BUFFER_SIZE - len, BASE_NAME, "urn:dev:mac:fcc23d0000003790", UNIT, "dB", VALUE, 50.00);
  len += SENML_ADD_RECORD(buffer_pointer + len, BUFFER_SIZE - len, BASE_NAME, "urn:dev:mac:fcc23d0000003788", UNIT, "w", VALUE, 0.00);
  len += SENML_ADD_RECORD(buffer_pointer + len, BUFFER_SIZE - len, BASE_NAME, "urn:dev:mac:fcc23d0000013290", UNIT, "m", VALUE, 35.25);
  len += SENML_END_PACK(buffer_pointer + len, BUFFER_SIZE - len);

  UNIT_TEST_ASSERT(strcmp(buffer_pointer, "[{\"bn\":\"urn:dev:mac:fcc23d0000003790\",\"u\":\"dB\",\"v\":50},{\"bn\":\"urn:dev:mac:fcc23d0000003788\",\"u\":\"w\",\"v\":0},{\"bn\":\"urn:dev:mac:fcc23d0000013290\",\"u\":\"m\",\"v\":35.25}]") == 0);

  UNIT_TEST_END();
}

UNIT_TEST(cbor_empty_string){
  char buffer_pointer[BUFFER_SIZE];
  int len = 0;
  UNIT_TEST_BEGIN();

  SENML_INIT_CBOR();
  len += SENML_START_PACK(buffer_pointer + len, BUFFER_SIZE - len);
  len += SENML_END_PACK(buffer_pointer + len, BUFFER_SIZE - len);

  char str[] = { 0x9F, 0xFF, 0x00 };

  UNIT_TEST_ASSERT(strcmpn(buffer_pointer, str, sizeof(str) / sizeof(char)) == 0);

  UNIT_TEST_END();
}

UNIT_TEST(cbor_noise_sensor){
  char buffer_pointer[BUFFER_SIZE];
  int len = 0;
  UNIT_TEST_BEGIN();

  SENML_INIT_CBOR();
  len += SENML_START_PACK(buffer_pointer + len, BUFFER_SIZE - len);
  len += SENML_ADD_RECORD(buffer_pointer + len, BUFFER_SIZE - len, BASE_NAME, "urn:mac:fcc2948375028573", UNIT, "dB", VALUE, 59.23);
  len += SENML_END_PACK(buffer_pointer + len, BUFFER_SIZE - len);
  char str[] = { 0x9F, 0xBF, 0x21, 0x78, 0x18, 0x75, 0x72, 0x6E, 0x3A, 0x6D, 0x61, 0x63, 0x3A, 0x66, 0x63, 0x63, 0x32, 0x39, 0x34, 0x38, 0x33, 0x37, 0x35, 0x30, 0x32, 0x38, 0x35, 0x37, 0x33, 0x01, 0x62, 0x64, 0x42, 0x02, 0xFA, 0x42, 0x6C, 0xEB, 0x85, 0xFF, 0xFF, 0x00 };
  UNIT_TEST_ASSERT(strcmpn(buffer_pointer, str, sizeof(str) / sizeof(char)) == 0);

  UNIT_TEST_END();
}

UNIT_TEST(cbor_multiple_records){
  char buffer_pointer[BUFFER_SIZE];
  int len = 0;
  UNIT_TEST_BEGIN();

  SENML_INIT_CBOR();
  len += SENML_START_PACK(buffer_pointer, BUFFER_SIZE - len);
  len += SENML_ADD_RECORD(buffer_pointer + len, BUFFER_SIZE - len, BASE_NAME, "urn:mac:fcc2948375028573", UNIT, "dB", VALUE, 8923.223);
  len += SENML_ADD_RECORD(buffer_pointer + len, BUFFER_SIZE - len, BOOLEAN_VALUE, 0, BASE_TIME, 123456789.00);
  len += SENML_END_PACK(buffer_pointer + len, BUFFER_SIZE - len);
  char str[] = { 0x9F, 0xBF, 0x21, 0x78, 0x18, 0x75, 0x72, 0x6E, 0x3A, 0x6D, 0x61, 0x63, 0x3A, 0x66, 0x63, 0x63, 0x32, 0x39, 0x34, 0x38, 0x33, 0x37, 0x35, 0x30, 0x32, 0x38, 0x35, 0x37, 0x33, 0x01, 0x62, 0x64, 0x42, 0x02, 0xFA, 0x46, 0x0B, 0x6C, 0xE4, 0xFF, 0xBF, 0x04, 0xF4, 0x22, 0xFA, 0x4C, 0xEB, 0x79, 0xA3, 0xFF, 0xFF, 0x00 };

  UNIT_TEST_ASSERT(strcmpn(buffer_pointer, str, sizeof(str) / sizeof(char)) == 0);

  UNIT_TEST_END();
}

UNIT_TEST(cbor_many_parameters) {
  char buffer_pointer[BUFFER_SIZE];
  int len = 0;
  UNIT_TEST_BEGIN();

  SENML_INIT_CBOR();
  len += SENML_START_PACK(buffer_pointer + len, BUFFER_SIZE - len);
  len += SENML_ADD_RECORD(buffer_pointer + len, BUFFER_SIZE - len, BASE_VERSION, 5, UNIT, "m/s", VALUE, 12.608765);
  len += SENML_END_PACK(buffer_pointer + len, BUFFER_SIZE - len);

  char str[] = { 0x9F, 0xBF, 0x20, 0x05, 0x01, 0x63, 0x6D, 0x2F, 0x73, 0x02, 0xFA, 0x41, 0x49, 0xBD, 0x80, 0xFF, 0xFF, 0x00 };
  UNIT_TEST_ASSERT(strcmpn(buffer_pointer, str, sizeof(str) / sizeof(char)) == 0);

  UNIT_TEST_END();
}

PROCESS(unit_testing, "Unit Testing");

AUTOSTART_PROCESSES(&unit_testing);

PROCESS_THREAD(unit_testing, ev, data){
  PROCESS_BEGIN();

  UNIT_TEST_RUN(json_empty_string);
  UNIT_TEST_RUN(json_noise_sensor);
  UNIT_TEST_RUN(json_many_parameters);
  UNIT_TEST_RUN(json_multiple_records);
  UNIT_TEST_RUN(cbor_empty_string);
  UNIT_TEST_RUN(cbor_noise_sensor);
  UNIT_TEST_RUN(cbor_many_parameters);
  UNIT_TEST_RUN(cbor_multiple_records);

  PROCESS_END();
}
