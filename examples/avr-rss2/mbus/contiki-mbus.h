#include "usart1.h"

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
int mbus_request_data_at_primary_address(int address, uint16_t *data);

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

/*
 * Functon for parsing the data received from the long frame.
 * Can be used after mbus_request_data_at_primary_address();
 * Specific for Kamstrup 2101
*/
int mbus_parse_data_kamstrup_2101(uint16_t *data, char text_data[37][32]);

int mbus_parse_data_kamstrup_2101_units(char text_units[37][32]);
int mbus_parse_data_kamstrup_2101_names(char text_names[37][32]);
int mbus_parse_data_kamstrup_2101_datas(uint16_t *data, char text_data[37][32]);
