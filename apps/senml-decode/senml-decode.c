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
 *
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
#include <stdlib.h>
#include <string.h>
#include "jsonparse.h"
#include "senml-decode.h"

/* REQUIRES JSON APP - MAKE SURE TO INCLUDE IT IN YOUR PROJECT ASWELL */

struct jsonparse_state state;

char *label_buf;
char *value_buf;

char *oldLabelAddr = 0;
void *oldValueAddr = 0;
char *oldAddBufAddr = 0;

int
length(char *string)
{
  int i;
  for(i = 0; string[i] != '\0'; ++i) {
  }
  return i;
}
void
init_json_decoder(char *msg)
{
  state.json = msg;
  state.pos = 0;
  state.len = length(msg);
  jsonparse_setup(&state, state.json, state.len);
  jsonparse_next(&state);
  jsonparse_next(&state);
}
void
read_next_token(struct pair *token)
{
  if(oldLabelAddr != 0) {
    free(oldLabelAddr);
  }
  if(oldValueAddr != 0) {
    free(oldValueAddr);
  }
  int parseNextNumber = jsonparse_next(&state);
  if(parseNextNumber == 44) { /*Reads comma */
    jsonparse_next(&state);
    int elem_len = jsonparse_get_len(&state);
    int totlen = elem_len;
    elem_len++;
    char *label_buf = malloc(sizeof(char) * elem_len);
    jsonparse_copy_value(&state, label_buf, elem_len);

    jsonparse_next(&state);
    elem_len = jsonparse_get_len(&state);
    totlen = totlen + elem_len;
    elem_len++;
    char *value_buf = malloc(sizeof(char) * elem_len);
    jsonparse_copy_value(&state, value_buf, elem_len);

    token->label = label_buf;
    token->value = value_buf;
    oldLabelAddr = label_buf;
    oldValueAddr = value_buf;
    state.json = state.json + state.pos;
    state.len = state.len - state.pos;
    state.pos = 0;
  } else if(parseNextNumber == 125) { /*Reads } */
    if(jsonparse_next(&state) == 93) { /*If ] */
      token->label = NULL;
      token->value = NULL;
    } else {  /*If not ] then it has to be comma */
      jsonparse_next(&state);
      jsonparse_next(&state);
      int elem_len = jsonparse_get_len(&state);
      int totlen = elem_len;
      elem_len++;
      char *label_buf = malloc(sizeof(char) * elem_len);
      jsonparse_copy_value(&state, label_buf, elem_len);

      jsonparse_next(&state);
      elem_len = jsonparse_get_len(&state);
      totlen = totlen + elem_len;
      elem_len++;
      char *value_buf = malloc(sizeof(char) * elem_len);
      jsonparse_copy_value(&state, value_buf, elem_len);

      token->label = label_buf;
      token->value = value_buf;
      oldLabelAddr = label_buf;
      oldValueAddr = value_buf;
      state.json = state.json + state.pos;
      state.len = state.len - state.pos;
      state.pos = 0;
    }
  } else { /*Reads a label+value */
    int elem_len = jsonparse_get_len(&state);
    elem_len++;
    char *label_buf = malloc(sizeof(char) * elem_len);
    jsonparse_copy_value(&state, label_buf, elem_len);

    jsonparse_next(&state);
    elem_len = jsonparse_get_len(&state);
    elem_len++;
    char *value_buf = malloc(sizeof(char) * elem_len);
    jsonparse_copy_value(&state, value_buf, elem_len);

    token->label = label_buf;
    token->value = value_buf;
    oldLabelAddr = label_buf;
    oldValueAddr = value_buf;
    state.json = state.json + state.pos;
    state.len = state.len - state.pos;
    state.pos = 0;
  }
}
void
add_new_msg(char *msg)
{
  if(oldAddBufAddr != 0) {
    free(oldAddBufAddr);
  }
  int stlen = state.len;
  int msglen = length(msg);
  int totlen = stlen + msglen;
  char *buf = malloc(sizeof(char) * (totlen + 1));
  strcpy(buf, state.json);
  strcat(buf, msg);
  state.json = buf;
  state.len = totlen;
  oldAddBufAddr = buf;
}