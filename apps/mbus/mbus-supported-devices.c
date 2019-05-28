/**
 * \file
 *         Contiki-OS M-Bus functionality
 * \authors
 *          Albert Asratyan https://github.com/Goradux
 *          Mandar Joshi https://github.com/mandaryoshi
 *
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "contiki-mbus.h"
#include "mbus-supported-devices.h"



/*

M-Bus parser template:

MQTT client requires three fields for data transmission: name, unit, value.

Value is stored in the array filled by mbus_request_data_at_primary_address()
function. Names and unit arrays should be created according to the template
below:

int
mbus_parse_data_kamstrup_2101_names(char text_names[37][32])
{
  sprintf(text_names[0],"%s", "Control Field");
  sprintf(text_names[1],"%s", "Primary address");
  sprintf(text_names[2],"%s", "CI field");
  ...
  ...
  ...
  return 1;
}

The data array has to be rewritten to an array of strings using the following
format. If a number is composed of multiple bytes, use the bitwise shift
operation, as shown below.

int
mbus_parse_data_kamstrup_2101_datas(uint16_t *data, char text_data[37][32])
{
  uint32_t tmp = 0;
  sprintf(text_data[0],"%u", data[4]);
  sprintf(text_data[1],"%u", data[5]);
  sprintf(text_data[2],"%u", data[6]);

  tmp = data[11] + (data[12] << 8);
  sprintf(text_data[3],"%lu", tmp);
  ...
  ...
  return 1;
}

NOTE: the functions above have 37 entries because it is the number of varible
fields reported by Kamstrup flowIQ 2101 water meter. 32 is the length of the
strings. For names, recommended string length is 32, for units - 8, for data -
8.

*/


/*---------------------------------------------------------------------------*/

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

/*---------------------------------------------------------------------------*/

/**
 * \brief      Fills an empty array with names for KAM flowIQ2101 variables
 * \param text_names[37][32]  Empty array that will be filled with names
 * \retval 1   returns 1 on successfull completion
 *
 *             Fills the array with the Kamstrup official flowIQ2101 long
 *             frame labels. Before calling this function, an array of strings
 *             has to be declared with the exact same size as the argument.
 */
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

/*---------------------------------------------------------------------------*/

/**
 * \brief      Fills an empty array with units for KAM flowIQ2101 variables
 * \param text_units[37][8]  Empty array that will be filled with units for data
 * \retval 1   returns 1 on successfull completion
 *
 *             Fills the array with the Kamstrup official flowIQ2101 long
 *             frame unit labels. Before calling this function, an array of strings
 *             has to be declared with the exact same size as the argument.
 */
int
mbus_parse_data_kamstrup_2101_units(char text_units[37][8])
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
  sprintf(text_units[9],"%s", "m3,3d");
  sprintf(text_units[10],"%s", "m3,3d");
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
  sprintf(text_units[23],"%s", "m3,3d");
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

/*---------------------------------------------------------------------------*/


/**
 * \brief      Fills an empty array with data for KAM flowIQ2101 variables
 * \param text_data[37][8]  Empty array that will be filled with units for data
 * \retval 1   returns 1 on successfull completion
 *
 *             Fills the array with the Kamstrup official flowIQ2101 long
 *             frame data values. Before calling this function, an array of strings
 *             has to be declared with the exact same size as the argument.
 *
 *             Configuration number (text_data[33]) is not displayed correctly
 *             because it is a 64 bit number, which can not be calculated properly.
 */
int
mbus_parse_data_kamstrup_2101_datas(uint16_t *data, char text_data[37][8])
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


  sprintf(text_data[36],"%x%x%x%x", data[10], data[9], data[8], data[7]);

  return 1;
}

/*---------------------------------------------------------------------------*/

/**
 * \brief      Fills an empty array with data for KAM flowIQ2101 variables
 * \param text_names[37][8]  Empty array that will be filled with names and data
 * \uint16_t *data holds the raw hex M-Bus data
 * \retval 1   returns 1 on successfull completion
 * \retval -1   returns -1 if the first four bytes are wrong.
 *
 *             Fills the array with the Kamstrup official flowIQ2101 long
 *             frame data names and values. Combines them both into one string
 *             and writes that string into the text_data[37][32] array.
 */
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

  str_data_combiner_id("ID number",data[7], data[8], data[9], data[10], text_data[36]);

  tmp = data[11] + (data[12] << 8);
  str_data_combiner("Manufacturer ID", tmp, text_data[3]);

  str_data_combiner("Version ID", data[13], text_data[4]);
  str_data_combiner("Device ID", data[14], text_data[5]);
  str_data_combiner("Number of accesses", data[15], text_data[6]);
  str_data_combiner("Status field", data[16], text_data[7]);

  tmp = data[17] + (data[18] << 8);
  str_data_combiner("Config (not used)", tmp, text_data[8]);

  tmp = data[21] + ((uint32_t) data[22] << 8) + ((uint32_t) data[23] << 16) + ((uint32_t) data[24] << 24);
  str_data_combiner("Volume(m3, 3 dec)", tmp, text_data[9]);

  tmp = data[28] + ((uint32_t) data[29] << 8) + ((uint32_t) data[30] << 16) + ((uint32_t) data[31] << 24);
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

/*---------------------------------------------------------------------------*/
