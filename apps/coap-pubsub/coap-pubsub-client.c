/*
 * Copyright (c) 2017, Jussi Haikara
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
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      CoAP Publish-Subscribe client
 * \author
 *      Jussi Haikara <haikara@kth.se>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "er-coap.h"
#include "er-coap-engine.h"
#include "er-coap-observe-client.h"
#include "rest-engine.h"
#include "coap-pubsub-client.h"

#define LINK_PARSE_BUFFER_SIZE (COAP_MAX_BLOCK_SIZE * 2)
#define LINK_RT_MAX_SIZE 15

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

typedef struct {
  uint16_t ct;
  char rt[LINK_RT_MAX_SIZE + 1];
} link_param_t;

static coap_packet_t request[1];
static client_topic_t *handled_topic;
static broker_t *discover_broker;
static list_t *discover_topics;
static char url_buffer[COAP_PUBSUB_MAX_URL_LEN + 2];
static char payload_buffer[COAP_MAX_BLOCK_SIZE + 1];
static char link_buffer[LINK_PARSE_BUFFER_SIZE + 1];

static int
parse_next_link(char *out_buffer, link_param_t *params, uint8_t is_last)
{
  char *url_ptr, *next_ptr, *option;
  size_t url_len;
  unsigned int i;

  params->ct = 0;
  memset(params->rt, '\0', sizeof(params->rt));

  next_ptr = strchr(link_buffer, ',');

  /* If there are more blocks and we can't find the next link separator,
     we won't know if we have the entire link */
  if(next_ptr == NULL && !is_last) {
    return 0;
  }

  if(next_ptr) {
    url_len = next_ptr - link_buffer;
  } else {
    url_len = sizeof(link_buffer);
  }

  if(link_buffer[0] == '<') {

    memcpy(url_buffer, link_buffer, url_len);
    url_buffer[url_len] = '\0';
    /* TODO: Check for invalid chars in url path */
    url_ptr = strtok(url_buffer, "<>");

    /* Find the content and resource types */
    while((option = strtok(NULL, ";"))) {
      if(strncmp(option, "ct=", 3) == 0 && strlen(option) > 3) {
        params->ct = atoi(&(option[3]));
        PRINTF("ct=%d \n", params->ct);
      } else if(strncmp(option, "rt=", 3) == 0 && strlen(option) > 5) {
        strncpy(params->rt, &(option[3]), LINK_RT_MAX_SIZE);
        PRINTF("rt=%s \n", params->rt);
      }
    }

    if(url_ptr[0] == '/') {
      strncpy(out_buffer, url_ptr + 1, strlen(url_ptr) + 1);
    } else {
      strncpy(out_buffer, url_ptr, strlen(url_ptr) + 1);
    }
    out_buffer[strlen(url_ptr) + 1] = '\0';
    PRINTF("Parsed link %s\n", out_buffer);
  } else {
    PRINTF("No valid link start found: %s\n", (char *)link_buffer);
  }

  /* Compact the link buffer. This could probably be optimized. */
  if(next_ptr != NULL && url_len > 1) {
    for(i = 0; i < (sizeof(link_buffer) - url_len - 1); i++) {
      link_buffer[i] = link_buffer[i + url_len + 1];
    }
  }
  return next_ptr != NULL;
}
/*---------------------------------------------------------------------------*/
void
process_discovery_response(void *response)
{
  const uint8_t *payload;
  size_t payload_len, broker_base_len;
  int links_remaining = 1;
  unsigned int response_ct = 0;
  uint32_t block_num;
  uint8_t more;
  size_t start = sizeof(char) * strlen(link_buffer);
  link_param_t parameters;
  broker_t *broker = discover_broker;
  static client_topic_t *topic;

  payload_len = coap_get_payload(response, &payload);
  coap_get_header_content_format(response, &response_ct);

  if(payload_len < 1 || ((coap_packet_t *)response)->code != CONTENT_2_05) {
    PRINTF("No-content response received\n");
    return;
  }

  if(coap_get_header_block2(response, &block_num, &more, NULL, NULL)) {
    if(block_num == 0) {
      memset(link_buffer, '\0', sizeof(link_buffer));

      if(discover_topics != NULL) {
        topic = (client_topic_t *)list_head(*discover_topics);
      }
    }
  } else {
    more = 0;
    memset(link_buffer, '\0', sizeof(link_buffer));

    if(discover_topics != NULL) {
      topic = (client_topic_t *)list_head(*discover_topics);
    }
  }

  memcpy(link_buffer + start, (const char *)payload, payload_len);
  link_buffer[start + payload_len] = '\0';

  do {
    links_remaining = parse_next_link((char *)url_buffer, &parameters, !more);
    if(url_buffer[0]) {
      /* Checks for pubsub function set if one hasn't been found yet */
      if(broker->base_url[0]== '\0' && parameters.ct == APPLICATION_LINK_FORMAT
	 && ((strncmp(parameters.rt, "core.ps", LINK_RT_MAX_SIZE) == 0) || 
	     (strncmp(parameters.rt, "core.ps.discover", LINK_RT_MAX_SIZE) == 0))) {
	   strncpy(broker->base_url, url_buffer, COAP_PUBSUB_MAX_URL_LEN);
	   PRINTF("Found broker function set at /%s\n", (char *)(broker->base_url));

	   /* Trimming to ps. Compatibility issue. To accept broker ps/ discovery response*/
	   if(!strncmp(broker->base_url, "ps/", LINK_RT_MAX_SIZE))
	      broker->base_url[2] = '\0';

      } else {
        broker_base_len = strlen(broker->base_url);
        if(broker_base_len > 0
           && strncmp(broker->base_url, url_buffer, broker_base_len) == 0) {

          PRINTF("Received topic link %s\n", (char *)url_buffer);

          if(topic) {

            strncpy(topic->url, &url_buffer[broker_base_len + 1], COAP_PUBSUB_MAX_URL_LEN);

            topic->content_type = parameters.ct;
            topic->broker = broker;

            topic = list_item_next(topic);
          } else {
            PRINTF("No struct passed for storing topics\n");
          }
        } else {
          PRINTF("Received non-topic link %s\n", (char *)url_buffer);
        }
      }
    }
    url_buffer[0] = '\0';
  } while(links_remaining);

  broker->last_response_code = ((coap_packet_t *)response)->code;
}
/*---------------------------------------------------------------------------*/
void
process_response_chunk(void *response)
{
  const uint8_t *payload;
  size_t payload_len;
  uint32_t block_offset = 0;

  if(handled_topic->content != NULL) {
    payload_len = coap_get_payload(response, &payload);

    coap_get_header_block2(response, NULL, NULL, NULL, &block_offset);

    if(payload_len + block_offset > COAP_PUBSUB_MAX_CONTENT_LEN) {
      payload_len = COAP_PUBSUB_MAX_CONTENT_LEN - block_offset;
    }

    memcpy(handled_topic->content + block_offset, payload, payload_len);
    handled_topic->content_len = payload_len + block_offset;
    PRINTF("Received payload chunk, offset %u, length %u\n", (unsigned int)block_offset, payload_len);
  } else {
    PRINTF("Received payload chunk, no content buffer set in topic\n");
  }

#if COAP_OBSERVE_CLIENT == 1
  if(IS_OPTION(((coap_packet_t *)response), COAP_OPTION_OBSERVE)) {
    handled_topic->flags |= IS_SUBSCRIBED;
    coap_get_header_observe(response, &handled_topic->observe_seq);
    PRINTF("Notification has observe %u\n", (unsigned int)handled_topic->observe_seq);
  } else if(handled_topic->flags & IS_SUBSCRIBED) {
    PRINTF("Notification missing observe\n");
    coap_get_header_uri_path(response, (const char **)&url_buffer);
    coap_obs_remove_observee_by_url(&handled_topic->broker->address,
                                    handled_topic->broker->port, (const char *)url_buffer);

    handled_topic->flags &= ~IS_SUBSCRIBED;
  }
#endif

  handled_topic->last_response_code = ((coap_packet_t *)response)->code;
}
/*---------------------------------------------------------------------------*/
#if COAP_OBSERVE_CLIENT == 1
void
observe_callback(coap_observee_t *obs,
                 void *notification, coap_notification_flag_t flag)
{
  PRINTF("Received notification\n");
  handled_topic = (client_topic_t *)obs->data;

  process_response_chunk(notification);

  if(handled_topic->notification_callback != NULL
     && (handled_topic->flags & SUBSCRIPTION_ENABLED)) {
    (handled_topic->notification_callback)(handled_topic);
  }
}
#endif
/*---------------------------------------------------------------------------*/
void
coap_pubsub_init_client()
{
  rest_init_engine();
}
coap_packet_t *
coap_pubsub_setup_discover(list_t *topic_list, broker_t *broker)
{
  discover_broker = broker;
  discover_topics = topic_list;

  broker->last_response_code = 0;

  coap_init_message(request, COAP_PUBSUB_MESSAGE_TYPE, COAP_GET, 0);

  /* If we don't want to receive topics, filter by the function set. */
  if(topic_list == NULL) {
    coap_set_header_uri_query(request, "rt=core.ps");
  }

  coap_set_header_uri_path(request, ".well-known/core");
  coap_set_header_content_format(request, APPLICATION_LINK_FORMAT);

  return request;
}
/*---------------------------------------------------------------------------*/
coap_packet_t *
coap_pubsub_setup_create(client_topic_t *topic)
{
  char *base_topic_name;
  size_t url_base_len, url_path_len;
  char buf[COAP_PUBSUB_MAX_URL_LEN];

  handled_topic = topic;

  topic->last_response_code = 0;

  coap_init_message(request, COAP_PUBSUB_MESSAGE_TYPE, COAP_POST, 0);

  base_topic_name = strrchr((char *)&topic->url, '/');

  if(!base_topic_name) {
    base_topic_name = (char *)&topic->url;
  } else {
    base_topic_name += 1 * sizeof(char); /* Add 1 to go past the last separator */
  }

  memcpy(buf,topic->url,strlen(topic->url));
  buf[strlen(topic->url)-1]=0;

  char* q = strrchr(buf, '/');
  if(q == 0) 
    q = (char *)&buf;
  else q++;

  url_base_len = strlen(topic->broker->base_url);
  url_path_len = (base_topic_name - (char *)&topic->url) * sizeof(char);

  if(url_path_len > COAP_PUBSUB_MAX_URL_LEN - url_base_len - 2) {
    url_path_len = COAP_PUBSUB_MAX_URL_LEN - url_base_len - 2;
  }

  strncpy((char *)&url_buffer, (char *)&topic->broker->base_url, COAP_PUBSUB_MAX_URL_LEN - 2);
  url_buffer[url_base_len] = '/';
  strncpy(&url_buffer[url_base_len + 1], (char *)&topic->url, url_path_len);

  snprintf((char *)&payload_buffer, COAP_PUBSUB_MAX_CREATE_MESSAGE_LEN,
           "<%s>;ct=%u", q, topic->content_type);
  
  memcpy(buf, url_buffer, strlen(url_buffer));
  char *p = strrchr((char *)&buf, '/');
  *p = 0;
  p = strrchr((char *)&buf, '/');
  *p = 0;

  coap_set_header_uri_path(request, (const char *)&buf);
  coap_set_payload(request, &payload_buffer, strlen((char *)&payload_buffer));
  coap_set_header_content_format(request, APPLICATION_LINK_FORMAT);

  if(topic->max_age > 0) {
    coap_set_header_max_age(request, topic->max_age);
  }

  return request;
}
/*---------------------------------------------------------------------------*/
coap_packet_t *
coap_pubsub_setup_publish(client_topic_t *topic)
{
  size_t url_base_len;
  handled_topic = topic;

  topic->last_response_code = 0;

  coap_init_message(request, COAP_PUBSUB_MESSAGE_TYPE, COAP_PUT, 0);

  url_base_len = strlen(topic->broker->base_url);
  strncpy((char *)&url_buffer, (char *)&topic->broker->base_url, COAP_PUBSUB_MAX_URL_LEN - 2);
  url_buffer[url_base_len] = '/';
  strncpy(&url_buffer[url_base_len + 1], (char *)&topic->url, COAP_PUBSUB_MAX_URL_LEN - url_base_len - 2);

  coap_set_header_uri_path(request, (const char *)&url_buffer);
  coap_set_payload(request, topic->content, topic->content_len);
  coap_set_header_content_format(request, topic->content_type);

  if(topic->max_age > 0) {
    coap_set_header_max_age(request, topic->max_age);
  }

  return request;
}
/*---------------------------------------------------------------------------*/
coap_packet_t *
coap_pubsub_setup_read(client_topic_t *topic)
{
  size_t url_base_len;
  handled_topic = topic;

  topic->last_response_code = 0;

  coap_init_message(request, COAP_PUBSUB_MESSAGE_TYPE, COAP_GET, 0);

  url_base_len = strlen(topic->broker->base_url);
  strncpy((char *)&url_buffer, (char *)&topic->broker->base_url, COAP_PUBSUB_MAX_URL_LEN - 2);
  url_buffer[url_base_len] = '/';
  strncpy(&url_buffer[url_base_len + 1], (char *)&topic->url, COAP_PUBSUB_MAX_URL_LEN - url_base_len - 2);

  coap_set_header_uri_path(request, (const char *)&url_buffer);
  coap_set_header_content_format(request, topic->content_type);

  return request;
}
/*---------------------------------------------------------------------------*/
#if COAP_OBSERVE_CLIENT == 1
coap_packet_t *
coap_pubsub_setup_subscribe(client_topic_t *topic)
{
  size_t token_len;
  uint8_t *token;
  coap_pubsub_setup_read(topic);

  token_len = coap_generate_token(&token);
  coap_set_token(request, token, token_len);
  coap_set_header_observe(request, 0);

  if(!(topic->flags & SUBSCRIPTION_ENABLED)) {
    coap_obs_add_observee(&topic->broker->address, (topic)->broker->port, token,
                          token_len, (const char *)&url_buffer, observe_callback, topic);
    topic->flags |= SUBSCRIPTION_ENABLED;
  }

  return request;
}
/*---------------------------------------------------------------------------*/
coap_packet_t *
coap_pubsub_setup_unsubscribe(client_topic_t *topic)
{
  topic->flags &= ~SUBSCRIPTION_ENABLED;
  coap_pubsub_setup_read(topic);

  coap_set_header_observe(request, 1);

  return request;
}
#endif
/*---------------------------------------------------------------------------*/
coap_packet_t *
coap_pubsub_setup_remove(client_topic_t *topic)
{
  size_t url_base_len;
  handled_topic = topic;

  topic->last_response_code = 0;

  coap_init_message(request, COAP_PUBSUB_MESSAGE_TYPE, COAP_DELETE, 0);

  url_base_len = strlen(topic->broker->base_url);
  strncpy((char *)&url_buffer, (char *)&topic->broker->base_url, COAP_PUBSUB_MAX_URL_LEN - 2);
  url_buffer[url_base_len] = '/';
  strncpy(&url_buffer[url_base_len + 1], (char *)&topic->url, COAP_PUBSUB_MAX_URL_LEN - url_base_len - 2);

  coap_set_header_uri_path(request, (const char *)&url_buffer);

  return request;
}
