#include "contiki.h"
#include "net/link-stats.h"
#include "net/rpl/rpl-private.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-nd6.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/nbr-table.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "sys/ctimer.h"

#include <limits.h>
#include <string.h>

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#define PUTFMT(...) { \
		len = snprintf(buf_ptr, remaining, __VA_ARGS__);	\
		if (len < 0 || len >= remaining) { \
			printf("Line %d: Buffer too short. Have %d, need %d + \\0\n", __LINE__, remaining, len); \
			return bufsize + len; \
		} \
		remaining -= len; \
		buf_ptr += len; \
	}

#define PUTIPADDR(ipaddr) PUTFMT("\":%02x%02x\"", ipaddr->u8[14], ipaddr->u8[15])
int
mqtt_rpl_pub(char *buf, int bufsize)  {
  int remaining = bufsize;
  char *buf_ptr = buf;
  int len;

  rpl_dag_t *dag;
  int i;

  rpl_instance_t *inst = default_instance;

  if(inst != NULL && inst->current_dag != NULL &&
      inst->of != NULL) {

    PUTFMT("{\"n\":\"rpl;inst\", \"vj\":{\"inst\":%d, \"mop\":%d, \"ocp\":%d}}",
	   inst->instance_id, inst->mop, inst->of->ocp);

    for(i = 0; i < RPL_MAX_DAG_PER_INSTANCE; ++i) {
      dag = &inst->dag_table[i];
      if(dag->used) {
	PUTFMT(",{\"n\":\"rpl;dag\",\"vj\":{\"root\":\":%02x%02x\", \"inst\":%d, \"rank\":%d, \"ver\":%d, \"pref\":%d, \"status\":\"%s%s\"",
	       dag->dag_id.u8[14], dag->dag_id.u8[15],
	       inst->instance_id,
	       dag->rank, dag->version, dag->preference,
	       dag->grounded ? "g": "",
	       dag->instance && dag->instance->current_dag == dag ? "c": "");
	PUTFMT(", \"parent\":");
	uip_ipaddr_t *paddr = NULL;
	if (dag->preferred_parent) {
	  paddr = rpl_get_parent_ipaddr(dag->preferred_parent);
	}
	if (paddr) {
	  PUTIPADDR(paddr);
	}
	else {
	  PUTFMT("\"-\"");
	}
	PUTFMT("}}");
      }
    }
  }
  else {
    PUTFMT("{\"n\": \"rpl\", \"vs\": \"No DODAG\"}");
  }

  rpl_parent_t *p = nbr_table_head(rpl_parents);
  clock_time_t clock_now = clock_time();

  PUTFMT(",{\"n\":\"rpl;parents\",\"vj\":[");
  int putcomma = 0;
  while(p != NULL) {
    const struct link_stats *stats = rpl_get_parent_link_stats(p);
    if (putcomma)
      PUTFMT(",");
    putcomma = 1;

    PUTFMT("{\"prn\":");
    PUTIPADDR(rpl_get_parent_ipaddr(p));
    PUTFMT(",\"rank\":%u, \"of:link_metric\":%u, \"of:rank_via\":%u, \"fresh\":%u, \"status\":\"%c%c\", \"last_tx (min)\":%u, \"tx_tot_cnt\":%lu, \"tx_ok_cnt\":%lu, \"tx_num_sum\":%lu, \"tx_collision\":%lu, \"tx_deferred\":%lu, \"tx_error\":%lu, \"rssi\":%d, \"lqi\":%u",
	   p->rank,
	   rpl_get_parent_link_metric(p),
	   rpl_rank_via_parent(p),
	   stats != NULL ? stats->freshness : 0,
	   link_stats_is_fresh(stats) ? 'f' : ' ',
	   p == default_instance->current_dag->preferred_parent ? 'p' : ' ',
	   (unsigned)((clock_now - stats->last_tx_time) / (60 * CLOCK_SECOND)),
	   (stats != NULL) ? stats->tx_tot_cnt : 0,
	   (stats != NULL) ? stats->tx_ok_cnt : 0,
	   (stats != NULL) ? stats->tx_num_sum : 0,
	   (stats != NULL) ? stats->tx_collision : 0,
	   (stats != NULL) ? stats->tx_deferred : 0,
	   (stats != NULL) ? stats->tx_error : 0,
	   (stats != NULL) ? stats->rssi : 0,
	   (stats != NULL) ? stats->lqi : 0
	   );
    PUTFMT("}");
    p = nbr_table_next(rpl_parents, p);
  }
  PUTFMT("]}");
  return buf_ptr - buf;
}
