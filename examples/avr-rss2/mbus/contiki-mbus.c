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

  int response = mbus_receive_ack();

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



char *
str_data_combiner(char *text, uint8_t value)
{
  char str[64];
  char data[16];
  sprintf(data, "%d", value);
  strcpy(str, text);
  strcat(str, data);
  return str;
}


char *
str_combiner(char *str1, char *str2)
{
  char str[64];
  strcpy(str, str1);
  strcat(str, str2);
  return str;
}


// THIS IS SO BROKEN

// before calling the function should declare a const char *text_data[144];
int
mbus_parse_data_kamstrup_2101(uint8_t *data, char **text_data)
{
  // strings are hard in C :(
  char tmp_str[64];
  uint16_t tmp = 0;
  uint16_t tmp2 = 0;

  if (data[0] != 0x68 || data[1] != 0x8A || data[2] != 0x8A || data[3] != 0x68)
  {
    printf("The frame is incorrect.\n");
    return -1;
  }



  // checksum check
  // implement later



  text_data[4] = str_data_combiner("Control field: ",data[4]);
  text_data[5] = str_data_combiner("Primary address: ",data[5]);
  text_data[6] = str_data_combiner("CI field: ",data[6]);
  text_data[7] = str_data_combiner(str_data_combiner(str_data_combiner(
       str_data_combiner("ID number: ", data[7]), data[8]), data[9]), data[10]);
  // ^^^ wrong


  tmp = data[11] + (data[12] << 8);
  text_data[11] = str_data_combiner("Manufacturer ID: ", tmp);

  text_data[13] = str_data_combiner("Version ID: ", data[13]);
  text_data[14] = str_data_combiner("Device ID: ", data[14]);
  text_data[15] = str_data_combiner("Number of accesses: ", data[15]);
  text_data[16] = str_data_combiner("Status field: ", data[16]);

  tmp = data[17] + (data[18] << 8);
  text_data[17] = str_data_combiner("Config (not used): ", tmp);

  // tmp = data[21] + (data[22] << 8);
  // tmp2 = data[23] + (data[24] << 8);
  // text_data[21] = str_data_combiner(
  //                 str_data_combiner("Volume (m3, 3 decimals): ", tmp), tmp2);
  //
  // tmp = data[28] + (data[29] << 8);
  // tmp2 = data[30] + (data[31] << 8);
  // text_data[28] = str_data_combiner(
  //                 str_data_combiner("Volume Reverse (m3, 3 decimals): ", tmp), tmp2);
  //
  // tmp = data[34] + (data[35] << 8);
  // tmp2 = data[36] + (data[37] << 8);
  // text_data[34] = str_data_combiner(
  //                 str_data_combiner("Power on (in hours): ", tmp), tmp2);

  tmp = data[40] + (data[41] << 8);
  text_data[40] = str_data_combiner("Flow (in l/h): ", tmp);

  text_data[44] = str_data_combiner("Media temperature (in C): ", data[44]);

  text_data[47] = str_data_combiner("Ambient temperature (in C): ", data[47]);

  tmp = data[50] + (data[51] << 8);
  text_data[50] = str_data_combiner("Flow MIN (in l/h): ", tmp);

  tmp = data[54] + (data[55] << 8);
  text_data[54] = str_data_combiner("Flow MAX (in l/h): ", tmp);

  text_data[58] = str_data_combiner("Media temperature MIN (in C): ", data[58]);

  text_data[63] = str_data_combiner("Media temperature AVG (in C, Kamstrup specific): ", data[63]);

  text_data[66] = str_data_combiner("Ambient temperature MIN (in C): ", data[66]);

  text_data[69] = str_data_combiner("Ambient temperature MAX (in C): ", data[69]);

  text_data[74] = str_data_combiner("Ambient temperature AVG (in C): ", data[74]);

  // time stamp is weird
  // tmp = data[21] + (data[22] << 8);
  // tmp2 = data[23] + (data[24] << 8);
  // text_data[21] = str_data_combiner(
  //                 str_data_combiner("Date and Time: ", tmp), tmp2);

  // tmp = data[83] + (data[84] << 8);
  // tmp2 = data[85] + (data[86] << 8);
  // text_data[83] = str_data_combiner("V1 Target (m3, 3 decimals): ", data[83]);




  tmp = data[89] + (data[90] << 8);
  text_data[89] = str_data_combiner("Flow MIN Month (in l/h): ", tmp);

  tmp = data[93] + (data[94] << 8);
  text_data[93] = str_data_combiner("Flow MAX Month (in l/h): ", tmp);

  text_data[97] = str_data_combiner("Media temperature MIN Month (in C): ", data[97]);

  text_data[102] = str_data_combiner("Media Ambient temperature AVG Month (in C): ", data[102]);

  text_data[105] = str_data_combiner("Media Ambient temperature MIN Month (in C): ", data[105]);

  text_data[108] = str_data_combiner("Media Ambient temperature MAX Month (in C): ", data[108]);

  text_data[113] = str_data_combiner("Ambient temperature AVG Month (in C): ", data[108]);

  // target date skip


  tmp = data[121] + (data[122] << 8);
  text_data[121] = str_data_combiner("Info codes (Kamstrup): ", tmp);

  // config number skip


  tmp = data[135] + (data[136] << 8);
  text_data[135] = str_data_combiner("Meter type (Kamstrup): ", tmp);

  tmp = data[140] + (data[141] << 8);
  text_data[140] = str_data_combiner("Software revision (Kamstrup): ", tmp);

  tmp = 0;

  return 1;
}