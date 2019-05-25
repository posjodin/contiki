#include "usart1.h"

//int wait_for_mbus(void);

//int mbus_verify_frame(uint16_t *data, int reply_type);

//int mbus_ping_frame(int address);

//int mbus_receive_ack();

int mbus_scan_primary_at_address(int address);

int mbus_scan_primary_all();

// secondary scan

//int mbus_send_data_request(int address);

//int mbus_receive_long(uint16_t *data);

int mbus_request_data_at_primary_address(int address, uint16_t *data);

//int mbus_send_address_change(int address, int new_address);

int mbus_set_primary_address(int address, int new_address);

//int mbus_send_frame_switch_baudrate(int address, int baudrate);

int mbus_switch_baudrate(int address, int baudrate);

// send custom message

//int str_data_combiner(char *text, uint16_t value, char **text_data);

int mbus_parse_data_kamstrup_2101(uint16_t *data, char text_data[144][64]);
