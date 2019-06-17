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
 *      CoAP Publish-Subscribe broker
 * \author
 *      Jussi Haikara <haikara@kth.se>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "er-coap.h"
#include "er-coap-observe.h"
#include "rest-engine.h"
#include "coap-pubsub-broker.h"

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static void ps_get_handler(void *request, void *response, uint8_t *buffer,
                           uint16_t preferred_size, int32_t *offset);
static void ps_post_handler(void *request, void *response, uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset);
static void ps_put_handler(void *request, void *response, uint8_t *buffer,
                           uint16_t preferred_size, int32_t *offset);
static void ps_delete_handler(void *request, void *response, uint8_t *buffer,
                              uint16_t preferred_size, int32_t *offset);

/* Resource for the pubsub function set. */
RESOURCE(ps_base, "rt=\"core.ps\";ct=40",
         NULL, ps_post_handler, NULL, NULL);

static topic_t base_topic = {
  NULL,
  &ps_base,
  NULL,
  0,
  APPLICATION_LINK_FORMAT,
  {   /* Max-age */
    0,
    0
  }
};

LIST(topics);

#if COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_STATIC
static topic_t topic_data[COAP_PUBSUB_BROKER_MAX_TOPICS];
#endif

static int num_topics = 0;

void
coap_pubsub_init_broker(void)
{
  list_init(topics);
  list_add(topics, &base_topic);

  rest_init_engine();

  rest_activate_resource(&ps_base, COAP_PUBSUB_BROKER_BASE_URI);
}
/*---------------------------------------------------------------------------*/
static topic_t *
find_topic(const char *url, size_t url_len)
{
  size_t topic_url_len;
  topic_t *topic = (topic_t *)list_head(topics);

  for(; topic != NULL; topic = topic->next) {

    topic_url_len = strlen(topic->resource->url);

    if((url_len == topic_url_len
        || (url_len > topic_url_len
            && (topic->resource->flags & HAS_SUB_RESOURCES)
            && url[topic_url_len] == '/'))
       && strncmp(topic->resource->url, url, topic_url_len) == 0) {

      return topic;
    }
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
/* Parses the URL and content type from a CoAP payload and allocates the URL to a string.
 * TODO: Does not currently do validation of the URL path.
 */
static int
parse_topic_url(char **topic_url, const uint8_t **payload, size_t payload_len, const char *prefix)
{
  char payload_buffer[COAP_PUBSUB_MAX_CREATE_MESSAGE_LEN + 1] = { 0 };
  char *url_buffer, *option;
  int ct = -1;

  if(payload_len < 1 || payload_len > COAP_PUBSUB_MAX_CREATE_MESSAGE_LEN) {
    PRINTF("Invalid payload length\n");
    return -1;
  }

  strncpy(payload_buffer, (const char *)*payload, payload_len);
  if(payload_buffer[0] != '<') {
    PRINTF("Invalid payload start\n");
    return -1;
  }
  /* Parses the first link in the payload. */
  strtok(payload_buffer, ","); /* Deilimit search by the first url */
  url_buffer = strtok(payload_buffer, "<>/"); /* Don't allow slashes in url */

  /* Find the content type */
  while((option = strtok(NULL, ";"))) {
    if(strncmp(option, "ct=", 3) == 0 && strlen(option) > 3) {
      ct = (int)strtol(&(option[3]), NULL, 10);
      break;
    }
  }

  if(!url_buffer || ct < 0) {
    PRINTF("Not all payload options found, ct = %d\n", ct);
    return -1;
  }

#if COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_MALLOC
  *topic_url = (char *)malloc(sizeof(char) * (strlen(url_buffer) + strlen(prefix) + 2));

  if(*topic_url == NULL) {
    PRINTF("Failed to allocate memory for topic\n");
    return -1;
  }
#endif

  strncpy(*topic_url, prefix, strlen(prefix));
  *(*topic_url + strlen(prefix)) = '/';
  strncpy(*topic_url + strlen(prefix) + 1, url_buffer, strlen(url_buffer) + 1);

  return ct;
}
/*---------------------------------------------------------------------------*/
/* Removes expired and deleted topics. */
static void
clean_topics()
{
  topic_t *topic;

  for(topic = (topic_t *)list_head(topics); topic != NULL; topic = topic->next) {
    if(topic->max_age.interval > 0
       && timer_expired(&topic->max_age)
       && topic->content_len == 0) {

      PRINTF("Old topic %s deleted\n", topic->resource->url);
      coap_remove_observer_by_uri(NULL, 0, topic->resource->url);
      list_remove(rest_get_resources(), topic->resource);
#if COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_MALLOC
      free((void *)topic->resource->url);
      free((void *)topic->resource->attributes);
      free(topic->resource);

      list_remove(topics, topic);
      free(topic);
#elif COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_STATIC
      topic->resource = NULL; /* Resource pointer set to NULL indicates topic slot is unused */
      list_remove(topics, topic);
#endif
      num_topics--;
    }
  }
}
/*---------------------------------------------------------------------------*/
#if COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_STATIC
/* Returns an unused topic form the topic_data array when static memory allocation is used. */
static topic_t *
get_free_topic()
{
  int i;

  for(i = 0; i < COAP_PUBSUB_BROKER_MAX_TOPICS; i++) {
    if(topic_data[i].resource == NULL) {
      return &topic_data[i];
    }
  }
  return NULL;
}
#endif
/*---------------------------------------------------------------------------*/
/* Handles the CREATE interface */
static void
ps_post_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  size_t payload_len, request_url_len;
  int ct;
  uint32_t max_age;
  const uint8_t *payload = NULL;
  const char *request_url = NULL;
#if COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_STATIC
  char topic_url_arr[COAP_PUBSUB_BROKER_STATIC_URL_SIZE + 1];
  char *topic_url = topic_url_arr;
#else
  char *topic_url = NULL;
  char attribute_buffer[16] = { 0 };
#endif
  resource_t *topic_res = NULL;
  topic_t *parent_topic, *created_topic, *existing_topic;
  char *attributes = NULL;

#if COAP_PUBSUB_BROKER_ALLOW_CLIENT_TOPIC_CREATION == 0
  REST.set_response_status(response, REST.status.UNAUTHORIZED);

#else
  if(num_topics >= COAP_PUBSUB_BROKER_MAX_TOPICS) {
    PRINTF("CREATE: Topic limit reached\n");
    REST.set_response_status(response, REST.status.SERVICE_UNAVAILABLE);
    REST.set_response_payload(response, "Topic limit reached", 19);
    return;
  }

  request_url_len = REST.get_url(request, &request_url);

  /* Since the rest engine doesn't pass which resource is being handled,
     the url string is matched again to find the topic. This could be optimized. */
  parent_topic = find_topic(request_url, request_url_len);

  if(parent_topic) {
    payload_len = REST.get_request_payload(request, &payload);
    PRINTF("CREATE on: %s, message: %s\n", parent_topic->resource->url, payload);

    /* Subtopics are only supported for topics with ct set to application/link-format. */
    if(parent_topic->content_format != APPLICATION_LINK_FORMAT) {
      PRINTF("Attempeted to CREATE subtopic on topic with ct=%u\n", parent_topic->content_format);
      REST.set_response_status(response, REST.status.BAD_REQUEST);
      return;
    }

    ct = parse_topic_url(&topic_url, &payload, payload_len, parent_topic->resource->url);
    if(ct >= 0) {

      existing_topic = find_topic(topic_url, strlen(topic_url));
      if(existing_topic) {

        /* Check if the topic is still valid */
        if(existing_topic->max_age.interval == 0
           || !timer_expired(&existing_topic->max_age)
           || existing_topic->content_len > 0) {

          PRINTF("CREATE attempted on exisitng URL: %s\n", topic_url);
          timer_restart(&existing_topic->max_age);
          REST.set_response_status(response, REST.status.FORBIDDEN);
          return;
        }

        /* If the topic has expiried, reset it to a created state */

        existing_topic->content_format = ct;
#if COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_MALLOC
        free(existing_topic->content);
#endif
        existing_topic->content = NULL;
        existing_topic->content_len = 0;
        existing_topic->max_age.interval = 0;

        REST.get_header_max_age(request, &max_age);
        if(max_age > 0) {
          timer_set(&existing_topic->max_age, max_age * CLOCK_SECOND);
          PRINTF("Max-Age: %u\n", (unsigned int)max_age);
        }

        REST.set_header_location(response, topic_url);
        REST.set_response_status(response, REST.status.CREATED);

        PRINTF("Recreated topic: %s\n", topic_url);

        return;
      }

      clean_topics(); /* May delete the parent topic */

#if COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_MALLOC

      sprintf(attribute_buffer, "ct=%d", ct);
      attributes = (char *)malloc(strlen(attribute_buffer) + 1);

      if(attributes == NULL) {
        PRINTF("Failed to allocate memory for topic\n");
        free(topic_url);
        REST.set_response_status(response, REST.status.INTERNAL_SERVER_ERROR);
        return;
      }

      strncpy(attributes, attribute_buffer, strlen(attribute_buffer) + 1);

      topic_res = (resource_t *)malloc(sizeof(resource_t));

      if(topic_res == NULL) {
        PRINTF("Failed to allocate memory for topic\n");
        free(topic_url);
        free(attributes);
        REST.set_response_status(response, REST.status.INTERNAL_SERVER_ERROR);
        return;
      }

      *topic_res = (resource_t){NULL, NULL, IS_OBSERVABLE, attributes,
                                ps_get_handler, ps_post_handler, ps_put_handler, ps_delete_handler, { NULL } };
      created_topic = (topic_t *)malloc(sizeof(topic_t));

      if(created_topic == NULL) {
        PRINTF("Failed to allocate memory for topic\n");
        free(topic_url);
        free(attributes);
        free(topic_res);
        REST.set_response_status(response, REST.status.INTERNAL_SERVER_ERROR);
        return;
      }
#elif COAP_PUBSUB_BROKER_MEMORY_ALLOC == MEM_STATIC
      created_topic = get_free_topic();

      attributes = created_topic->data_res_attributes;
      sprintf(attributes, "ct=%d", ct);

      topic_res = &created_topic->data_resource;
      *topic_res = (resource_t){NULL, NULL, IS_OBSERVABLE, attributes,
                                ps_get_handler, ps_post_handler, ps_put_handler, ps_delete_handler, { NULL } };

      strncpy((char *)&created_topic->data_res_url, topic_url, COAP_PUBSUB_BROKER_STATIC_URL_SIZE);
      topic_url = (char *)&created_topic->data_res_url;
#endif

      num_topics++;
      created_topic->content = NULL;
      created_topic->content_len = 0;
      created_topic->resource = topic_res;
      created_topic->content_format = ct;
      list_add(topics, created_topic);
      rest_activate_resource(topic_res, topic_url);
      created_topic->max_age.interval = 0;

      REST.get_header_max_age(request, &max_age);
      if(max_age > 0) {
        timer_set(&created_topic->max_age, max_age * CLOCK_SECOND);
        PRINTF("Max-Age: %u\n", (unsigned int)max_age);
      }

      if(IS_OPTION((coap_packet_t *)request, COAP_OPTION_BLOCK1)) {
        coap_set_header_block1(response, ((coap_packet_t *)request)->block1_num, 0,
                               ((coap_packet_t *)request)->block1_size);
      }

      REST.set_header_location(response, topic_url);
      REST.set_response_status(response, REST.status.CREATED);

      PRINTF("New topic: %s\n", topic_url);
    } else {
      PRINTF("CREATE: Invalid payload\n");
      REST.set_response_status(response, REST.status.BAD_REQUEST);
    }
  } else {
    /* This shouldn't happen, since unregistered resources are handled by the rest engine. */
    PRINTF("CREATE: Topic data structure missing\n");
    REST.set_response_status(response, REST.status.NOT_FOUND);
  }
#endif
/*---------------------------------------------------------------------------*/
}
/* Handles the PUBLISH interface */
static void
ps_put_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  size_t payload_len, request_url_len;
  unsigned int ct;
  const uint8_t *payload = NULL;
  const char *request_url = NULL;
  uint32_t max_age;
  topic_t *topic;
  uint32_t block_num;
  uint8_t block_more = 0;
  uint16_t block_size = 0;
  uint32_t block_offset = 0;

  clean_topics();
  request_url_len = REST.get_url(request, &request_url);
  topic = find_topic(request_url, request_url_len);

  if(topic) {
    payload_len = REST.get_request_payload(request, &payload);
    PRINTF("PUBLISH on: %s, content: %s\n", topic->resource->url, payload);

    REST.get_header_content_type(request, &ct);

    if(topic->content_format != ct) {
      PRINTF("PUBLISH: Mismatched content format ct=%u, expected ct=%u\n", ct, topic->content_format);
      REST.set_response_status(response, REST.status.BAD_REQUEST);
      return;
    }

    coap_get_header_block1(request, &block_num, &block_more, &block_size, &block_offset);

    if(payload_len + block_offset > COAP_PUBSUB_MAX_CONTENT_LEN) {
      payload_len = COAP_PUBSUB_MAX_CONTENT_LEN - block_offset;
    }

    if(payload_len + block_offset == topic->content_len && !block_more) {

      memcpy(topic->content + block_offset, payload, payload_len);
    } else {

#if COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_MALLOC
      /* Using free + malloc since some toolchains do not support realloc */
      free(topic->content);

      if(block_more) {
        /* We don't know the total size, so allocate max possible. */
        topic->content = (uint8_t *)malloc(sizeof(uint8_t) * COAP_PUBSUB_MAX_CONTENT_LEN);
      } else {
        topic->content = (uint8_t *)malloc(sizeof(uint8_t) * payload_len);
      }

      if(topic->content == NULL && (payload_len != 0 || block_more)) {
        PRINTF("Failed to allocate memory for content\n");
        REST.set_response_status(response, REST.status.INTERNAL_SERVER_ERROR);
        return;
      }
#elif COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_STATIC
      topic->content = (uint8_t *)&topic->data_content;
#endif
      topic->content_len = payload_len + block_offset;
      memcpy(topic->content + block_offset, payload, payload_len);
    }

    topic->max_age.interval = 0;

    REST.get_header_max_age(request, &max_age);
    if(max_age > 0) {
      timer_set(&topic->max_age, max_age * CLOCK_SECOND);
      PRINTF("Max-Age: %u\n", (unsigned int)max_age);
    }

    if(block_size > 0) {
      PRINTF("%u, %u, %u\n", (unsigned int)block_num, block_more, block_size);
      coap_set_header_block1(response, block_num, block_more, block_size);
    }

    if(block_more) {
      REST.set_response_status(response, CONTINUE_2_31);
      PRINTF("Content block %u received\n", (unsigned int)block_num);
    } else {
      REST.notify_subscribers(topic->resource);
      REST.set_response_status(response, REST.status.CHANGED);
      PRINTF("Content published\n");
    }
  } else {
    /* If clean_topics() removed the topic */
    REST.set_response_status(response, REST.status.NOT_FOUND);
  }
}
/*---------------------------------------------------------------------------*/
/* Handles the READ, SUBSCRIBE and UNSUBSCRIBE interfaces */
static void
ps_get_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  size_t request_url_len;
  const char *request_url = NULL;
  topic_t *topic;

  request_url_len = REST.get_url(request, &request_url);
  topic = find_topic(request_url, request_url_len);

  if(topic) {

    if(topic->content != NULL) {
      if(topic->max_age.interval == 0 || !timer_expired(&topic->max_age)) {

        if(topic->content_len - *offset > preferred_size) {
          REST.set_response_payload(response, topic->content + *offset, preferred_size);
        } else {
          REST.set_response_payload(response, topic->content + *offset, topic->content_len);
        }
        PRINTF("Get on: %s, content: %s, length: %u\n", topic->resource->url,
               topic->content, (unsigned int)(topic->content_len - *offset));

        if(topic->max_age.interval > 0) {
          REST.set_header_max_age(response, (uint32_t)(timer_remaining(&topic->max_age) / CLOCK_SECOND));
        }

        REST.set_header_content_type(response, topic->content_format);

        REST.set_response_status(response, REST.status.OK); /* 2.05 Content */
        return;
      } else {
        PRINTF("Stale content\n");
      }
    }
    PRINTF("Get on: %s, no content\n", topic->resource->url);
    REST.set_response_status(response, REST.status.CHANGED); /* 2.04 No Content */
  } else {
    /* This shouldn't happen since unregistered resources are handled by the rest engine. */
    PRINTF("Get: Topic data structure missing\n");
    REST.set_response_status(response, REST.status.NOT_FOUND);
  }
}
/*---------------------------------------------------------------------------*/
/* Handles the REMOVE interface */
static void
ps_delete_handler(void *request, void *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  size_t request_url_len;
  const char *request_url = NULL;
  topic_t *topic;

  request_url_len = REST.get_url(request, &request_url);
  topic = find_topic(request_url, request_url_len);

  if(topic) {
#if COAP_PUBSUB_BROKER_ALLOW_CLIENT_TOPIC_DELETION == 1

    PRINTF("Topic %s deleted\n", topic->resource->url);
    coap_remove_observer_by_uri(NULL, 0, topic->resource->url);
    list_remove(rest_get_resources(), topic->resource);

#if COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_MALLOC
    free((void *)topic->resource->url);
    free((void *)topic->resource->attributes);
    free(topic->resource);

    list_remove(topics, topic);
    free(topic->content);
    free(topic);

#elif COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_STATIC
    list_remove(topics, topic);
    topic->resource = NULL;
#endif

    num_topics--;
    REST.set_response_status(response, REST.status.DELETED);

#else
    PRINTF("Attempted to delete topic %s\n", topic->resource->url);
    REST.set_response_status(response, REST.status.UNAUTHORIZED);

#endif /* ALLOW_CLIENT_TOPIC_DELETION */
  } else {
    /* This shouldn't happen, since unregistered resources are handled by the rest engine. */
    PRINTF("DELETE: Topic data structure missing\n");
    REST.set_response_status(response, REST.status.NOT_FOUND);
  }
}
/*---------------------------------------------------------------------------*/
list_t *
coap_pubsub_broker_get_topic_list()
{
  return &topics;
}
/*---------------------------------------------------------------------------*/
topic_t *
coap_pubsub_broker_create_topic(char *url, coap_content_format_t resource_type)
{
  resource_t *topic_res = NULL;
  topic_t *created_topic;
  char *res_url, *attributes = NULL;

#if COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_MALLOC
  char attribute_buffer[16] = { 0 };

  res_url = (char *)malloc(sizeof(char) * (strlen(url) + 1));

  if(res_url == NULL) {
    PRINTF("Failed to allocate memory for topic\n");
    return NULL;
  }

  strncpy(res_url, url, strlen(url));

  sprintf(attribute_buffer, "ct=%d", (int)resource_type);
  attributes = (char *)malloc(sizeof(char) * (strlen(attribute_buffer) + 1));

  if(attributes == NULL) {
    PRINTF("Failed to allocate memory for topic\n");
    free(res_url);
    return NULL;
  }

  strncpy(attributes, attribute_buffer, strlen(attribute_buffer) + 1);

  topic_res = (resource_t *)malloc(sizeof(resource_t));

  if(topic_res == NULL) {
    PRINTF("Failed to allocate memory for topic\n");
    free(res_url);
    free(attributes);
    return NULL;
  }

  *topic_res = (resource_t){NULL, NULL, IS_OBSERVABLE, attributes,
                            ps_get_handler, ps_post_handler, ps_put_handler, ps_delete_handler, { NULL } };
  created_topic = (topic_t *)malloc(sizeof(topic_t));

  if(created_topic == NULL) {
    PRINTF("Failed to allocate memory for topic\n");
    free(res_url);
    free(attributes);
    free(topic_res);
    return NULL;
  }

#elif COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_STATIC
  created_topic = get_free_topic();

  if(created_topic == NULL) {
    return NULL;
  }

  sprintf(created_topic->data_res_attributes, "ct=%d", (int)resource_type);
  topic_res = &created_topic->data_resource;
  *topic_res = (resource_t){NULL, NULL, IS_OBSERVABLE, attributes,
                            ps_get_handler, ps_post_handler, ps_put_handler, ps_delete_handler, { NULL } };

  strncpy((char *)&created_topic->data_res_url, url, COAP_PUBSUB_BROKER_STATIC_URL_SIZE);
  res_url = (char *)&created_topic->data_res_url;

#endif
  num_topics++;
  created_topic->resource = topic_res;
  created_topic->content_format = resource_type;
  list_add(topics, created_topic);
  rest_activate_resource(topic_res, res_url);

  return created_topic;
}
/*---------------------------------------------------------------------------*/
void
coap_pubsub_broker_remove_topic(topic_t *topic)
{
  coap_remove_observer_by_uri(NULL, 0, topic->resource->url);
  list_remove(rest_get_resources(), topic->resource);

#if COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_MALLOC
  free((void *)topic->resource->url);
  free((void *)topic->resource->attributes);
  free(topic->resource);

  list_remove(topics, topic);
  free(topic->content);
  free(topic);

#elif COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_STATIC
  list_remove(topics, topic);
  topic->resource = NULL;
#endif

  num_topics--;
}
/*---------------------------------------------------------------------------*/
void
coap_pubsub_broker_set_topic_content(topic_t *topic, uint8_t *content, size_t content_len)
{
  if(content_len == topic->content_len) {

    memcpy(topic->content, content, content_len);
  } else {

#if COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_MALLOC
    /* Using free + malloc since some toolchains do not support realloc */
    free(topic->content);
    topic->content = (uint8_t *)malloc(sizeof(uint8_t) * content_len);

    if(topic->content == NULL && content_len != 0) {
      PRINTF("Failed to allocate memory for content\n");
      return;
    }
#elif COAP_PUBSUB_BROKER_MEMORY_ALLOC == COAP_PUBSUB_MEM_STATIC
    topic->content = topic->data_content;

#endif
    topic->content_len = content_len;
    memcpy(topic->content, content, content_len);
  }
  REST.notify_subscribers(topic->resource);
}
/*---------------------------------------------------------------------------*/
size_t
coap_pubsub_broker_get_topic_content(topic_t *topic, uint8_t **content)
{
  if(topic && topic->content && topic->content_len > 0) {
    *content = topic->content;
    return topic->content_len;
  }

  *content = NULL;
  return 0;
}
