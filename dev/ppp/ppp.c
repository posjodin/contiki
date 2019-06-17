/*															
 *---------------------------------------------------------------------------
 * ppp.c - PPP Processor/Handler											-
 *																			-
 *---------------------------------------------------------------------------
 * 
 * Version                                                                  -
 *		0.1 Original Version Jun 3, 2000									-        
 *																			-
 *---------------------------------------------------------------------------    
 */
/*
 * Copyright (c) 2003, Mike Johnson, Mycal Labs, www.mycal.net
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Mike Johnson/Mycal Labs
 *		www.mycal.net.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the Mycal Modified uIP TCP/IP stack.
 *
 * $Id: ppp.c,v 1.5 2005/02/23 22:38:43 oliverschmidt Exp $
 *
 */

/*			*/ 
/* include files 	*/
/*			*/ 


#include <stdio.h>
#include <dev/watchdog.h>

#include "lcp.h"
#include "pap.h"
#include "ipcp.h"
/*#include "time.h"*/
/*#include "mip.h"*/

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*
  Set the debug message level
*/
#define	PACKET_RX_DEBUG	1

/*
  Include stuff
*/
/*#include "mTypes.h"*/
#include "ppp.h"
#include "ahdlc.h"
#include "ipcp.h"
#include "lcp.h"


/*
  Buffers that this layer needs (this can be optimized out)
*/
uint8_t ppp_rx_buffer[PPP_RX_BUFFER_SIZE];
/*uint8_t ppp_tx_buffer[PPP_TX_BUFFER_SIZE];*/

/*
 * IP addr set by PPP server
 */
uip_ip4addr_t our_ipaddr;

/*
 * Other state storage (this can be placed in a struct and this could could
 *	support multiple PPP connections, would have to embedded the other ppp
 *	module state also)
 */
uint8_t ppp_flags;
uint8_t ppp_id;
uint8_t ppp_retry;

#if PACKET_RX_DEBUG
uint16_t ppp_rx_frame_count=0;
uint16_t ppp_rx_tobig_error;
uint8_t done;    /* temporary variable */
#endif

/*---------------------------------------------------------------------------*/
#if 0
static uint8_t
check_ppp_errors(void)
{
  uint8_t ret = 0;

  /* Check Errors */
  if(lcp_state & LCP_TX_TIMEOUT) {
    ret = 1;
  }
  if(lcp_state & LCP_RX_TIMEOUT) {
    ret = 2;
  }

  if(pap_state & PAP_TX_AUTH_FAIL) {
    ret = 3;
  }
  if(pap_state & PAP_RX_AUTH_FAIL) {
    ret = 4;
  }

  if(pap_state & PAP_TX_TIMEOUT) {
    ret = 5;
  }
  if(pap_state & PAP_RX_TIMEOUT) {
    ret = 6;
  }

  if(ipcp_state & IPCP_TX_TIMEOUT) {
    ret = 7;
  }

  return ret;
}
#endif
/*---------------------------------------------------------------------------*/
/*
 * Unknown Protocol Handler, sends reject
 */
static void
ppp_reject_protocol(uint16_t protocol, uint8_t *buffer, uint16_t count)
{
  uint16_t	i;
  uint8_t *dptr, *sptr;
  LCPPKT *pkt;
	
  /* first copy rejected packet back, start from end and work forward,
     +++ Pay attention to buffer managment when updated. Assumes fixed
     PPP blocks. */
  PRINTF("Rejecting Protocol\n");
  if((count + 6) > PPP_RX_BUFFER_SIZE) {
    /* This is a fatal error +++ do somthing about it. */
    PRINTF("Cannot Reject Protocol, PKT to big\n");
    return;
  }
  dptr = buffer + count + 6;
  sptr = buffer + count;
  for(i = 0; i < count; ++i) {
    *dptr-- = *sptr--;
  }

  pkt = (LCPPKT *)buffer;
  pkt->code = PROT_REJ;		/* Write Conf_rej */
  /*pkt->id = tid++;*/			/* write tid */
  pkt->len = uip_htons(count + 6);
  *((uint16_t *)(&pkt->data[0])) = uip_htons(protocol);

  ahdlc_tx(LCP, buffer, 0, (uint16_t)(count + 6), 0);
}
/*---------------------------------------------------------------------------*/
char
*ppp_codestr(uint8_t code) {
  static char buf[10];

  switch (code) {
  case CONF_REQ: return("CONF_REQ"); 
  case CONF_ACK: return("CONF_ACK"); 
  case CONF_NAK: return("CONF_NAK"); 
  case CONF_REJ: return("CONF_REJ"); 
  case TERM_REQ: return("TERM_REQ"); 
  case TERM_ACK: return("TERM_ACK"); 
  case PROT_REJ: return("PROT_REJ"); 
  default:
    snprintf(buf, sizeof(buf), "code %d", code);
    return buf;
  }
}
/*---------------------------------------------------------------------------*/
#if PACKET_RX_DEBUG
void
dump_ppp_packet(uint8_t *buffer, uint16_t len)
{
  int i;

  PRINTF("\n");
  for(i = 0;i < len; ++i) {
    if((i & 0x1f) == 0x10) {
      PRINTF("\n");
    }
    PRINTF("%02x ",buffer[i]);
  }
  PRINTF("\n");
}
#endif
/*---------------------------------------------------------------------------*/
/* Initialize and start PPP engine.  This just sets things up to
 * starting values.  This can stay a private method.
 */
/*---------------------------------------------------------------------------*/
void
ppp_init()
{
#if PACKET_RX_DEBUG
  ppp_rx_frame_count = 0;
  done = 0;
#endif
  ppp_flags = 0;
  pap_init();
  ipcp_init();
  lcp_init();
  ppp_flags = 0;
	
  ahdlc_init(ppp_rx_buffer, PPP_RX_BUFFER_SIZE);
  ahdlc_rx_ready();
}
/*---------------------------------------------------------------------------*/
/* raise_ppp() - This routine will try to bring up a PPP connection, 
 *  It is blocking. In the future we probably want to pass a
 *  structure with all the options on bringing up a PPP link, like
 *  server/client, DSN server, username password for PAP... +++ for
 *  now just use config and bit defines
 */
/*---------------------------------------------------------------------------*/
#if 0
uint16_t
ppp_raise(uint8_t config, uint8_t *username, uint8_t *password)
{
  uint16_t	status = 0;

  /* Initialize PPP engine */
  /* init_ppp(); */
  pap_init();
  ipcp_init();
  lcp_init();

  /* Enable PPP */
  ppp_flags = PPP_RX_READY;

  /* Try to bring up the layers */
  while(status == 0) {
#ifdef SYSTEM_POLLER
    /* If the the serial interrupt is not hooked to ahdlc_rx, or the
       system needs to handle other stuff while were blocking, call
       the system poller.*/
      system_poller();
#endif		

      /* call the lcp task to bring up the LCP layer */
      lcp_task(ppp_tx_buffer);

      /* If LCP is up, neg next layer */
      if(lcp_state & LCP_TX_UP) {
	/* If LCP wants PAP, try to authenticate, else bring up IPCP */
	if((lcp_state & LCP_RX_AUTH) && (!(pap_state & PAP_TX_UP))) {
	  pap_task(ppp_tx_buffer,username,password);  
	} else {
	  ipcp_task(ppp_tx_buffer);
	}
      }


      /* If IPCP came up then our link should be up. */
      if((ipcp_state & IPCP_TX_UP) && (ipcp_state & IPCP_RX_UP)) {
	break;
      }
      
      status = check_ppp_errors();
    }
  
  return status;
}
#endif
/*---------------------------------------------------------------------------*/
void
ppp_connect(void)
{
  /* Initialize PPP engine */
  /* init_ppp(); */
  /*pap_init();
  ipcp_init();
  lcp_init();*/
  
  /* Enable PPP */
  ppp_flags = PPP_RX_READY;
}
/*---------------------------------------------------------------------------*/
void
ppp_send(void)
{
  /* If IPCP came up then our link should be up. */
  if((ipcp_state & IPCP_TX_UP) && (ipcp_state & IPCP_RX_UP)) {
    ahdlc_tx(IPV4, NULL, uip_buf, 0, uip_len); 
  }
}
/*---------------------------------------------------------------------------*/
void
#define PPP_MAX_CONSEC_RX 92
ppp_poll(void)
{
  uint8_t c;
  uip_len = 0;

  if(!(ppp_flags & PPP_RX_READY)) {
    return;
  }

  while(uip_len == 0 && ppp_arch_getchar(&c)) {
    ahdlc_rx(c);
    watchdog_periodic();
  }

  /* If IPCP came up then our link should be up. */
  if((ipcp_state & IPCP_TX_UP) && (ipcp_state & IPCP_RX_UP)) {
    return;
  }

  /* call the lcp task to bring up the LCP layer */
  lcp_task(uip_buf);

  /* If LCP is up, neg next layer */
  if(lcp_state & LCP_TX_UP) {
    /* If LCP wants PAP, try to authenticate, else bring up IPCP */
    if((lcp_state & LCP_RX_AUTH) && (!(pap_state & PAP_TX_UP))) {
      pap_task(uip_buf);  
    } else {
      ipcp_task(uip_buf);
    }
  }
}
/*---------------------------------------------------------------------------*/
/* ppp_upcall() - this is where valid PPP frames from the ahdlc layer are
 *	sent to be processed and demuxed.
 */
/*---------------------------------------------------------------------------*/
void
ppp_upcall(uint16_t protocol, uint8_t *buffer, uint16_t len)
{
#if PACKET_RX_DEBUG

  ++ppp_rx_frame_count;
  PRINTF("PPP upcall: "); dump_ppp_packet(buffer, len);
  if(ppp_rx_frame_count > 18) {
    done = 1;
  }
#endif

  /* check to see if we have a packet waiting to be processed */
  if(ppp_flags & PPP_RX_READY) {	
    /* demux on protocol field */
    switch(protocol) {
    case LCP:	/* We must support some level of LCP */
      PRINTF("LCP Packet - ");
      lcp_rx(buffer, len);
      PRINTF("\n");
      break;
    case PAP:	/* PAP should be compile in optional */
      PRINTF("PAP Packet - ");
      pap_rx(buffer, len);
      PRINTF("\n");
      break;
    case IPCP:	/* IPCP should be compile in optional. */
      PRINTF("IPCP Packet - ");
      ipcp_rx(buffer, len);
      PRINTF("\n");
      break;
    case IPV4:	/* We must support IPV4 */
      PRINTF("IPV4 Packet---\n");
      memcpy(uip_buf, buffer, len);
      uip_len = len;
      PRINTF("\n");
      break;
    default:
      PRINTF("Unknown PPP Packet Type 0x%04x - ",protocol);
      ppp_reject_protocol(protocol, buffer, len);
      PRINTF("\n");
      break;
    }
  }
}
/*---------------------------------------------------------------------------*/
/* scan_packet(list,buffer,len)
 *
 * list = list of supported ID's
 * *buffer pointer to the first code in the packet
 * length of the codespace
 */
uint16_t
scan_packet(uint16_t protocol, uint8_t *list, uint8_t *buffer, uint8_t *options, uint16_t len)
{
  uint8_t *tlist, *bptr;
  uint8_t *tptr;
  uint8_t bad = 0;
  uint8_t i, j, good;

  bptr = tptr = options;
  /* scan through the packet and see if it has any unsupported codes */
  while(bptr < options + len) {
    /* get code and see if it matches somwhere in the list, if not
       we don't support it */
    i = *bptr++;
    
    /*    DEBUG2("%x - ",i);*/
    tlist = list;
      good = 0;
      while(*tlist) {
	/*	DEBUG2("%x ",*tlist);*/
	if(i == *tlist++) {
	  good = 1;
	  break;
	}
      }
      if(!good) {
	/* we don't understand it, write it back */
	PRINTF("We don't understand option 0x%02x\n",i);
	bad = 1;
	*tptr++ = i;
	j = *tptr++ = *bptr++;
	for(i = 0; i < j - 2; ++i) {
	  *tptr++ = *bptr++;
	}
      } else {
	/* advance over to next option */
	bptr += *bptr - 1;
      }
  }
  
  /* Bad? if we we need to send a config Reject */
  if(bad) {
    /* write the config Rej packet we've built above, take on the header */
    bptr = buffer;
    *bptr++ = CONF_REJ;		/* Write Conf_rej */
    bptr++;			/* skip over ID */
    *bptr++ = 0;
    *bptr = tptr - buffer;
    /* length right here? */
		
    /* write the reject frame */
    PRINTF("Writing Reject frame --\n");

    ahdlc_tx(protocol, buffer, 0, (uint16_t)(tptr - buffer), 0);
    PRINTF("\nEnd writing reject \n");
    
  }		
  return bad;
}
/*---------------------------------------------------------------------------*/
