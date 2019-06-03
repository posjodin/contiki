/*  www.mycal.com
 *---------------------------------------------------------------------------
 *lcp.c - Link Configuration Protocol Handler.  - -
 *---------------------------------------------------------------------------
 *Version - 0.1 Original Version June 3, 2000 -
 *
 *---------------------------------------------------------------------------
 *- Copyright (C) 2000, Mycal Labs www.mycal.com - -
 *---------------------------------------------------------------------------
 *
 *
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
 * $Id: lcp.c,v 1.3 2005/01/26 23:36:22 oliverschmidt Exp $
 *
 */

/*			*/ 
/* include files 	*/
/*			*/ 

#include "ppp-conf.h"
#include "uip.h"
/*#include "time.h"*/
#include "ppp.h"
#include "ahdlc.h"
#include "lcp.h"

#define DEBUG 1

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define DEBUG2(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#define DEBUG2(...)
#endif

#ifndef LCP_INTERVAL
#define LCP_INTERVAL 2*CLOCK_SECOND
#endif 
static struct timer timer;
#define TIMER_expire() timer.start = 0
#define TIMER_set() timer_set(&timer, LCP_INTERVAL)
#define TIMER_timeout(x) (timer.start == 0 || timer_expired(&timer))

/*uint8_t				tflag;
uint8_t				*lcp_buffer;
uint16_t				lcp_tx_time;
uint8_t				lcp_retry;
uint16_t				lcp_timeout=5;*/

uint8_t lcp_state;
uint16_t ppp_tx_mru = 0;

/* We need this when we neg our direction.
   uint8_t				lcp_tx_options; */

/*
 * Define the supported paramets for this module here.
 */
uint8_t lcplist[] = {
  LPC_MAGICNUMBER,
  LPC_PFC,
  LPC_ACFC,
  //LPC_AUTH,
  LPC_ACCM,
  LPC_MRU,
  0};	

char *ppp_codestr(uint8_t);
void
lcp_print(uint8_t *buffer, uint16_t count)
{
  uint8_t *bptr = buffer;
  uint16_t len;
  uip_ip4addr_t	ipaddr;
  printf("%s ", ppp_codestr(*bptr++));
  printf("id %d ", *bptr++);
  len = *bptr++ << 8;
  len |= *bptr++;
  printf("len %d ", len);
  
  while(bptr < buffer + len) {
    switch(*bptr) {

    case LPC_MRU:	/* mru */
      bptr++;
      if(*bptr++ == 4) {
        uint16_t ppp_tx_mru = ((*bptr++<<8) | (*bptr++));
        PRINTF("<mru %d> ",ppp_tx_mru);
      } else {
        PRINTF("<mru }> ");
      }
      break;
    case LPC_ACCM:	/*		*/
      bptr++;
      bptr++;		/* skip length */	
      PRINTF("<asyncmap 0x");
      PRINTF("%02x", *bptr++);
      PRINTF("%02x", *bptr++);
      PRINTF("%02x", *bptr++);
      PRINTF("%02x> ", *bptr++);
      break;
    case LPC_AUTH:
      bptr++;
      bptr++;      
      uint16_t auth;
      auth = *bptr++<<8;
      auth |= *bptr++;
      if((auth == 0xc023)) {
        PRINTF("<auth pap> ");

      } else {
        PRINTF("<auth 0x%04x>", auth);
      }
      break;
    case LPC_MAGICNUMBER:
      bptr++;
      bptr++;
      PRINTF("<magic ");
      PRINTF("%02x", *bptr++);
      PRINTF("%02x", *bptr++);
      PRINTF("%02x", *bptr++);
      PRINTF("%02x> ", *bptr++);
      break;
    case LPC_PFC:
      bptr++;
      bptr++;
      PRINTF("<pcomp> ");
      break;
    case LPC_ACFC:
      bptr++;
      bptr++;
      PRINTF("<accomp> ");
      break;
    default:
      {
        uint8_t len;
        printf("<type %d -", *(bptr++));
        len = *bptr++;
        while (len-- > 2) {
          printf(" %02x", *bptr++);
        }
        printf(">");
      }
    }
  }
  printf("\n");
}


/* lcp_init() - Initialize the LCP engine to startup values */
/*---------------------------------------------------------------------------*/
void
lcp_init(void)
{
  lcp_state = 0;
  ppp_retry = 0;
  TIMER_expire();
}
/*---------------------------------------------------------------------------*/
/* lcp_rx() - Receive an LCP packet and process it.  
 *	This routine receives a LCP packet in buffer of length count.
 *	Process it here, support for CONF_REQ, CONF_ACK, CONF_NACK, CONF_REJ or
 *	TERM_REQ.
 */
/*---------------------------------------------------------------------------*/
void
lcp_rx(uint8_t *buffer, uint16_t count)
{
  uint8_t *bptr = buffer, *tptr;
  uint8_t error = 0;
  uint8_t id;
  uint16_t len, j;

  lcp_print(buffer, count);
  switch(*bptr++) {
  case CONF_REQ:			/* config request */
    /* parce request and see if we can ACK it */
    id = *bptr++;
    len = (*bptr++ << 8);
    len |= *bptr++;
    /*len -= 2;*/
    if(scan_packet((uint16_t)LCP, lcplist, buffer, bptr, (uint16_t)(len-4))) {
      /* must do the -4 here, !scan packet */
      
      PRINTF(" options were rejected\n");
    } else {
      /* lets try to implement what peer wants */
      tptr = bptr = buffer;
      bptr += 4;			/* skip code, id, len */
      error = 0;
      /* first scan for unknown values */
      while(bptr < buffer+len) {
	switch(*bptr++) {
	case LPC_MRU:	/* mru */
	  j = *bptr++;
	  j -= 2;
	  if(j == 2) {
	    ppp_tx_mru = ((*bptr++<<8) | (*bptr++));
	    PRINTF("<mru %d> ",ppp_tx_mru);
	  } else {
	    PRINTF("<mru }> ");
	  }
	  break;
	case LPC_ACCM:	/*		*/
	  bptr++;		/* skip length */	
	  j = *bptr++;
	  j += *bptr++;
	  j += *bptr++;
	  j += *bptr++;
	  if((j==0) || (j==0x3fc)) {
	    // ok
	    PRINTF("<asyncmap 0x%04x>",j);
	  } else {
	    /*
	     * fail we only support default or all zeros
	     */
	    PRINTF("We only support default or all zeros for ACCM ");
	    error = 1;
	    *tptr++ = LPC_ACCM;
	    *tptr++ = 0x6;
	    *tptr++ = 0;
	    *tptr++ = 0;
	    *tptr++ = 0;
	    *tptr++ = 0;
	  }
	  break;
	case LPC_AUTH:
	  bptr++;
	  if(0 && (*bptr++==0xc0) && (*bptr++==0x23)) {
	    PRINTF("<auth pap> ");
	    /* negotiate PAP */
	    lcp_state |= LCP_RX_AUTH;	
	  } else {
	    /* we only support PAP */
	    PRINTF("<auth }>");
	    error = 1;
	    *tptr++ = LPC_AUTH;
	    *tptr++ = 0x4;
	    *tptr++ = 0xc0;
	    *tptr++ = 0x23;
	  }
	  break;
	case LPC_MAGICNUMBER:
	  PRINTF("<magic > ");
	  /*
	   * Compair incoming number to our number (not implemented)
	   */
	  bptr++;		/* for now just dump */
	  bptr++;
	  bptr++;
	  bptr++;
	  bptr++;
	  break;
	case LPC_PFC:
	  bptr++;
	  PRINTF("<pcomp> ");
          ahdlc_flags |= AHDLC_PFC;
	  /*tflag|=PPP_PFC;*/
	  break;
	case LPC_ACFC:
	  bptr++;
	  PRINTF("<accomp> ");
          ahdlc_flags |= AHDLC_ACFC;
	  /*tflag|=PPP_ACFC;*/
	  break;
	  
	}
      }
      /* Error? if we we need to send a config Reject ++++ this is good for a subroutine */
      if(error) {
	/* write the config NAK packet we've built above, take on the header */
	bptr = buffer;
	//*bptr++ = CONF_NAK;		/* Write Conf_rej */
        *bptr++ = CONF_REJ;		/* Write Conf_rej */        
	*bptr++;/*tptr++;*/		/* skip over ID */

	/* Write new length */
	*bptr++ = 0;
	*bptr = tptr - buffer;

	/* write the reject frame */
	PRINTF("\nWriting NAK frame \n");

        PRINTF("LCP_TX: "); lcp_print(buffer, (uint16_t)(tptr-buffer));
	// Send packet ahdlc_txz(procol,header,data,headerlen,datalen);				
        ahdlc_tx(LCP, 0, buffer, 0, (uint16_t)(tptr-buffer));
	
      } else {
	/*
	 * If we get here then we are OK, lets send an ACK and tell the rest
	 * of our modules our negotiated config.
	 */
	bptr = buffer;
	*bptr++ = CONF_ACK;		/* Write Conf_ACK */
	bptr++;				/* Skip ID (send same one) */
	/*
	 * Set stuff
	 */
	/*ppp_flags|=tflag;*/
	/* DEBUG2("SET- stuff -- are we up? c=%d dif=%d \n", count, (uint16_t)(bptr-buffer)); */
	
	/* write the ACK frame */
        PRINTF("LCP_TX: "); lcp_print(buffer, count);
	/* Send packet ahdlc_txz(procol,header,data,headerlen,datalen);	*/
	ahdlc_tx(LCP, 0, buffer, 0, count /*bptr-buffer*/);
	
	/* expire the timer to make things happen after a state change */
	/*timer_expire();*/
	
      }
    }
    break;
  case CONF_ACK:			/* config Ack   Anytime we do an ack reset the timer to force send. */
    /* check that ID matches one sent */
    if(*bptr++ == ppp_id) {	
      /* Change state to PPP up. */
      PRINTF(">>>>>>>> good ACK id up!\n",ppp_id);
      /* copy negotiated values over */
      
      lcp_state |= LCP_TX_UP;		
      
      /* expire the timer to make things happen after a state change */
      TIMER_expire();
    }
    else
      PRINTF("*************++++++++++ bad id %d\n",ppp_id);
    break;
  case CONF_NAK:			/* Config Nack */
    ppp_id++;
    break;
  case CONF_REJ:			/* Config Reject */
    ppp_id++;
    break;
  case TERM_REQ:			/* Terminate Request */
    bptr = buffer;
    *bptr++ = TERM_ACK;			/* Write TERM_ACK */
    /* write the reject frame */
    /* Send packet ahdlc_txz(procol,header,data,headerlen,datalen); */
    PRINTF("LCP_TX: "); lcp_print(buffer, count);
    lcp_state &= ~LCP_TX_UP;	
    lcp_state |= LCP_TERM_PEER;
    break;
  case TERM_ACK:
    break;
  default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
/* lcp_task(buffer) - This routine see if a lcp request needs to be sent
 *	out.  It uses the passed buffer to form the packet.  This formed LCP
 *	request is what we negotiate for sending options on the link.
 *
 *	Currently we negotiate : Magic Number Only, but this will change.
 */
/*---------------------------------------------------------------------------*/
void
lcp_task(uint8_t *buffer)
{
  uint8_t *bptr;
  uint16_t t;
  LCPPKT *pkt;

  /* lcp tx not up and hasn't timed out then lets see if we need to send a request */
  if(!(lcp_state & LCP_TX_UP) && !(lcp_state & LCP_TX_TIMEOUT)) {
    /* Check if we have a request pending */
    /*t=get_seconds()-lcp_tx_time;*/
    if(1 == TIMER_timeout(LCP_TX_TIMEOUT)) {
      /*
       * No pending request, lets build one
       */
      pkt = (LCPPKT *)buffer;		

      /* Configure-Request only here, write id */
      pkt->code = CONF_REQ;
      pkt->id = ppp_id;
      
      bptr = pkt->data;
      
      /* Write options */
      
      /* Write magic number */
      *bptr++ = LPC_MAGICNUMBER;
      *bptr++ = 0x6;
      *bptr++ = 0;
      *bptr++ = 0;
      *bptr++ = 0;
      *bptr++ = 1;
      
      /* ACCM */

      //if((lcp_tx_options & LCP_OPT_ACCM) & 0) {
      if (1) {
	*bptr++ = LPC_ACCM;
	*bptr++ = 0x6;
	*bptr++ = 0;
	*bptr++ = 0;
	*bptr++ = 0;
	*bptr++ = 0;
      }
      /*
       * Authentication protocol
       */
      //if((lcp_tx_options & LCP_OPT_AUTH) && 0) {
      if (0) {
	/*
	 * If turned on, we only negotiate PAP
	 */
	*bptr++ = LPC_AUTH;
	*bptr++ = 0x4;
	*bptr++ = 0xc0;
	*bptr++ = 0x23;
      }
      /*
       * PFC
       */
      //if((lcp_tx_options & LCP_OPT_PFC) && 0) {
      if (1) {
	/*
	 * If turned on, we only negotiate PAP
	 */
	*bptr++ = LPC_PFC;
	*bptr++ = 0x2;
      }
      /*
       * ACFC
       */
      //if((lcp_tx_options & LCP_OPT_ACFC) && 0) {
      if (1) {
	/*
	 * If turned on, we only negotiate PAP
	 */
	*bptr++ = LPC_ACFC;
	*bptr++ = 0x2;
      }

      if (0) {
	/*
	 * Negotiate smallest MRU allowed
	 */
	*bptr++ = LPC_MRU;
	*bptr++ = 0x4;
        *bptr++ = 0;
        *bptr++ = 128;
      }

      /* Write length */
      t = bptr - buffer;
      pkt->len = uip_htons(t);			/* length here -  code and ID + */
      
      PRINTF(" len %d\n",t);
      
      /* Send packet */
      /* Send packet ahdlc_txz(procol,header,data,headerlen,datalen); */
      PRINTF("LCP_TX: "); lcp_print(buffer, t);
      ahdlc_tx(LCP, 0, buffer, 0, t);
      
      /* Set timer */
      TIMER_set();
      /*lcp_tx_time=get_seconds();*/
      /* Inc retry */
      ppp_retry++;
      /*
       * Have we timed out?
       */
      if(ppp_retry > LCP_RETRY_COUNT) {
	lcp_state &= LCP_TX_TIMEOUT;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
