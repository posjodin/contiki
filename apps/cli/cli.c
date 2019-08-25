/*
 * Copyright (c) 2011-2016, Swedish Institute of Computer Science.
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
 */

/**
 * \file
 *         Minimal and simple serial line API for ipv6 and RPL monitoring
 *         See help command to get an overview
 *
 * \Author
 *         Robert Olsson
 *
 * \Used code from border-router.c with authors
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *
 * \Used code from pdr.c with authors:
 *         Atis Elsts       <atis.elsts@bristol.ac.uk>
 *         Christian Rohner <christian.rohner@it.uu.se>
 *         Robert Olsson    <roolss@kth.se>
 */

#define CLI_VERSION "0.9-2017-03-13\n"

#include "contiki.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include "net/mac/framer-802154.h"
#include "net/link-stats.h"
#include <string.h>
#include <stdlib.h>
#include "net/packetbuf.h"
#include "dev/serial-line.h"
#include "dev/i2c.h"
#include "sys/process.h"

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#if CONTIKI_TARGET_AVR_RSS2
#define radio_get_rssi    rf230_rssi
#include <avr/wdt.h>
#include "dev/pms5003/pms5003.h"
#include "adc.h"
#include "radio/rf230bb/rf230bb.h"
#endif

#ifdef CLI_CONF_COMMAND_PROMPT
#define CLI_COMMAND_PROMPT CLI_CONF_COMMAND_PROMPT
#else
#define CLI_COMMAND_PROMPT  "> "
#endif

#ifdef CLI_CONF_PROJECT
#define CLI_PROJECT CLI_CONF_PROJECT
#else
#define CLI_PROJECT  ""
#endif

static const char *delim = " \t\r,";
#define END_OF_FILE 26
static uint8_t eof = END_OF_FILE;
static uint8_t channel;

#define READY_PRINT_INTERVAL (CLOCK_SECOND * 5)

extern double no2(void);

#if CLI_STANDALONE
PROCESS(cli, "cli app");
AUTOSTART_PROCESSES(&cli);
#endif

static void
print_help(void)
{
/**
 * We don't show uptime and version to save space in CLI reply
 * Otherwise, if they need to be shown, we show them first
 */
//  printf("Uptime %lu sec\n", clock_seconds());
//  printf("PROJECT: %s\nCLI: %s", CLI_PROJECT, CLI_VERSION);
  printf("sh dag -- DODAG info\n");
  printf("sh neighbor -- neighbor list\n");
  printf("sh routes\n");
  printf("sh stats\n");
  printf("sh channel\n");
  printf("sh txpower\n");
  printf("sh frame_retries -- 1+MAX_FRAME_RETRIES\n");
  printf("sh csma_retries -- MAX_CSMA_RETRIES\n");
  printf("sh reboot\n");
  printf("sh uptime\n");
  printf("sh version\n");
  printf("set channel -- [11-26]\n");
  printf("set debug -- set debug info\n");
  printf("txpower -- [0-15]\n");
  printf("frame_retries -- set 1+MAX_FRAME_RETRIES\n");
  printf("csma_retries -- set MAX_CSMA_RETRIES\n");
  printf("i2c -- probe i2c bus\n");
  printf("repair -- global dag repair\n");
  printf("upgr -- reboot via bootloader\n");
  printf("sh pm_warmup -- PMS5003 warmup time (sec)\n");
  printf("sh pm_period -- PMS5003 sample period (sec)\n");  
  printf("sh no2 -- ug/m**3\n");  
  printf("help -- this menu\n");
}
static void
radio_get_frame_retries(void)
{
  static uint8_t frame_retries;
  frame_retries = rf230_get_frame_retries();
  if (frame_retries == 16)
    printf("cannot read value\n");
  else
    printf("1+MAX_FRAME_RETRIES: %u\n", frame_retries+1);
}
static void
radio_set_frame_retries(uint8_t frame_retries)
{
  if ((frame_retries >= 0) && (frame_retries <= 16)) {
    rf230_set_frame_retries(frame_retries);
    printf("set 1+MAX_FRAME_RETRIES = %u\n", frame_retries);
  }
  else
    printf("Invalid value: MAX_FRAME_RETRIES must be 0-16\n");
}
static void
radio_get_csma_retries(void)
{
  static uint8_t csma_retries;
  csma_retries = rf230_get_csma_retries();
  if (csma_retries == 6)
    printf("cannot read value\n");
  else
    printf("MAX_CSMA_RETRIES: %u\n", csma_retries);
}
static void
radio_set_csma_retries(uint8_t csma_retries)
{
  if (csma_retries == 6) 
    printf("Invalid value: 6 is reseved and cannot be used\n");
  else if ((csma_retries >= 0) && (csma_retries <= 7)) {
    rf230_set_csma_retries(csma_retries);
    printf("set MAX_CSMA_RETRIES = %u\n", csma_retries);
  }
  else
    printf("Invalid value: MAX_CSMA_RETRIES must be 0-7\n");
}
static uint8_t
radio_get_txpower(void)
{
  int txpower;
  if(NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &txpower) ==
     RADIO_RESULT_OK) {
    return txpower;
  }
  printf("Err get_txpower\n");
  return -1;
}
static void
radio_set_txpower(int txpower)
{
  if(NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, txpower) ==
     RADIO_RESULT_OK) {
    printf("SET TX_PWR=%d\n", txpower);
  } else {
    printf("Err set_txpower=%d\n", txpower);
  }
}
static uint8_t
radio_get_channel(void)
{
  radio_value_t chan;

  if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan) ==
     RADIO_RESULT_OK) {
    return chan;
  }
  printf("Err get_chan\n");
  return 0;
}
static void
radio_set_channel(uint8_t channel)
{
  if(NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channel) ==
     RADIO_RESULT_OK) {
    printf("Err set_chan=%d\n", channel);
    return;
  }
}
static const char *
get_nbr_state_name(uint8_t state)
{
  switch(state) {
  case NBR_INCOMPLETE:
    return "INCOMPLETE";
  case NBR_REACHABLE:
    return "REACHABLE";
  case NBR_STALE:
    return "STALE";
  case NBR_DELAY:
    return "DELAY";
  case NBR_PROBE:
    return "PROBE";
  default:
    return "unknown";
  }
}
static void
rpl_print_neighbor_list_cli(void)
{
  if(default_instance != NULL && default_instance->current_dag != NULL &&
      default_instance->of != NULL) {
    int curr_dio_interval = default_instance->dio_intcurrent;
    int curr_rank = default_instance->current_dag->rank;
    rpl_parent_t *p = nbr_table_head(rpl_parents);
    clock_time_t clock_now = clock_time();

    printf("RPL: MOP %u OCP %u rank %u dioint %u, nbr count %u\n",
        default_instance->mop, default_instance->of->ocp, curr_rank, curr_dio_interval, uip_ds6_nbr_num());
    while(p != NULL) {
      const struct link_stats *stats = rpl_get_parent_link_stats(p);
      printf("RPL: nbr %02x%02x %5u, %5u => %5u -- %2u %c%c (last tx %u min ago)\n",
          rpl_get_parent_ipaddr(p)->u8[14],
          rpl_get_parent_ipaddr(p)->u8[15],
          p->rank,
          rpl_get_parent_link_metric(p),
          rpl_rank_via_parent(p),
          stats != NULL ? stats->freshness : 0,
          link_stats_is_fresh(stats) ? 'f' : ' ',
          p == default_instance->current_dag->preferred_parent ? 'p' : ' ',
          (unsigned)((clock_now - stats->last_tx_time) / (60 * CLOCK_SECOND))
      );
      p = nbr_table_next(rpl_parents, p);
    }
    printf("RPL: end of list\n");
  }
}
static void
show_dag(void)
{
  rpl_dag_t *dag;
  rpl_instance_t *instance;
  uip_ipaddr_t ipaddr;
  rpl_parent_t *parent;
  rpl_instance_t *end;

  uip_ip6addr(&ipaddr, 0xfe80, 0, 0, 0, 0, 0, 0, 0);
  /* uip_ds6_set_addr_iid(&ipaddr, (uip_lladdr_t *)addr); */

  for(instance = &instance_table[0], end = instance + RPL_MAX_INSTANCES; instance < end; ++instance) {
    if(instance->used == 1) {
      parent = rpl_find_parent_any_dag(instance, &ipaddr);
      if(parent != NULL) {

        /* prinf("DAG %u: \n"); */
      }
    }
  }

  dag = rpl_get_any_dag();
  if(dag != NULL) {
    printf("DAG_ID: ");
    uip_debug_ipaddr_print(&dag->dag_id);

    printf("\nDAG: rank:%u version:%u\n", dag->rank, dag->version);
    printf("DAG: grounded:%u preferenc:%u\n", dag->grounded, dag->preference);
    printf("DAG: used:%u joined:%u\n", dag->used, dag->joined);
    printf("DAG: lifetime:%lu\n", dag->lifetime);

#if 0
    pp = dag > preferred_parent;
    if(pp) {
      pp->dag;
      if(ppd) {
        printf("DAG_ID: ");
        uip_debug_ipaddr_print(&dag->dag_id);
      }
    }
#endif

    instance = dag->instance;
    printf("INSTANCE: instance_id: %u used; %u\n",
           instance->instance_id, instance->used);

#if RPL_CONF_STATS
    printf("    dio_sent: %u dio_recv: %u dio_totint: %u\n",
           instance->dio_totsend, instance->dio_totrecv,
           instance->dio_totint);
#endif
  } else {
    printf("No DAG joined\n");
  }
  rpl_print_neighbor_list_cli();
}
static void
show_rpl_stats(void)
{
#if RPL_CONF_STATS
  printf("rpl.mem_overflows=%-u\n", rpl_stats.mem_overflows);
  printf("rpl.local_repairs=%-u\n", rpl_stats.local_repairs);
  printf("rpl.global_repairs=%-u\n", rpl_stats.global_repairs);
  printf("rpl.malformed_msgs=%-u\n", rpl_stats.malformed_msgs);
  printf("rpl.resets=%-u\n", rpl_stats.resets);
  printf("rpl.parent_switch=%-u\n", rpl_stats.parent_switch);
  printf("rpl.forward_errors=%-u\n", rpl_stats.forward_errors);
  printf("rpl.loop_errors=%-u\n", rpl_stats.loop_errors);
  printf("rpl.loop_warnings=%-u\n", rpl_stats.loop_warnings);
  printf("rpl.root_repairs=%-u\n", rpl_stats.root_repairs);
#endif
}
static void
show_routes(void)
{
  uip_ds6_route_t *r;
  uip_ds6_defrt_t *defrt;
  uip_ipaddr_t *ipaddr;
  defrt = NULL;
  if((ipaddr = uip_ds6_defrt_choose()) != NULL) {
    defrt = uip_ds6_defrt_lookup(ipaddr);
  }
  if(defrt != NULL) {
    printf("DefRT: :: -> ");
    uip_debug_ipaddr_print(&defrt->ipaddr);
    if(defrt->isinfinite) {
      printf(" (infinite lifetime)\n");
    } else {
      printf(" lifetime: %lu sec\n", stimer_remaining(&defrt->lifetime));
    }
  } else {
    printf("DefRT: :: -> NULL\n");
  }

  printf("Routes:\n");
  for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
    printf(" ");
    uip_debug_ipaddr_print(&r->ipaddr);
    printf(" -> ");
    if(uip_ds6_route_nexthop(r) != NULL) {
      uip_debug_ipaddr_print(uip_ds6_route_nexthop(r));
    } else {
      printf("NULL");
    }
    if(r->state.lifetime < 600) {
      printf(" %ld s\n", r->state.lifetime);
    } else {
      printf(" >600 s\n");
    }
  }
  printf("\n");
}
static void
show_neighbors(void)
{
  uip_ds6_nbr_t *nbr;
  rpl_instance_t *def_instance;
  const uip_lladdr_t *lladdr;
  rpl_parent_t *p;
  uint16_t rank;
  const struct link_stats *ls;

  def_instance = rpl_get_default_instance();

  printf("Neighbors IPv6         \t  Sec  State       Rank      RPL Flg   ETX  RSSI Freshness\n");
  for(nbr = nbr_table_head(ds6_neighbors);
      nbr != NULL;
      nbr = nbr_table_next(ds6_neighbors, nbr)) {
    uip_debug_ipaddr_print(&nbr->ipaddr);
    lladdr = uip_ds6_nbr_get_ll(nbr);
    p = nbr_table_get_from_lladdr(rpl_parents, (const linkaddr_t *)lladdr);
    rank = p != NULL ? p->rank : 0;
#if UIP_ND6_SEND_NS || UIP_ND6_SEND_RA
    if(stimer_expired(&nbr->reachable)) {
      printf("\t %5c ", '-');
    } else {
      printf("\t %5lu ", stimer_remaining(&nbr->reachable));
    }
#else
    printf("\t  N/A  ");
#endif
    printf("%-10s ", get_nbr_state_name(nbr->state));
    printf(" %d.%02d[%3d] %c",
           rank / 128, (int)(rank * 100L / 128) % 100, rank,
           p != NULL ? 'P' : ' ');
    if(def_instance != NULL && def_instance->current_dag != NULL &&
       def_instance->current_dag->preferred_parent == p) {
      printf("*");
    } else {
      printf(" ");
    }
    if(p != NULL) {
      printf(" %2x", p->flags);
    } else {
      printf("   ");
    }
    lladdr = uip_ds6_nbr_get_ll(nbr);
    ls = link_stats_from_lladdr((const linkaddr_t *)lladdr);
    printf("     %-4u  %-2u    %-u", ls->etx, ls->rssi, ls->freshness);
    printf("\n");
  }
  printf("\n");
}
static void
show_net_all(void)
{
  show_dag();
  show_routes();
  show_neighbors();
}
static int
cmd_chan(uint8_t verbose)
{
  uint8_t tmp;
  char *p = strtok(NULL, delim);

  if(p) {
    tmp = atoi((char *)p);
    if(tmp >= 11 && tmp <= 26) {
      channel = tmp;
      radio_set_channel(channel);
    } else {
      printf("Invalid chan=%d\n", tmp);
      return 0;
    }
  }
  if(verbose) {
    printf("chan=%d\n", radio_get_channel());
  }
  return 1;
}

static void
debug_cmd(char *p)
{
  printf("To be added\n");
}
void
handle_serial_input(const char *line)
{
  char *p;
  /* printf("in: '%s'\n", line); */
  p = strtok((char *)&line[0], (const char *)delim);

  if(!p) {
    return;
  }

  printf("\n");

  /* Show commands */
  if(!strcmp(p, "sh") || !strcmp(p, "sho") || !strcmp(p, "show")) {
    p = strtok(NULL, (const char *)delim);
    if(p) {
      if(!strcmp(p, "d") || !strcmp(p, "da") || !strcmp(p, "dag")) {
        show_dag();
      } else if(!strcmp(p, "s") || !strcmp(p, "stat") || !strcmp(p, "stats")) {
        show_rpl_stats();
      } else if(!strcmp(p, "r") || !strcmp(p, "ro") || !strcmp(p, "route")) {
        show_routes();
      } else if(!strcmp(p, "n") || !strcmp(p, "ne") || !strcmp(p, "neigh")) {
        show_neighbors();
      } else if(!strcmp(p, "ch") || !strcmp(p, "chan")) {
        cmd_chan(1);
      } else if(!strcmp(p, "tx") || !strcmp(p, "txp") || !strcmp(p, "txpower")) {
        printf("TX_PWR: %d\n", radio_get_txpower());
      } else if(!strcmp(p, "fr") || !strcmp(p, "frame") || !strcmp(p, "frame_retries")) {
	radio_get_frame_retries();
      } else if(!strcmp(p, "cs") || !strcmp(p, "csma") || !strcmp(p, "csma_retries")) {
	radio_get_csma_retries();
      } else if(!strcmp(p, "re") || !strcmp(p, "reb") || !strcmp(p, "reboot")) {
#if CONTIKI_TARGET_AVR_RSS2
	void print_mcusr(uint8_t reg);
	print_mcusr(GPIOR0);
#endif
      } else if(!strcmp(p, "up") || !strcmp(p, "upt") || !strcmp(p, "uptime")) {
	printf("Uptime %lu sec\n", clock_seconds());
      } else if(!strcmp(p, "v") || !strcmp(p, "ver")) {
        printf("PROJECT: %s\nCLI: %s", CLI_PROJECT, CLI_VERSION);
#ifdef PMS5003_CONF_SERIAL_I2C
      } else if(!strcmp(p, "pm_w") || !strcmp(p, "pm_warmup")) {
        printf("PMS5003 warmup: %d sec\n", pms5003_get_warmup_interval());
      } else if(!strcmp(p, "pm_p") || !strcmp(p, "pm_period")) {
        printf("PMS5003 period: %d sec\n", pms5003_get_sample_period());
#endif
      }
    }
  }
  /* Set commands */
  else if(!strcmp(p, "sh") || !strcmp(p, "sho") || !strcmp(p, "show")) {
    p = strtok(NULL, (const char *)delim);
    if(p) {
      if(!strcmp(p, "de") || !strcmp(p, "debug")) {
        debug_cmd(p);
      }
    }
  }
  /* Misc commands */
  else if(!strcmp(p, "li") || !strcmp(p, "list")) {
    show_net_all();
  }

#ifdef CONTIKI_TARGET_AVR_RSS2
  else if(!strcmp(p, "i2c")) {
    printf("I2C: ");
    i2c_probed = i2c_probe();
    printf("\n");
  } else if(!strcmp(p, "upgr") || !strcmp(p, "upgrade")) {
    printf("OK\n");
    printf("%c", eof);
    cli();
    wdt_enable(WDTO_15MS);
    while(1) ;
  } else if(!strcmp(p, "tx") || !strcmp(p, "txp") || !strcmp(p, "txpower")) {
    p = strtok(NULL, (const char *)delim);
    if(p) {
	int txpower = atoi(p);
        radio_set_txpower(txpower);
    }
  } else if(!strcmp(p, "fr") || !strcmp(p, "frame") || !strcmp(p, "frame_retries")) {
    p = strtok(NULL, (const char *)delim);
    if(p) {
	int frame_retries = atoi(p);
        radio_set_frame_retries((uint8_t) frame_retries);
    }
  } else if(!strcmp(p, "cs") || !strcmp(p, "csma") || !strcmp(p, "csma_retries")) {
    p = strtok(NULL, (const char *)delim);
    if(p) {
	int csma_retries = atoi(p);
        radio_set_csma_retries((uint8_t) csma_retries);
    }
#ifdef PMS5003_CONF_SERIAL_I2C
  } else if(!strcmp(p, "pm_w") || !strcmp(p, "pm_warmup")) {
    p = strtok(NULL, (const char *)delim);
    if (p) {
      int warmup = atoi(p);
      pms5003_config_warmup_interval(warmup);
    }
  } else if(!strcmp(p, "pm_p") || !strcmp(p, "pm_period")) {
    p = strtok(NULL, (const char *)delim);
    if (p) {
      int period = atoi(p);
      pms5003_config_sample_period(period);
    }
#endif
  }

#endif
  else if(!strcmp(p, "rep") || !strcmp(p, "repair")) {
    rpl_repair_root(RPL_DEFAULT_INSTANCE);
  } else if(!strcmp(p, "help") || !strcmp(p, "h")) {
    print_help();
  } else { printf("Illegal command '%s'\n", p);
  }
  printf(CLI_COMMAND_PROMPT);
}
#ifdef CLI_STANDALONE

static struct etimer periodic;

PROCESS_THREAD(cli, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();

  etimer_set(&et, CLOCK_SECOND);

  for(;;) {
    PROCESS_WAIT_EVENT();

    /* printf("event %u (%u) at %u, data %p\n", (uint16_t)ev, (uint16_t)serial_line_event_message, currentState, data); */
    printf("event %u (%u) at  data %p\n", (uint16_t)ev, (uint16_t)serial_line_event_message, data);

    if(etimer_expired(&periodic)) {
      etimer_set(&periodic, READY_PRINT_INTERVAL);
    }
    if(ev == PROCESS_EVENT_POLL) {
      etimer_set(&periodic, READY_PRINT_INTERVAL);
    }
  }
  PROCESS_END();
}

#endif
