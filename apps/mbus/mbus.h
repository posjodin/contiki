/*
* Copyright (c) 2019, Copyright Albert Asratyan, Mandar Joshi
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
*
* This file is part of the Contiki operating system.
*
*/

/**
 * \file
 *         Header file for Contiki-OS M-Bus functionality
 * \authors
 *          Albert Asratyan asratyan@kth.se
 *          Mandar Joshi mandarj@kth.se
 *
 */

#define MBUS_FRAME_SIZE_KAMSTRUP_2101 144



/*
 * Request an ACK from a slave at param address
*/
int mbus_scan_primary_at_address(int address);

/*
 * Scan entire address space for M-Bus devices
*/
int mbus_scan_primary_all();

/*
 * Request data (long frame) from slave at param address,
 * requires an empty array of string to where the data is stored.
 * Has to be char data[144][64]
*/
int mbus_request_data_at_primary_address(int address, uint16_t *data, int frame_length);

/*
 * Set or change the primary address of a M-Bus slave
*/
int mbus_set_primary_address(int address, int new_address);

/*
 * Change the baud rate of M-Bus slave. Baud rate can be 300, 2400 or 9600.
 * The slave returns an ACK (0xE5) for a successful change at the previous baudrate.
 * In the future the baud rate for communication will be changed.
 * Note: Kamstrup 2101 supports auto baud rate detection. It will return an ACK,
 * but the baud rate will not be set.
*/
int mbus_switch_baudrate(int address, int baudrate);
