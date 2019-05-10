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

#include "dev/at-radio/at-radio.h"
#include "ip64.h"

#include <string.h>
#include <stdio.h>

#define UIP_IP_BUF        ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

PROCESS(ip64_ppp_process, "IP64 PPP process");

static uip_ipaddr_t last_sender;

/*---------------------------------------------------------------------------*/
static void
create_unique_local_address() {
  static uip_ipaddr_t loc_fipaddr;

  /* Hack */
  loc_fipaddr.u16[0] = 0xfd;
  loc_fipaddr.u16[1] = rand();
  loc_fipaddr.u16[2] = rand();  
  loc_fipaddr.u16[3] = rand();
  loc_fipaddr.u16[4] = rand();
  loc_fipaddr.u16[5] = rand();
#if UIP_CONF_ROUTER
  uip_ds6_prefix_add(&loc_fipaddr, 6*8, 0, 0, 0, 0);
#else /* UIP_CONF_ROUTER */
  uip_ds6_prefix_add(&loc_fipaddr, 6*8, 0);
#endif /* UIP_CONF_ROUTER */

  uip_ds6_set_addr_iid(&loc_fipaddr, &uip_lladdr);
  uip_ds6_addr_add(&loc_fipaddr, 0, ADDR_AUTOCONF);
}
/*---------------------------------------------------------------------------*/
/*
 * A node with only an uplink may not get an IPv6 address through autoconf.
 * Assign one statically instead as a translatable address (64:ff9b::/96).
 */

static void
create_translatable_address( uip_ip4addr_t *ip4addr) {
  uip_ipaddr_t loc_fipaddr;

  memset(&loc_fipaddr, 0, sizeof(loc_fipaddr));
  loc_fipaddr.u8[0] = 0x00;
  loc_fipaddr.u8[1] = 0x64;  
  loc_fipaddr.u8[2] = 0xff;
  loc_fipaddr.u8[3] = 0x9b;  
  loc_fipaddr.u8[12] = ip4addr->u8[0];
  loc_fipaddr.u8[13] = ip4addr->u8[1];
  loc_fipaddr.u8[14] = ip4addr->u8[2];
  loc_fipaddr.u8[15] = ip4addr->u8[3];
      
  uip_ds6_addr_add(&loc_fipaddr, 0, ADDR_AUTOCONF);
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
  struct at_radio_status *atstat;
  static struct etimer et;

  PROCESS_BEGIN();
  printf("ip64_ppp: here is PPP process\n");
  PROCESS_WAIT_EVENT_UNTIL(ev == at_radio_ev_init);
  printf("ip64_ppp: RADIO IS READY\n");
  atstat = at_radio_status();
  {
    int i;
    for (i = 0; i < sizeof(atstat->ip4addr); i++) {
      printf("%d.",       ((uint8_t *) &status.ip4addr)[i]);
    }
    for (i = 0; i < sizeof(atstat->ip4mask); i++) {
      printf("%d.",       ((uint8_t *) &status.ip4mask)[i]);
    }
    printf("\n");
  }
  ip64_set_ipv4_address(&atstat->ip4addr, &atstat->ip4mask);
  //create_unique_local_address();
  create_translatable_address(&atstat->ip4addr);
  ppp_connect();

  printf("ip64_ppp: Did ppp_connect()\n");
  while (1) {
    etimer_set(&et, 3*CLOCK_SECOND);
    while (!etimer_expired(&et)) {
      PROCESS_PAUSE();
    } 
    ppp_poll();
    if (uip_len > 0) {
      printf("##########ip64_ppp: uip_len %d\n", uip_len);
      /* Save the last sender received over PPP to avoid bouncing the
         packet back if no route is found */
      uip_ipaddr_copy(&last_sender, &UIP_IP_BUF->srcipaddr);

      uint16_t len = ip64_4to6(&uip_buf[UIP_LLH_LEN], uip_len, 
                               ip64_packet_buffer);
      if(len > 0) {
        memcpy(&uip_buf[UIP_LLH_LEN], ip64_packet_buffer, len);
        uip_len = len;
        printf("IP6 len %d\n", len);
        tcpip_input();
      } else {
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
  printf("here is ip64-ppp-output\n");
  printf("ip64-ppp-interface: output source ");

  PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
  printf(" destination ");
  PRINT6ADDR(&UIP_IP_BUF->destipaddr);
  printf("\n");

  if(uip_ipaddr_cmp(&last_sender, &UIP_IP_BUF->srcipaddr)) {
    PRINTF("ip64-interface: output, not sending bounced message\n");
  } else {
    len = ip64_6to4(&uip_buf[UIP_LLH_LEN], uip_len,
		    ip64_packet_buffer);
    PRINTF("ip64-ppp-interface: output len %d\n", len);
    if(len > 0) {
      memcpy(&uip_buf[UIP_LLH_LEN], ip64_packet_buffer, len);
      uip_len = len;
      ppp_send();
      return len;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct uip_fallback_interface ip64_ppp_interface = {
  init, output
};
/*---------------------------------------------------------------------------*/



