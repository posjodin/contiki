#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include "sys/etimer.h"
#include "dev/leds.h"
#include <unistd.h>
#include <string.h>
#include "dev/watchdog.h"
#include <stdio.h>

#include "ringbuf-mbus.h"
#include "usart1.h"



/*
TODO list:

add the frame check to every function

*/



#define MBUS_FRAME_TYPE_ACK     1
#define MBUS_FRAME_TYPE_SHORT   2
#define MBUS_FRAME_TYPE_CONTROL 3
#define MBUS_FRAME_TYPE_LONG    4

#define MBUS_START_BIT_SHORT 0x10
#define MBUS_START_BIT_LONG 0x68
#define MBUS_START_BIT_CONTROL 0x68
#define MBUS_END_BIT 0x16

#define MBUS_FRAME_SIZE_SHORT 5
#define MBUS_FRAME_SIZE_LONG 144
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



int
wait_for_mbus()
{
  int baudrate = USART1_CONF_BAUD_RATE;
  if (baudrate == 300)
  {
    clock_wait_msec(4000);
  } else
  if (baudrate == 2400)
  {
    clock_wait_msec(1000);
  } else
  if (baudrate == 9600)
  {
    clock_wait_msec(250);
  } else
  {
    printf("Unsupported baudrate!\n");
    return -1;
  }
}


int
mbus_verify_frame(uint16_t *data, int reply_type)
{
  switch(reply_type)
  {
    case MBUS_FRAME_TYPE_ACK:
      if (data[0] == 0xE5)
      {
        return 1;
      }
    case MBUS_FRAME_TYPE_SHORT:
      if (data[0] == 0x10 && data[4] == 0x16)
      {
        return 1;
      }
    //case MBUS_FRAME_TYPE_CONTROL:
    case MBUS_FRAME_TYPE_LONG:
      if (data[0] == 0x10 && data[143] == 0x16)
      {
        return 1;
      }
    default:
      printf("Frame type not supported!\n");
      return -1;
  }
  return 1;
}



int
mbus_ping_frame(int address)
{
  int ping = MBUS_CONTROL_MASK_SND_NKE;
  if (address > 255)
  {
    printf("Invalid address!\n");
    return -2;
  }
  int checksum = (MBUS_CONTROL_MASK_SND_NKE + address) % 0x100;
  uint8_t frame[MBUS_FRAME_SIZE_SHORT] = {MBUS_START_BIT_SHORT, ping,
                                          address, checksum, MBUS_END_BIT};

  for (int i = 0; i < MBUS_FRAME_SIZE_SHORT; i++) {
      usart1_tx(&frame[i], 1);
      // change usart1_tx later to usart1_tx(&buff[i]);
    }
  return 1;
}


// mb have like a char *
int
mbus_receive_ack()
{
  int response = 0;
  uint8_t buff[1];

  usart1_rx(buff, 1);
  // might act wrong is a situation where for example at address 67 there is an
  // mbus device, but at the address 68 there isnt, but it still sees a E5 there
  response = buff[0];
  ringbuf16_put(&rxbuf, 0);
  // ^ this might solve the issue above by writing a 0 to the buffer right after
  // we got the ACK

  return response;
}



int
mbus_scan_primary_at_address(int address)
{
  int result = mbus_ping_frame(address);
  if (result == -1)
  {
    printf("Scan failed at the following address: %d\n.", address);
    return -1;
  }

  if (wait_for_mbus() == -1)
    return -1;

  int response = mbus_receive_ack();

  if (response == MBUS_ANSWER_ACK)
  {
    printf("M-Bus device at %x has sent an ACK.\n", response);
  } else {
    printf("No M-Bus device found at the address %d.\n", address);
  }

  return 1;
}



int
mbus_scan_primary_all()
{
  int address = 0;
  for (address = 0; address <= 250; address++)
  {
    int result = mbus_ping_frame(address);
    if (result == -1)
    {
      printf("Scan failed at the following address: %d", address);
    }

    if (wait_for_mbus() == -1)
      return -1;

    int response = mbus_receive_ack();

    if (response == MBUS_ANSWER_ACK)
    {
      printf("M-Bus device at %x has sent an ACK.", address);
    } else {
      printf("No M-Bus device found at the address %d", address);
    }

    watchdog_periodic();
  }
  return 1;
}



int
mbus_scan_secondary_all()
{
  //to be added later
}



int
mbus_send_data_request(int address)
{
  int request = MBUS_CONTROL_MASK_REQ_UD2;

  if (address > 250)
  {
    printf("Invalid address!\n");
    return -2;
  }

  int checksum = request + address;

  uint8_t frame[MBUS_FRAME_SIZE_SHORT] = {MBUS_START_BIT_SHORT, request,
                                          address, checksum, MBUS_END_BIT};

  for (int i = 0; i < MBUS_FRAME_SIZE_SHORT; i++) {
    usart1_tx(&frame[i], 1);
    // change usart1_tx later to usart1_tx(&buff[i]);
  }
}



int
mbus_receive_long(uint16_t *data)
{
  uint16_t buff[MBUS_FRAME_SIZE_LONG]; // can be 0?
  memset((void *)buff, 0, sizeof(buff));
  //uint16_t buff2[MBUS_FRAME_SIZE_LONG];
  //memset((void *)buff2, 0, sizeof(buff2));

  for (int i = 0; i < MBUS_FRAME_SIZE_LONG; i++) {
     usart1_rx(buff, 1);
     data[i] = buff[0];     // buff[0] is it really correct? should be &buff
     //buff2[i] = buff[0];
  }

  //data = buff2;

  return 1;
}

/*
 * Data array passed as an argument should have exactly 144 entries.
*/
int
mbus_request_data_at_primary_address(int address, uint16_t *data)
{
  int result = mbus_send_data_request(address);
  if (result == -1)
  {
    printf("Failed to send a request to the address: %d\n.", address);
    return -1;
  }

  if (wait_for_mbus() == -1)
    return -1;

  uint16_t received_data[MBUS_FRAME_SIZE_LONG];
  memset((void *)received_data, 0, sizeof(received_data));

  int response = mbus_receive_long(&received_data);

  for (int i = 0; i < MBUS_FRAME_SIZE_LONG; i++)
  {
    data[i] = received_data[i];
  }

  //data = received_data;

  // or can be this?
  //int response = mbus_receive_long(&data);

  return 1;
}



int
mbus_send_address_change(int address, int new_address)
{
  int control = MBUS_CONTROL_MASK_SND_UD;
  if (address > 250)
  {
    printf("Invalid old address!\n");
    return -1;
  }
  if (new_address > 250)
  {
    printf("Invalid new address!\n");
    return -1;
  }
  int checksum = (control + address + 0x51 + 0x01 + 0x7A + new_address) % 0x100;

  uint8_t frame[12] = {MBUS_START_BIT_CONTROL, 0x06, 0x06,
                      0x68, control, address, 0x51, 0x01, 0x7A,
                      new_address, checksum, MBUS_END_BIT};

  for (int i = 0; i < 12; i++) {
      usart1_tx(&frame[i], 1);
      // change usart1_tx later to usart1_tx(&buff[i]);
    }
  return 1;
}



int
mbus_set_primary_address(int address, int new_address)
{
  if (address > 250)
  {
    printf("Invalid address!\n");
    return -1;
  }

  int result = mbus_send_address_change(address, new_address);
  if (result == -1)
  {
    printf("Failed to send a control frame at the address: %d\n.", address);
    return -1;
  }

  if (wait_for_mbus() == -1)
    return -1;

  int response = mbus_receive_ack();

  if (response == MBUS_ANSWER_ACK)
  {
    printf("Control frame sent successfully.\n");
    printf("M-Bus device at %x has sent an ACK.\n", address);
  } else {
    printf("No M-Bus device found at the address %d.\n", address);
  }

  return 1;
}



int
mbus_send_frame_switch_baudrate(int address, int baudrate)
{
  int control = MBUS_CONTROL_MASK_SND_UD;
  if (address > 250)
  {
    printf("Invalid old address!\n");
    return -1;
  }
  switch(baudrate)
  {
    case 300:
      baudrate = 0xB8;
    case 2400:
      baudrate = 0xBB;
    case 9600:
      baudrate = 0xBD;
  }
  int checksum = (control + address + baudrate) % 0x100;

  uint8_t frame[9] = {MBUS_START_BIT_CONTROL, 0x03, 0x03,
                      0x68, control, address, baudrate, checksum, MBUS_END_BIT};

  for (int i = 0; i < 9; i++) {
      usart1_tx(&frame[i], 1);
      // change usart1_tx later to usart1_tx(&buff[i]);
    }
  return 1;
}



int
mbus_switch_baudrate(int address, int baudrate)
{
  // 68 03 03 68 | 53 FE BD | 0E 16 to switch from 2400 to 9600
  int result = mbus_send_frame_switch_baudrate(address, baudrate);
  if (result == -1)
  {
    printf("Failed to send a control frame at the address: %d\n.", address);
    return -1;
  }

  if (wait_for_mbus() == -1)
    return -1;

  if (response == MBUS_ANSWER_ACK)
  {
    printf("Baudrate change control frame sent successfully.\n");
    printf("M-Bus device at %x has sent an ACK.\n", address);
  } else {
    printf("No response from the M-Bus device at the address %d.\n", address);
  }

  // should probably reinitialise the OS's baudrate here?

  return 1;
}



int
mbus_send_custom_message(uint8_t *message, int reply_type)
{
  // fill later
}



int
mbus_parse_data_kamstrup(uint8_t *data, uint8_t *text_data)
{
  // strings are hard in C :(
  text_data[6-1] = data[5];
}
