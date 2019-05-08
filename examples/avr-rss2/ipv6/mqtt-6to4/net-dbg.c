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
 *         Sets up some commands for the border router
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include "net/rpl/rpl.h"
#include "net/rpl/rpl-private.h"
#include "net/mac/handler-802154.h"
#include "net/mac/framer-802154.h"
#include "net/net-control.h"
#include <string.h>
#include <stdlib.h>
#include "net/packetbuf.h"
#include "packetutils.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

void
dbg_rpl(void)
{
    if(rpl) {
      rpl_dag_t *dag = rpl_get_any_dag();
      rpl_instance_t *instance;
      if(dag != NULL) {
        instance = dag->instance;
        printf("DAG: rank:%d version:%d\n", dag->rank, dag->version);
        printf("    dio_sent: %d dio_recv: %d dio_totint: %d\n",
               instance->dio_totsend, instance->dio_totrecv,
               instance->dio_totint);
        /* printf("    dao_sent: %d dao_recv: %d dao_acc: %d dao_nack_sent: %d dao_nopath: %d\n", */
        /*        instance->dao_totsend, instance->dao_totrecv, */
        /*        instance->dao_totaccepted, instance->dao_nack_totsend, */
        /*        instance->dao_nopath_totsend); */
      } else {
        printf("No DAG found - not joined yet\n");
      }

      rpl_print_neighbor_list();
}

void
dbg_routes(void)
{
      uip_ds6_route_t *r;
      uip_ds6_de	    frt_t *defrt;
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
          printf(" %d s\n", r->state.lifetime);
        } else {
          printf(" >600 s\n");
        }
      }
      printf("\n");
}

void
dbg_nbr(void)
{
      uip_ds6_nbr_t *nbr;
      rpl_instance_t *def_instance;
      const uip_lladdr_t *lladdr;
      rpl_parent_t *p;
      uint16_t rank;

      def_instance = rpl_get_default_instance();

      printf("Neighbors IPv6         \t  sec  state       rank      RPL fg\n");
      for(nbr = nbr_table_head(ds6_neighbors);
          nbr != NULL;
          nbr = nbr_table_next(ds6_neighbors, nbr)) {
        uip_debug_ipaddr_print(&nbr->ipaddr);
        lladdr = uip_ds6_nbr_get_ll(nbr);
        p = nbr_table_get_from_lladdr(rpl_parents, (const linkaddr_t*)lladdr);
        rank = p != NULL ? p->rank : 0;
        if(stimer_expired(&nbr->reachable)) {
          printf("\t %5c ", '-');
        } else {
          printf("\t %5lu ", stimer_remaining(&nbr->reachable));
        }
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
        printf("\n");
      }
      printf("\n");
}
