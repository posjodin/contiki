#ifndef __PPP_H__
#define __PPP_H__
/*																www.mycal.net
---------------------------------------------------------------------------
 ppp.h - ppp header file      									 
---------------------------------------------------------------------------
 Version                                                                 
 0.1 Original Version June 3, 2000					      
 (c)2000 Mycal Labs, All Rights Reserved	     
 --------------------------------------------------------------------------- */
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
 * $Id: ppp.h,v 1.4 2005/01/26 23:36:22 oliverschmidt Exp $
 *
 */
#include "contiki.h"
#include "uip.h"
#include "ppp-conf.h"
#include "ahdlc.h"
#include "lcp.h"
#include "ipcp.h"
#include "pap.h"
#include "ppp_arch.h"
/*#include "mip.h"*/

/* moved to pppconfig.h
#define PPP_RX_BUFFER_SIZE	1024 
#define PPP_TX_BUFFER_SIZE	64*/

#define CRC_GOOD_VALUE		0xf0b8

/* ppp_rx_status values */
#define	PPP_RX_IDLE		0
#define PPP_READY		1

/* ppp flags */
#define PPP_ESCAPED		0x1
#define	PPP_RX_READY		0x2
#define	PPP_RX_ASYNC_MAP	0x8
#define PPP_TX_ASYNC_MAP	0x8
#define PPP_PFC			0x10
#define PPP_ACFC		0x20

/* Supported PPP Protocols */
#define LCP			0xc021
#define PAP			0xc023
#define IPCP			0x8021
#define	IPV4			0x0021

/* LCP codes packet types */
#define CONF_REQ		0x1			
#define CONF_ACK		0x2
#define CONF_NAK		0x3
#define CONF_REJ		0x4
#define TERM_REQ		0x5
#define TERM_ACK		0x6
#define PROT_REJ		0x8

/* Raise PPP config bits */
#define USE_PAP			0x1
#define USE_NOACCMBUG		0x2
#define USE_GETDNS		0x4

#define ppp_setusername(un)	strncpy(pap_username, (un), PAP_USERNAME_SIZE)
#define ppp_setpassword(pw)	strncpy(pap_password, (pw), PAP_PASSWORD_SIZE)

/*
 * Export Variables
 */
/*extern uint8_t ppp_tx_buffer[PPP_TX_BUFFER_SIZE];*/
extern uint8_t ppp_rx_buffer[];
extern uint8_t ppp_rx_count;

#if 0
typedef union {
  uint8_t ip8[4];
  uint16_t ip16[2];
} uip_ip4addr_t;
#endif

extern uip_ip4addr_t our_ipaddr;

/*extern uint16_t ppp_crc_error;*/

extern uint8_t ppp_flags;
extern uint8_t ppp_status;
/*extern uint16_t ppp_rx_crc; */
extern uint16_t ppp_rx_tobig_error;
extern uint8_t ppp_lcp_state;

extern uint8_t ppp_id;
extern uint8_t ppp_retry;

/*
 * Function Prototypes
 */
void ppp_init(void);
void ppp_connect(void);

void ppp_send(void);
void ppp_poll(void);

void ppp_upcall(uint16_t, uint8_t *, uint16_t);
uint16_t scan_packet(uint16_t, uint8_t *list, uint8_t *buffer, uint8_t *options, uint16_t len);

#endif /* __PPP_H__ */
