#ifndef __AHDLC_H__
#define __AHDLC_H__

/*---------------------------------------------------------------------------
 ahdlc.h - ahdlc header file  					  
---------------------------------------------------------------------------
 Version    
		0.1 Original Version Jan 11, 1998				
 (c)1998 Mycal Labs, All Rights Reserved
 ---------------------------------------------------------------------------*/
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
 * $Id: ahdlc.h,v 1.2 2004/08/22 12:37:00 oliverschmidt Exp $
 *
 */

#include "uip.h"
/*---------------------------------------------------------------------------
 * ahdlc flags bit defins, for ahdlc_flags variable
 ---------------------------------------------------------------------------*/
/* Escaped mode bit */
#define AHDLC_ESCAPED		0x1
/* Frame is ready bit */
#define	AHDLC_RX_READY		0x2				
#define	AHDLC_RX_ASYNC_MAP	0x4
#define AHDLC_TX_ASYNC_MAP	0x8
#define AHDLC_PFC		0x10
#define AHDLC_ACFC		0x20

uint8_t ahdlc_flags;

void ahdlc_init(uint8_t *, uint16_t);
void ahdlc_rx_ready(void);
uint8_t ahdlc_rx(uint8_t);  
uint8_t ahdlc_tx(uint16_t protocol, uint8_t *header, uint8_t *buffer,
	      uint16_t headerlen, uint16_t datalen);

#endif /* __AHDLC_H__ */
