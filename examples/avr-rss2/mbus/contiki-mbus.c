#include "sys/etimer.h"
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
  int baudrate = 9600; // change later
  if (baudrate == 300)
  {
    clock_delay_msec(4000);
  } else
  if (baudrate == 2400)
  {
    clock_delay_msec(1000);
  } else
  if (baudrate == 9600)
  {
    clock_delay_msec(300);
  } else
  {
    printf("Unsupported baudrate!\n");
    return -1;
  }
  return 1;
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
      usart1_tx(&frame[i]);
    }
  return 1;
}


// mb have like a char *
int
mbus_receive_ack()
{
  int response = 0;
  uint16_t buff[1];

  usart1_rx(buff);
  response = buff[0];

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
    printf("M-Bus device at %d has sent an ACK.\n\n", address);
    return response;
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
      printf("Scan failed at the following address: %d\n", address);
    }

    if (wait_for_mbus() == -1)
      return -1;

    int response = mbus_receive_ack();

    clock_delay_msec(3);

    if (response == MBUS_ANSWER_ACK)
    {
      printf("M-Bus device at %d has sent an ACK.\n\n", address);
    } else {
      printf("No M-Bus device found at the address %d\n", address);
    }

    watchdog_periodic();
  }
  return 1;
}



int
mbus_scan_secondary_all()
{
  //to be added later
  return 1;
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
    usart1_tx(&frame[i]);
  }
  return 1;
}



int
mbus_receive_long(uint16_t *data)
{
  uint16_t buff[MBUS_FRAME_SIZE_LONG]; // can be 0?
  memset((void *)buff, 0, sizeof(buff));

  for (int i = 0; i < MBUS_FRAME_SIZE_LONG; i++) {
     usart1_rx(buff);
     data[i] = buff[0];     // buff[0] is it really correct? should be &buff
  }

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

  mbus_receive_long(received_data);

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
      usart1_tx(&frame[i]);
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
    printf("M-Bus device at %d has sent an ACK.\n", address);
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
      usart1_tx(&frame[i]);
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
    printf("M-Bus device at %d has sent an ACK.\n", address);
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
  return 1;
}



int
str_data_combiner(char *text, uint16_t value, char *text_data)
{
  sprintf(text_data, "%s:%d", text, value);
  return 1;
}

int
str_data_combiner_id(char *text, uint16_t value1, uint16_t value2, uint16_t value3, uint16_t value4, char *text_data)
{
  sprintf(text_data, "%s:%x%x%x%x", text, value4, value3, value2, value1);
  return 1;
}



int
mbus_parse_data_kamstrup_2101_names(char text_names[37][32])
{
  sprintf(text_names[0],"%s", "Control Field");
  sprintf(text_names[1],"%s", "Primary address");
  sprintf(text_names[2],"%s", "CI field");
  sprintf(text_names[3],"%s", "Manufacturer ID");
  sprintf(text_names[4],"%s", "Version ID");
  sprintf(text_names[5],"%s", "Device ID");
  sprintf(text_names[6],"%s", "Number of accesses");
  sprintf(text_names[7],"%s", "Status Field");
  sprintf(text_names[8],"%s", "Config (not used)");
  sprintf(text_names[9],"%s", "Volume");
  sprintf(text_names[10],"%s", "Vol.Rev.");
  sprintf(text_names[11],"%s", "Power on");
  sprintf(text_names[12],"%s", "Flow");
  sprintf(text_names[13],"%s", "Media temp.");
  sprintf(text_names[14],"%s", "Amb. temp.");
  sprintf(text_names[15],"%s", "Flow MIN");
  sprintf(text_names[16],"%s", "Flow MAX");
  sprintf(text_names[17],"%s", "Media temp. MIN");
  sprintf(text_names[18],"%s", "Media temp. AVG");
  sprintf(text_names[19],"%s", "Ambient temp. MIN");
  sprintf(text_names[20],"%s", "Ambient temp. MAX");
  sprintf(text_names[21],"%s", "Ambient temp. AVG");
  sprintf(text_names[22],"%s", "Date and Time");
  sprintf(text_names[23],"%s", "V1 Target");
  sprintf(text_names[24],"%s", "Flow MIN Month");
  sprintf(text_names[25],"%s", "Flow MAX Month");
  sprintf(text_names[26],"%s", "Media temp.MIN Month");
  sprintf(text_names[27],"%s", "Media Amb.temp.AVG Month");
  sprintf(text_names[28],"%s", "Media Amb.temp.MIN Month");
  sprintf(text_names[29],"%s", "Media Amb.temp.MAX Month");
  sprintf(text_names[30],"%s", "Amb.temp.AVG Month(C)");
  sprintf(text_names[31],"%s", "Target date (Kamstrup)");
  sprintf(text_names[32],"%s", "Info codes (Kamstrup)");
  sprintf(text_names[33],"%s", "Config number (KAM)");
  sprintf(text_names[34],"%s", "Meter type (KAM)");
  sprintf(text_names[35],"%s", "Software revision (KAM)");
  sprintf(text_names[36],"%s", "ID number");

  return 1;
}

/*

int
mbus_parse_data_kamstrup_2101_names(char text_names[37][32])
{
  sprintf(text_names[0],"%s", "Control Field");
  sprintf(text_names[1],"%s", "Primary address");
  sprintf(text_names[2],"%s", "CI field");
  sprintf(text_names[3],"%s", "Manufacturer ID");
  sprintf(text_names[4],"%s", "Version ID");
  sprintf(text_names[5],"%s", "Device ID");
  sprintf(text_names[6],"%s", "Number of accesses");
  sprintf(text_names[7],"%s", "Status Field");
  sprintf(text_names[8],"%s", "Config (not used)");
  sprintf(text_names[29],"%s", "Volume");
  sprintf(text_names[10],"%s", "Vol.Rev.");
  sprintf(text_names[11],"%s", "Power on(hours)");
  sprintf(text_names[12],"%s", "Flow(l/h)");
  sprintf(text_names[13],"%s", "Media temp.(C)");
  sprintf(text_names[14],"%s", "Amb. temp.(C)");
  sprintf(text_names[15],"%s", "Flow MIN (l/h)");
  sprintf(text_names[16],"%s", "Flow MAX (l/h)");
  sprintf(text_names[17],"%s", "Media temp. MIN (C)");
  sprintf(text_names[18],"%s", "Media temp. AVG (C, KAM)");
  sprintf(text_names[19],"%s", "Ambient temp. MIN (C)");
  sprintf(text_names[20],"%s", "Ambient temp. MAX (C)");
  sprintf(text_names[21],"%s", "Ambient temp. AVG (C)");
  sprintf(text_names[22],"%s", "Date and Time");
  sprintf(text_names[23],"%s", "V1 Target (m3, 3 decimals)");
  sprintf(text_names[24],"%s", "Flow MIN Month (in l/h)");
  sprintf(text_names[25],"%s", "Flow MAX Month (in l/h)");
  sprintf(text_names[26],"%s", "Media temp.MIN Month (C)");
  sprintf(text_names[27],"%s", "Media Amb.temp.AVG Month(C)");
  sprintf(text_names[28],"%s", "Media Amb.temp.MIN Month(C)");
  sprintf(text_names[29],"%s", "Media Amb.temp.MAX Month(C)");
  sprintf(text_names[30],"%s", "Amb.temp.AVG Month(C)");
  sprintf(text_names[31],"%s", "Target date (Kamstrup)");
  sprintf(text_names[32],"%s", "Info codes (Kamstrup)");
  sprintf(text_names[33],"%s", "Config number (KAM)");
  sprintf(text_names[34],"%s", "Meter type (KAM)");
  sprintf(text_names[35],"%s", "Software revision (KAM)");
  sprintf(text_names[36],"%s", "ID number");

  return 1;
}
*/


int
mbus_parse_data_kamstrup_2101_units(char text_units[37][32])
{
  sprintf(text_units[0],"%s", "num");
  sprintf(text_units[1],"%s", "num");
  sprintf(text_units[2],"%s", "num");
  sprintf(text_units[3],"%s", "num");
  sprintf(text_units[4],"%s", "num");
  sprintf(text_units[5],"%s", "num");
  sprintf(text_units[6],"%s", "num");
  sprintf(text_units[7],"%s", "num");
  sprintf(text_units[8],"%s", "num");
  sprintf(text_units[9],"%s", "m3, 3 dec.");
  sprintf(text_units[10],"%s", "m3, 3 dec.");
  sprintf(text_units[11],"%s", "hours");
  sprintf(text_units[12],"%s", "l/h");
  sprintf(text_units[13],"%s", "C");
  sprintf(text_units[14],"%s", "C");
  sprintf(text_units[15],"%s", "l/h");
  sprintf(text_units[16],"%s", "l/h");
  sprintf(text_units[17],"%s", "C");
  sprintf(text_units[18],"%s", "C");
  sprintf(text_units[19],"%s", "C");
  sprintf(text_units[20],"%s", "C");
  sprintf(text_units[21],"%s", "C");
  sprintf(text_units[22],"%s", "s");
  sprintf(text_units[23],"%s", "m3, 3 dec.");
  sprintf(text_units[24],"%s", "l/h");
  sprintf(text_units[25],"%s", "l/h");
  sprintf(text_units[26],"%s", "C");
  sprintf(text_units[27],"%s", "C");
  sprintf(text_units[28],"%s", "C");
  sprintf(text_units[29],"%s", "C");
  sprintf(text_units[30],"%s", "C");
  sprintf(text_units[31],"%s", "num");
  sprintf(text_units[32],"%s", "num");
  sprintf(text_units[33],"%s", "num");
  sprintf(text_units[34],"%s", "num");
  sprintf(text_units[35],"%s", "num");
  sprintf(text_units[36],"%s", "num");

  return 1;
}



int
mbus_parse_data_kamstrup_2101_datas(uint16_t *data, char text_data[37][32])
{
  uint32_t tmp = 0;

  sprintf(text_data[0],"%u", data[4]);
  sprintf(text_data[1],"%u", data[5]);
  sprintf(text_data[2],"%u", data[6]);

  tmp = data[11] + (data[12] << 8);
  sprintf(text_data[3],"%lu", tmp);

  sprintf(text_data[4],"%u", data[13]);
  sprintf(text_data[5],"%u", data[14]);
  sprintf(text_data[6],"%u", data[15]);
  sprintf(text_data[7],"%u", data[16]);

  tmp = data[17] + (data[18] << 8);
  sprintf(text_data[8],"%lu", tmp);

  tmp = (uint32_t) data[21] + ((uint32_t) data[22] << 8) + ((uint32_t) data[23] << 16) + ((uint32_t) data[24] << 24);
  sprintf(text_data[9],"%lu", tmp);

  tmp =  data[28] + ((uint32_t) data[29] << 8) + ((uint32_t) data[30] << 16) + ((uint32_t) data[31] << 24);
  sprintf(text_data[10],"%lu", tmp);

  tmp = data[34] + ((uint32_t) data[35] << 8) + ((uint32_t) data[36] << 16) + ((uint32_t) data[37] << 24);
  sprintf(text_data[11],"%lu", tmp);

  tmp = data[40] + (data[41] << 8);
  sprintf(text_data[12],"%lu", tmp);

  sprintf(text_data[13],"%u", data[44]);
  sprintf(text_data[14],"%u", data[47]);

  tmp = data[50] + (data[51] << 8);
  sprintf(text_data[15],"%lu", tmp);

  tmp = data[54] + (data[55] << 8);
  sprintf(text_data[16],"%lu", tmp);

  sprintf(text_data[17],"%u", data[58]);
  sprintf(text_data[18],"%u", data[63]);
  sprintf(text_data[19],"%u", data[66]);
  sprintf(text_data[20],"%u", data[69]);
  sprintf(text_data[21],"%u", data[74]);

  tmp = data[77] + ((uint32_t) data[78] << 8) + ((uint32_t) data[79] << 16) + ((uint32_t) data[80] << 24);
  sprintf(text_data[22],"%lu", tmp);

  tmp = data[83] + ((uint32_t) data[84] << 8) + ((uint32_t) data[85] << 16) + ((uint32_t) data[86] << 24);
  sprintf(text_data[23],"%lu", tmp);

  tmp = data[89] + (data[90] << 8);
  sprintf(text_data[24],"%lu", tmp);

  tmp = data[93] + (data[94] << 8);
  sprintf(text_data[25],"%lu", tmp);

  sprintf(text_data[26],"%u", data[97]);
  sprintf(text_data[27],"%u", data[102]);
  sprintf(text_data[28],"%u", data[105]);
  sprintf(text_data[29],"%u", data[108]);
  sprintf(text_data[30],"%u", data[113]);

  tmp = data[116] +(data[117] << 8);
  sprintf(text_data[31],"%lu", tmp);

  tmp = data[121] + (data[122] << 8);
  sprintf(text_data[32],"%lu", tmp);

  tmp = data[126] + ((uint32_t) data[127] << 8) + ((uint32_t) data[128] << 16) + ((uint32_t) data[129] << 24);
  sprintf(text_data[33],"%lu", tmp);

  tmp = data[135] + (data[136] << 8);
  sprintf(text_data[34],"%lu", tmp);

  tmp = data[140] + (data[141] << 8);
  sprintf(text_data[35],"%lu", tmp);


  sprintf(text_data[36],"%x%x%x%x", data[7], data[8], data[9], data[10]);

  return 1;
}



int
mbus_parse_data_kamstrup_2101(uint16_t *data, char text_data[37][32])
{
  uint32_t tmp = 0;

  if (data[0] != 0x68 || data[1] != 0x8A || data[2] != 0x8A || data[3] != 0x68)
  {
    printf("The frame is incorrect.\n");
    return -1;
  }


  // checksum check
  // implement later


  str_data_combiner("Control field", data[4], text_data[0]);


  str_data_combiner("Primary address",data[5], text_data[1]);
  str_data_combiner("CI field",data[6], text_data[2]);

  // str_data_combiner(str_data_combiner(str_data_combiner(
  //      str_data_combiner("ID number: ", data[10], text_data[7]), data[9],
  //      text_data[7]), data[8], text_data[7]), data[7], text_data[7]);

  str_data_combiner_id("ID number",data[7], data[8], data[9], data[10], text_data[36]);




  tmp = data[11] + (data[12] << 8);
  str_data_combiner("Manufacturer ID", tmp, text_data[3]);

  str_data_combiner("Version ID", data[13], text_data[4]);
  str_data_combiner("Device ID", data[14], text_data[5]);
  str_data_combiner("Number of accesses", data[15], text_data[6]);
  str_data_combiner("Status field", data[16], text_data[7]);

  tmp = data[17] + (data[18] << 8);
  str_data_combiner("Config (not used)", tmp, text_data[8]);

  tmp = (uint32_t) data[21] + ((uint32_t) data[22] << 8) + ((uint32_t) data[23] << 16) + ((uint32_t) data[24] << 24);
  str_data_combiner("Volume(m3, 3 dec)", tmp, text_data[9]);

  tmp =  data[28] + ((uint32_t) data[29] << 8) + ((uint32_t) data[30] << 16) + ((uint32_t) data[31] << 24);
  str_data_combiner("Vol.Rev.(m3, 3 dec)", tmp, text_data[10]);

  tmp = data[34] + ((uint32_t) data[35] << 8) + ((uint32_t) data[36] << 16) + ((uint32_t) data[37] << 24);
  str_data_combiner("Power on(hours)", tmp, text_data[11]);

  tmp = data[40] + (data[41] << 8);
  str_data_combiner("Flow(l/h)", tmp, text_data[12]);

  str_data_combiner("Media temp.(C)", data[44], text_data[13]);

  str_data_combiner("Amb. temp.(C)", data[47], text_data[14]);

  tmp = data[50] + (data[51] << 8);
  str_data_combiner("Flow MIN (l/h)", tmp, text_data[15]);

  tmp = data[54] + (data[55] << 8);
  str_data_combiner("Flow MAX (l/h)", tmp, text_data[16]);

  str_data_combiner("Media temp. MIN (C)", data[58], text_data[17]);

  str_data_combiner("Media temp. AVG (C, KAM)", data[63], text_data[18]);

  str_data_combiner("Ambient temp. MIN (C)", data[66], text_data[19]);

  str_data_combiner("Ambient temp. MAX (C)", data[69], text_data[20]);

  str_data_combiner("Ambient temp. AVG (C)", data[74], text_data[21]);

  tmp = data[77] + ((uint32_t) data[78] << 8) + ((uint32_t) data[79] << 16) + ((uint32_t) data[80] << 24);

  str_data_combiner("Date and Time", tmp, text_data[22]);

  tmp = data[83] + ((uint32_t) data[84] << 8) + ((uint32_t) data[85] << 16) + ((uint32_t) data[86] << 24);
  str_data_combiner("V1 Target (m3, 3 decimals)", data[83], text_data[23]);

  tmp = data[89] + (data[90] << 8);
  str_data_combiner("Flow MIN Month (in l/h)", tmp, text_data[24]);

  tmp = data[93] + (data[94] << 8);
  str_data_combiner("Flow MAX Month (in l/h)", tmp, text_data[25]);

  str_data_combiner("Media temp.MIN Month (C)", data[97], text_data[26]);

  str_data_combiner("Media Amb.temp.AVG Month(C)", data[102], text_data[27]);

  str_data_combiner("Media Amb.temp.MIN Month(C)", data[105], text_data[28]);

  str_data_combiner("Media Amb.temp.MAX Month(C)", data[108], text_data[29]);

  str_data_combiner("Amb.temp.AVG Month(C)", data[113], text_data[30]);

  tmp = data[116] +(data[117] << 8);
  str_data_combiner("Target date (Kamstrup)", tmp, text_data[31]);

  tmp = data[121] + (data[122] << 8);
  str_data_combiner("Info codes (Kamstrup)", tmp, text_data[32]);

  tmp = data[126] + ((uint32_t) data[127] << 8) + ((uint32_t) data[128] << 16) + ((uint32_t) data[129] << 24);
  str_data_combiner("Config number (KAM)", tmp, text_data[33]);

  tmp = data[135] + (data[136] << 8);
  str_data_combiner("Meter type (KAM)", tmp, text_data[34]);

  tmp = data[140] + (data[141] << 8);
  str_data_combiner("Software revision (KAM)", tmp, text_data[35]);

  tmp = 0;

  return 1;
}
