//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson and contributors, Raditex AB
// All rights reserved.
//
// rSCADA
// http://www.rSCADA.se
// info@rscada.se
//
// Contributors:
// Large parts of this file was contributed by Tomas Menzl.
//
//------------------------------------------------------------------------------

/**
 * @file   mbus-protocol-aux.h
 *
 * @brief  Auxiliary functions to the Freescada libmbus library
 *
 * \endverbatim
 *
 * Note that the quantity values are partially "normalized". For example energy
 * is in Wh even when originally used "decimal" prefixes like MWh. This seems to
 * be sensible as it make easier any processing of a dataset with huge
 * fluctuation (no need to lookup/convert the prefixes). Also the potential
 * conversion to desired units is pretty easy.
 *
 * On the other hand we acknowledge that use of certain units are expected so we
 * do not convert all units to Si (i.e. no conversion from J to Wh or Bar to Pa.)
 */

#ifndef __MBUS_PROTOCOL_AUX_H__
#define __MBUS_PROTOCOL_AUX_H__

#include <mbus/mbus.h>
#include <mbus/mbus-protocol.h>
#include <mbus/mbus-serial.h>

#define MBUS_PROBE_NOTHING   0
#define MBUS_PROBE_SINGLE    1
#define MBUS_PROBE_COLLISION 2
#define MBUS_PROBE_ERROR     -1



/**
 * MBus slave address type (primary/secodary address)
 */
typedef struct _mbus_address {
    char is_primary;             /**< Address type (non zero for primary) */
    union {
        int    primary;         /**< Primary address (for slaves shall be from 1 to 250) */
        char * secondary;       /**< Secondary address (shall be 16 digits) */
    };
} mbus_address;


/**
 * _string type
 *
 * In general support binary strings (octet string, non zero terminated)
 * but so far almost all strings are zero terminated ("texts").
 */
typedef struct _mbus_string {
    char * value;               /**< Buffer */
    int    size;                /**< _size */
} mbus_string;


/**
 * Quantity value type union
 */
typedef union _mbus_value {
    double      real_val;   /**< Numerical  */
    mbus_string str_val;    /**< Text/binary */
} mbus_value;


/**
 * Single measured quantity record type
 */
typedef struct _mbus_record {
    mbus_value value;          /**< Quantity value */
    char                is_numeric;      /**< Quantity value type (nonzero is numeric) */
    char               *unit;           /**< Quantity unit (e.g. Wh) */
    char               *function_medium; /**< Quantity medium or function (e.g. Electricity) */
    char               *quantity;       /**< Quantity type (e.g. Energy) */
} mbus_record;


/**
 * Event callback functions
 */
extern void (*_mbus_scan_progress)(const char *mask);
extern void (*_mbus_found_event)(mbus_frame *frame);

/**
 * Event register functions
 */
void mbus_register_scan_progress(void (*event)(const char *mask));
void mbus_register_found_event(void (*event)(mbus_frame *frame));




/**
 * Receives a frame
 *
 *
 * @param frame  Received frame
 *
 * @return Zero when successful.
 */
int mbus_recv_frame(mbus_frame *frame);

/**
 * Sends frame
 *
 *
 * @param frame  Frame to send
 *
 * @return Zero when successful.
 */
int mbus_send_frame(mbus_frame *frame);

/**
 * Sends secodary address selection frame
 *
 *
 * @param secondary_addr_str Secondary address
 *
 * @return Zero when successful.
 */
int mbus_send_select_frame(const char *secondary_addr_str);

/**
 * Sends switch baudrate frame
 *
 *
 * @param address  Address (0-255)
 * @param baudrate Baudrate (300,600,1200,2400,4800,9600,19200,38400)
 *
 * @return Zero when successful.
 */
int mbus_send_switch_baudrate_frame(int address, int baudrate);

/**
 * Sends request frame to given slave
 *
 *
 * @param address Address (0-255)
 *
 * @return Zero when successful.
 */
int mbus_send_request_frame(int address);

/**
 * Sends a request and read replies until no more records available
 * or limit is reached.
 *
 *
 * @param address    Address (0-255)
 * @param reply      pointer to an mbus frame for the reply
 * @param max_frames limit of frames to readout (0 = no limit)
 *
 * @return Zero when successful.
 */
int mbus_sendrecv_request(int address, mbus_frame *reply, int max_frames);

/**
 * Sends ping frame to given slave
 *
 *
 * @param address Address (0-255)
 *
 * @return Zero when successful.
 */
int mbus_send_ping_frame(int address);

/**
 * Select slave by secondary address
 *
 *
 * @param mask          Address/mask to select
 *
 * @return See MBUS_PROBE_* constants
 */
int mbus_select_secondary_address(const char *mask);

/**
 * Probe/address slave by secondary address
 *
 *
 * @param mask          Address/mask to probe
 * @param matching_addr Matched address (the buffer has tobe at least 16 bytes)
 *
 * @return See MBUS_PROBE_* constants
 */
int mbus_probe_secondary_address(const char *mask, char *matching_addr);

/**
 * Read data from given slave using address types
 *
 *
 * @param address Address of the slave
 * @param reply   Reply from the slave
 *
 * @return Zero when successful.
 */
int mbus_read_slave(mbus_address *address, mbus_frame *reply);


/**
 * Allocate new data record. Use #mbus_record_free when finished.
 *
 * @return pointer to the new record, NULL when failed
 */
mbus_record * mbus_record_new();

/**
 * Destructor for mbus_record
 *
 * @param rec record to be freed
 */
void mbus_record_free(mbus_record *rec);


/**
 * Create/parse single counter from the fixed data structure
 *
 * @param statusByte     status byte
 * @param medium_unit_byte medium/unit byte
 * @param data           pointer to the data counter (4 bytes)
 *
 * @return Newly allocated record if succesful, NULL otherwise. Later on need to use #mbus_record_free
 */
mbus_record *mbus_parse_fixed_record(char statusByte, char medium_unit_byte, uint8_t *data);


/**
 * Create/parse single counter from the variable data structure record
 *
 * @param data record data to be parsed
 *
 * @return Newly allocated record if succesful, NULL otherwise. Later on need to use #mbus_record_free
 */
mbus_record * mbus_parse_variable_record(mbus_data_record *record);



/**
 * Get normalized counter value for a fixed counter
 *
 * Get "normalized" value and unit of the counter
 *
 * @param medium_unit_byte medium/unit byte of the fixed counter
 * @param medium_value    raw counter value
 * @param unit_out        units of the counter - use free when done
 * @param value_out       resulting counter value
 * @param quantity_out    parsed quantity, when done use "free"
 *
 * @return zero when OK
 */
int mbus_data_fixed_normalize(int medium_unit_byte, long medium_value, char **unit_out, double *value_out, char **quantity_out);



/**
 * Decode value of a variable data structure
 *
 * @param record          record to be decoded
 * @param value_out_real    numerical counter value output (when numerical)
 * @param value_out_str     string counter value output (when string, NULL otherwise), when finished use "free *value_out_str"
 * @param value_out_str_size string counter value size
 *
 * @return zero when OK
 */
int mbus_data_variable_value_decode(mbus_record *record, double *value_out_real, char **value_out_str, int *value_out_str_size);

/**
 * Decode units and normalize value using VIF/VIFE (used internally by mbus_vib_unit_normalize)
 *
 * @param vif         VIF (including standard extensions)
 * @param value       already parsed "raw" numerical value
 * @param unit_out     parsed unit, when done use "free"
 * @param value_out    normalized value
 * @param quantity_out parsed quantity, when done use "free"
 *
 * @return zero when OK
 */
int mbus_vif_unit_normalize(int vif, double value, char **unit_out, double *value_out, char **quantity_out);

/**
 * Decode units and normalize value from VIB
 *
 * @param vib         mbus value information block of the variable record
 * @param value       already parsed "raw" numerical value
 * @param unit_out     parsed unit, when done use "free"
 * @param value_out    normalized value
 * @param quantity_out parsed quantity, when done use "free"
 *
 * @return zero when OK
 */
int mbus_vib_unit_normalize(mbus_value_information_block *vib, double value, char **unit_out, double *value_out, char ** quantity_out);

/**
 * Generate XML for normalized variable-length data
 *
 * @param data    variable-length data
 *
 * @return string with XML
 */
char * mbus_data_variable_xml_normalized(mbus_data_variable *data);

/**
 * Return a string containing an XML representation of the normalized M-BUS frame data.
 *
 * @param data    M-Bus frame data
 *
 * @return string with XML
 */
char * mbus_frame_data_xml_normalized(mbus_frame_data *data);

/**
 * Iterate over secondary addresses, send a probe package to all addresses matching
 * the given addresses mask.
 *
 * @param pos         current address
 * @param addr_mask   address mask to
 *
 * @return zero when OK
 */
int mbus_scan_2nd_address_range(int pos, char *addr_mask);

#endif // __MBUS_PROTOCOL_AUX_H__
