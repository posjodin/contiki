/*
 * Functon for parsing the data received from the long frame.
 * Can be used after mbus_request_data_at_primary_address();
 * Specific for Kamstrup 2101
*/
int mbus_parse_data_kamstrup_2101(uint16_t *data, char text_data[37][32]);

int mbus_parse_data_kamstrup_2101_names(char text_names[37][32]);
int mbus_parse_data_kamstrup_2101_units(char text_units[37][8]);
int mbus_parse_data_kamstrup_2101_datas(uint16_t *data, char text_data[37][8]);
