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
 *         Contiki-OS M-Bus functionality
 * \authors
 *          Albert Asratyan asratyan@kth.se
 *          Mandar Joshi mandarj@kth.se
 *
 */

#include "sys/etimer.h"
#include <unistd.h>
#include <string.h>
#include "dev/watchdog.h"
#include <stdio.h>

//#include "dev/mbus-arch.h"
#include "dev/mbus/mbus-arch.h"
#include "mbus-supported-devices.h"


#define MBUS_FRAME_TYPE_ACK     1
#define MBUS_FRAME_TYPE_SHORT   2
#define MBUS_FRAME_TYPE_CONTROL 3
#define MBUS_FRAME_TYPE_LONG    4

#define MBUS_START_BIT_SHORT 0x10
#define MBUS_START_BIT_LONG 0x68
#define MBUS_START_BIT_CONTROL 0x68
#define MBUS_END_BIT 0x16

#define MBUS_FRAME_SIZE_SHORT 5
#define MBUS_FRAME_SIZE_CONTROL 9

#define MBUS_CONTROL_FIELD_DIRECTION    0x07
#define MBUS_CONTROL_FIELD_FCB          0x06
#define MBUS_CONTROL_FIELD_ACD          0x06
#define MBUS_CONTROL_FIELD_FCV          0x05
#define MBUS_CONTROL_FIELD_DFC          0x05
#define MBUS_CONTROL_FIELD_F3           0x04
#define MBUS_CONTROL_FIELD_F2           0x03
#define MBUS_CONTROL_FIELD_F1           0x02
#define MBUS_CONTROL_FIELD_F0           0x01

#define MBUS_CONTROL_MASK_SND_NKE       0x40
#define MBUS_CONTROL_MASK_SND_UD        0x53
#define MBUS_CONTROL_MASK_REQ_UD2       0x5B
#define MBUS_CONTROL_MASK_REQ_UD1       0x5A
#define MBUS_CONTROL_MASK_RSP_UD        0x08

#define MBUS_CONTROL_MASK_FCB           0x20
#define MBUS_CONTROL_MASK_FCV           0x10

#define MBUS_CONTROL_MASK_ACD           0x20
#define MBUS_CONTROL_MASK_DFC           0x10

#define MBUS_CONTROL_MASK_DIR           0x40
#define MBUS_CONTROL_MASK_DIR_M2S       0x40
#define MBUS_CONTROL_MASK_DIR_S2M       0x00

#define MBUS_ANSWER_ACK                 0xE5

#define  MBUS_BAUD_RATE                 300




/*---------------------------------------------------------------------------*/
/**
 * \brief      Timeout period for M-Bus slave transmission.
 * \retval 1   on success
 * \retval -1  if the given baudrate is unsupported
 *
 *             M-Bus documentation specifies a timeout period of
 *             11 bit times + (330 bit times + 50 ms)
 *
 */
int
wait_for_mbus()
{
  if (MBUS_BAUD_RATE == 300) {
    clock_delay_msec(1000);

  } else
  if (MBUS_BAUD_RATE == 2400) {
    clock_delay_msec(700);
  } else
  if (MBUS_BAUD_RATE == 9600) {
    clock_delay_msec(300);
  } else {
    printf("Unsupported baudrate!\n");
    return -1;
  }
  return 1;
}
int
wait_for_mbus_data()
{
  if (MBUS_BAUD_RATE == 300) {
    for (int i = 0; i < 80; i ++) {
      clock_delay_msec(100);
      watchdog_periodic();
    }
   } else
  if (MBUS_BAUD_RATE == 2400) {
    for (int i = 0; i < 10; i ++) {
      clock_delay_msec(100);
      watchdog_periodic();
    }
  } else
  if (MBUS_BAUD_RATE == 9600) {
    clock_delay_msec(300);
  } else {
    printf("Unsupported baudrate!\n");
    return -1;
  }
  return 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief       Verify the long frame
 * \param *data
 * \retval 1   on success
 *
 *             Verifies long frame starting and ending bits.
 */

int
mbus_verify_frame_long(uint16_t *data, int frame_length)
{
  if (data[0] == 0x68 && data[frame_length-1] == 0x16) {
    return 1;
  }
  return -1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief           Ping the given address for an ACK
 *                  send it the correct frame. Local function.
 * \param address   Address of the M-Bus slave
 * \retval -2   For invalid primary address input
 * \retval 1    On success
 *
 *                 Pings a M-Bus slave device by transmitting a coressponding
 *                 frame.
 */

int
mbus_ping_frame(int address)
{
  int checksum;
  int ping = MBUS_CONTROL_MASK_SND_NKE;

  if (address > 255) {
    printf("Invalid address!\n");
    return -2;
  }

  checksum = (MBUS_CONTROL_MASK_SND_NKE + address) % 0x100;
  uint8_t frame[MBUS_FRAME_SIZE_SHORT] = {MBUS_START_BIT_SHORT, ping,
                                          address, checksum, MBUS_END_BIT};

  for (int i = 0; i < MBUS_FRAME_SIZE_SHORT; i++) {
      mbus_arch_send_byte(&frame[i]);
  }
  return 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Function to read an input of 1 ACK
 * \return     The response, should be 0xE5 on success, everything else on failure
 */

int
mbus_receive_ack()
{
  int response = 0;
  uint16_t buff[1];

  mbus_arch_read_byte(buff);
  response = buff[0];

  return response;
}

/**
 * \brief      Scan for a M-Bus slave
 * \param address    M-Bus slave address to scan
 * \retval -1     On failure
 * \retval 1      On successful ping
 * \retval 0xE5   On M-Bus device found
 *
 *              Pings a M-Bus slave at a specific primary address. Waits some
 *              time for a response and reads it. If a device was found, returns
 *              its ACK hex value.
 */

int
mbus_scan_primary_at_address(int address)
{
  int result, response;

  result = mbus_ping_frame(address);
  if (result == -1) {
    printf("Scan failed at the following address: %d\n.", address);
    return -1;
  }

  if (wait_for_mbus() == -1) {
    return -1;
  }

  response = mbus_receive_ack();

  if (response == MBUS_ANSWER_ACK) {
    printf("M-Bus device at %d has sent an ACK.\n\n", address);
    return response;
  } else {
    printf("No M-Bus device found at the address %d.\n", address);
  }

  return 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Function to scan all M-Bus primary addresses
 *
 * \retval 1   on success
 * \retval -1   on failure
 *
 *              Pings all possible M-Bus slaves (250 devices) one by one.
 */

int
mbus_scan_primary_all()
{
  int address = 0;
  int result, response;

  for (address = 0; address <= 250; address++) {

    watchdog_periodic();

    result = mbus_ping_frame(address);
    if (result == -1) {
      printf("Scan failed at the following address: %d\n", address);
    }

    if (wait_for_mbus() == -1) {
      return -1;
    }

    response = mbus_receive_ack();

    clock_delay_msec(3);

    if (response == MBUS_ANSWER_ACK) {
      printf("M-Bus device at %d has sent an ACK.\n\n", address);
    } else {
      printf("No M-Bus device found at the address %d\n", address);
    }
  }
  return 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      function that sends request data at a specific primary address
               Internal.
 * \param address Known primary address of M-Bus slave
 * \retval -2   Invalid address
 * \retval 1   On success
 */

int
mbus_send_data_request(int address)
{
  int request, checksum;

  request = MBUS_CONTROL_MASK_REQ_UD2;

  if (address > 250) {
    printf("Invalid address!\n");
    return -2;
  }

  checksum = request + address;

  uint8_t frame[MBUS_FRAME_SIZE_SHORT] = {MBUS_START_BIT_SHORT, request,
                                          address, checksum, MBUS_END_BIT};

  for (int i = 0; i < MBUS_FRAME_SIZE_SHORT; i++) {
    mbus_arch_send_byte(&frame[i]);
    watchdog_periodic();
  }
  return 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Read a long frame (data) from M-Bus slave
 * \param data array to store the values to
 * \param frame_length specifies the length of the long frame
 * \retval 1   on completion
 */

int
mbus_receive_long(uint16_t *data, int frame_length)
{
  uint16_t buff[frame_length];
  memset((void *)buff, 0, sizeof(buff));

  for (int i = 0; i < frame_length; i++) {
     mbus_arch_read_byte(buff);
     data[i] = buff[0];
  }

  return 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Function to request data from M-Bus slave at given primary
 *             Address
 * \param address    Primary address of M-Bus slave
 * \param data    Array to store data received to
 * \param frame_length    Length of expected received frame
 * \retval -1   Failure
 * \retval 1   Success
 *
 *             Data array passed as an argument should
 *             be of the same length as the value of the frame_length variable.
 *
 */
int
mbus_request_data_at_primary_address(int address, uint16_t *data, int frame_length)
{
  int result;
  uint16_t received_data[frame_length];

  result = mbus_send_data_request(address);
  if (result == -1) {
    printf("Failed to send a request to the address: %d\n.", address);
    return -1;
  }

  if (wait_for_mbus_data() == -1) {
    return -1;
  }

  memset((void *)received_data, 0, sizeof(received_data));

  mbus_receive_long(received_data, frame_length);

  for (int i = 0; i < frame_length; i++) {
    data[i] = received_data[i];
  }

  if (mbus_verify_frame_long(data, frame_length) == -1) {
    printf("Corrupted frame!\n");
    return -1;
  }

  return 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Function to send the frame for address change. Internal.
 * \param address Primary address
 * \param new_address Address to change to
 * \retval -1   Wrong inputs
 * \retval 1   Frame sent.
 */

int
mbus_send_address_change(int address, int new_address)
{
  int control, checksum;

  control = MBUS_CONTROL_MASK_SND_UD;
  if (address > 250) {
    printf("Invalid old address!\n");
    return -1;
  }
  if (new_address > 250) {
    printf("Invalid new address!\n");
    return -1;
  }
  checksum = (control + address + 0x51 + 0x01 + 0x7A + new_address) % 0x100;

  uint8_t frame[12] = {MBUS_START_BIT_CONTROL, 0x06, 0x06,
                      0x68, control, address, 0x51, 0x01, 0x7A,
                      new_address, checksum, MBUS_END_BIT};

  for (int i = 0; i < 12; i++) {
      mbus_arch_send_byte(&frame[i]);
    }
  return 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Function to change primary address of a M-Bus slave
 * \param address Primary address
 * \param new_address Address to change to
 * \retval -1   Wrong input, failure to send
 * \retval 1   Function completion
 *
 *              Changes primary address of a device, given the current address.
 */

int
mbus_set_primary_address(int address, int new_address)
{
  int result, response;

  if (address > 250) {
    printf("Invalid address!\n");
    return -1;
  }

  result = mbus_send_address_change(address, new_address);
  if (result == -1) {
    printf("Failed to send a control frame at the address: %d\n.", address);
    return -1;
  }

  if (wait_for_mbus() == -1) {
    return -1;
  }

  response = mbus_receive_ack();

  if (response == MBUS_ANSWER_ACK) {
    printf("Control frame sent successfully.\n");
    printf("M-Bus device at %d has sent an ACK.\n", address);
  } else {
    printf("No M-Bus device found at the address %d.\n", address);
  }

  return 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Function to send frame to switch baudrate. Internal.
 * \param address    Primary address for M-Bus slave
 * \param baudrate    New baudrate for communication
 * \retval -1   Wrong address
 * \retval 1   Frame sent.
 */

int
mbus_send_frame_switch_baudrate(int address, int baudrate)
{
  int control, checksum;

  control = MBUS_CONTROL_MASK_SND_UD;
  if (address > 250) {
    printf("Invalid old address!\n");
    return -1;
  }
  switch(baudrate) {
    case 300:
      baudrate = 0xB8;
    case 2400:
      baudrate = 0xBB;
    case 9600:
      baudrate = 0xBD;
  }
  checksum = (control + address + baudrate) % 0x100;

  uint8_t frame[9] = {MBUS_START_BIT_CONTROL, 0x03, 0x03,
                      0x68, control, address, baudrate, checksum, MBUS_END_BIT};

  for (int i = 0; i < 9; i++) {
      mbus_arch_send_byte(&frame[i]);
    }
  return 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief      Function to switch baudrate on M-Bus slave
 * \param address    Primary address for M-Bus slave
 * \param baudrate    New baudrate for communication
 * \retval -1   Wrong address, failure to send
 * \retval 1   Function completion.
 *
 *              Sends a control frame that request a change of the baudrate of
 *              a M-Bus slave. The slave should reply with an ACK on the old
 *              baudrate and start listening/sending on the new one right after.
 */

int
mbus_switch_baudrate(int address, int baudrate)
{

  int result, response;

  result = mbus_send_frame_switch_baudrate(address, baudrate);
  if (result == -1) {
    printf("Failed to send a control frame at the address: %d\n.", address);
    return -1;
  }

  if (wait_for_mbus() == -1)
    return -1;

  response = mbus_receive_ack();

  if (response == MBUS_ANSWER_ACK) {
    printf("Baudrate change control frame sent successfully.\n");
    printf("M-Bus device at %d has sent an ACK.\n", address);
  } else {
    printf("No response from the M-Bus device at the address %d.\n", address);
  }

  return 1;
}
