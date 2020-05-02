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
#ifndef SENML_API_H_
#define SENML_API_H_
#include "label.h"
#include "senml-formatter.h"

#define SENML_START_PACK(buf_ptr, buf_len) senml_start_pack(buf_ptr, buf_len)
#define SENML_END_PACK(buf_ptr, buf_len) senml_end_pack(buf_ptr, buf_len)

#define SENML_ADD_RECORD(buf_ptr, buf_len, ...) senml_add_record(buf_ptr, buf_len, __VA_ARGS__, END)

void senml_init(struct senml_formatter frmttr);
int senml_end_pack(char *buf_ptr, int buf_len);
int senml_start_pack(char *buf_ptr, int buf_len);
int senml_add_record(char *buf_ptr, int buf_len, Label label, ...);
#endif