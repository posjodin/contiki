/*
 * Copyright (c) 2012-2014, Thingsquare, http://www.thingsquare.com/.
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
 */

#ifndef TCP_SOCKET_AT_RADIO_COMPAT_H
#define TCP_SOCKET_AT_RADIO_COMPAT_H

#include "../../../../core/net/ip/tcp-socket.h"

#ifdef AT_RADIO_SOCKETS

/* Use Contiki socket API to access builtin protocol
 * stack on cellular data radio modules. 
 * Lacking a generic interface between TCP sockets and protocol
 * stack, rename socket operations to use functions from 
 * tcp-socket-at-radio.h.
 */

#include "tcp-socket-at-radio.h"

#define tcp_socket tcp_socket_at_radio 

#define tcp_socket_register tcp_socket_at_radio_register
#define tcp_socket_connect tcp_socket_at_radio_connect
#define tcp_socket_listen tcp_socket_at_radio_listen
#define tcp_socket_unlisten  tcp_socket_at_radio_unlisten
#define tcp_socket_send tcp_socket_at_radio_send
#define tcp_socket_send_str tcp_socket_at_radio_send_str 
#define tcp_socket_close tcp_socket_at_radio_close 
#define tcp_socket_unregister tcp_socket_at_radio_unregister 
#define tcp_socket_max_sendlen tcp_socket_at_radio_max_sendlen 
#define tcp_socket_queuelen tcp_socket_at_radio_queuelen
#endif /* AT_RADIO_SOCKETS */
#endif /* TCP_SOCKET_AT_RADIO_COMPAT_H */
