/*
 * Copyright (c) 2012, Thingsquare, http://www.thingsquare.com/.
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
 *
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
 */
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "dev/ppp/ppp.h"
#include "dev/ppp/ipcp.h"

#include "dev/at-radio/at-radio.h"
#include "ip64.h"

#include <string.h>
#include <stdio.h>

#define UIP_IP_BUF        ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

PROCESS(ip64_ppp_process, "IP64 PPP process");

static uip_ipaddr_t last_sender;

#define PPP_6TO4_STATS
#ifdef PPP_6TO4_STATS
uint16_t ppp_6to4good, ppp_6to4bad, ppp_4to6good, ppp_4to6bad;
#endif /* PPP_6TO4_STATS */
/*---------------------------------------------------------------------------*/
/*
 * A node with only an uplink may not get an IPv6 address through autoconf.
 * Assign one statically instead as a translatable address (64:ff9b::/96).
 */

static void
create_translatable_address(uip_ipaddr_t *ipaddr, uip_ip4addr_t *ip4addr) {

  memset(ipaddr, 0, sizeof(*ipaddr));
  ipaddr->u8[0] = 0x00;
  ipaddr->u8[1] = 0x64;  
  ipaddr->u8[2] = 0xff;
  ipaddr->u8[3] = 0x9b;  
  ipaddr->u8[12] = ip4addr->u8[0];
  ipaddr->u8[13] = ip4addr->u8[1];
  ipaddr->u8[14] = ip4addr->u8[2];
  ipaddr->u8[15] = ip4addr->u8[3];
}
/*---------------------------------------------------------------------------*/
static void get_ppp_v4addr(uip_ip4addr_t *addr, uip_ip4addr_t *mask) {
  struct at_radio_status *atstat;

  atstat = at_radio_status();
  *addr = atstat->ip4addr; *mask = atstat->ip4mask;
}
/*---------------------------------------------------------------------------*/
/* 
 * Set IPv4 address to use for source in IP64 translation
 */
static void set_ip64_v4addr() {
  uip_ip4addr_t ip4addr, ip4mask;

  get_ppp_v4addr(&ip4addr, &ip4mask);
  ip64_set_ipv4_address(&ip4addr, &ip4mask);
}
/*---------------------------------------------------------------------------*/
/* 
 * Set a local static IPv6 address 
 */
static void set_local_v6addr() {
  uip_ip4addr_t ip4addr, ip4mask;
  uip_ipaddr_t loc_ipaddr;

  get_ppp_v4addr(&ip4addr, &ip4mask);
  create_translatable_address(&loc_ipaddr, &ip4addr);
  uip_ds6_addr_add(&loc_ipaddr, 0, ADDR_ANYTYPE);

}
/*---------------------------------------------------------------------------*/
void
ip64_ppp_interface_input(uint8_t *packet, uint16_t len)
{
  /* Dummy definition: this function is not actually called, but must
     be here to conform to the ip65-interface.h structure. */
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ip64_ppp_process, ev, data) {
  static struct etimer et;
  PROCESS_BEGIN();

  PROCESS_WAIT_EVENT_UNTIL(ev == at_radio_ev_init);
  etimer_set(&et, 2*CLOCK_SECOND);
  PROCESS_WAIT_UNTIL(etimer_expired(&et));

  at_radio_datamode(NULL);
  etimer_set(&et, 2*CLOCK_SECOND);
  PROCESS_WAIT_UNTIL(etimer_expired(&et));
  
  set_ip64_v4addr();
  /* With only an uplink, we get no address through autoconf. 
   * Set static address instead
   */
  set_local_v6addr();

  ppp_connect();

  while (1) {
    PROCESS_PAUSE();
    ppp_poll();
    if (uip_len > 0) {
      static uint16_t max_uip_len = 0;
      if (uip_len > max_uip_len)
        max_uip_len = uip_len;
      printf("ppp_poll set uip_len %d (%d)\n", uip_len, max_uip_len);
    }
    if (uip_len > 0) {
      if ((ipcp_state & IPCP_TX_UP) && (ipcp_state & IPCP_RX_UP)) {
        /* Save the last sender received over PPP to avoid bouncing the
           packet back if no route is found */
        uip_ipaddr_copy(&last_sender, &UIP_IP_BUF->srcipaddr);

        uint16_t len = ip64_4to6(&uip_buf[UIP_LLH_LEN], uip_len, 
                                 ip64_packet_buffer);
        printf("4to6 len %d (uip_len %d)\n", len, uip_len);
        if(len > 0) {
#ifdef PPP_6TO4_STATS
          ppp_4to6good++;
#endif /* PPP_6TO4_STATS */
          memcpy(&uip_buf[UIP_LLH_LEN], ip64_packet_buffer, len);
          uip_len = len;
          tcpip_input();
        } else {
#ifdef PPP_6TO4_STATS
          ppp_4to6bad++;
#endif /* PPP_6TO4_STATS */
          uip_clear_buf();
        }
      }
      else {
        /* Link not up - discard */
        uip_clear_buf();
      }
    }
  }
  PROCESS_END();  
}

/*---------------------------------------------------------------------------*/
static void
init(void)
{
  ppp_init();
  process_start(&ip64_ppp_process, NULL);
}
/*---------------------------------------------------------------------------*/
static int
output(void)
{
  int len;

  if (UIP_IP_BUF->vtc != (6 << 4)) {
    printf("ip64_ppp output: bad ip version %02x\n", UIP_IP_BUF->vtc);
    uip_clear_buf();
    return 0;
  }
  printf("ip64-ppp-interface: from ");
  PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
  printf(" to ");
  PRINT6ADDR(&UIP_IP_BUF->destipaddr);

  if(uip_ipaddr_cmp(&last_sender, &UIP_IP_BUF->srcipaddr)) {
    PRINTF(". Not sending bounced message\n");
  } else {
    len = ip64_6to4(&uip_buf[UIP_LLH_LEN], uip_len,
		    ip64_packet_buffer);
    if(len > 0) {
      PRINTF(". Output len %d.\n", len);
#ifdef PPP_6TO4_STATS
      ppp_6to4good++;
#endif /* PPP_6TO4_STATS */
      memcpy(&uip_buf[UIP_LLH_LEN], ip64_packet_buffer, len);
      uip_len = len;
      ppp_send();
      return len;
    }
    else {
      PRINTF(". 6to4 failed.\n");
#ifdef PPP_6TO4_STATS
      ppp_6to4bad++;
#endif /* PPP_6TO4_STATS */
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface ip64_ppp_interface = {
  init, output
};
/*---------------------------------------------------------------------------*/



