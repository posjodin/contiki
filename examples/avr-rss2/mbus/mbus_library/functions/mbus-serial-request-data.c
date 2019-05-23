//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
//#include "../mbus.h"
#include "mbus-serial-request-data.h"

// ^^^ mb will need watchdog_periodic???

static int debug = 1;

int
mbus_serial_request_data()
{
    mbus_frame reply;
    //mbus_frame_data reply_data;

    char *addr_str; //*xml_result;
    //int address, baudrate = 9600;

    int address;

    // input args
    //baudrate = 9600;
    addr_str = "67";
    debug = 1;



    // if (debug)
    // {
    //     mbus_register_send_event(&mbus_dump_send_event);
    //     mbus_register_recv_event(&mbus_dump_recv_event);
    // }


    if (strlen(addr_str) == 16)
    {
        // secondary addressing

        int ret;

        ret = mbus_select_secondary_address(addr_str);

        if (ret == MBUS_PROBE_COLLISION)
        {
            printf("Error: The address mask [%s] matches more than one device.\n", addr_str);
            return 1;
        }
        else if (ret == MBUS_PROBE_NOTHING)
        {
            printf("Error: The selected secondary address does not match any device [%s].\n", addr_str);
            return 1;
        }
        else if (ret == MBUS_PROBE_ERROR)
        {
            printf("Error: Failed to select secondary address [%s].\n", addr_str);
            return 1;
        }
        // else MBUS_PROBE_SINGLE

        if (mbus_send_request_frame(253) == -1)
        {
            printf("Failed to send M-Bus request frame.\n");
            return 1;
        }
    }
    else
    {
        // primary addressing

        address = atoi(addr_str);
        if (mbus_send_request_frame(address) == -1)
        {
            printf("Failed to send M-Bus request frame.\n");
            return 1;
        }
    }

    if (mbus_recv_frame(&reply) == -1)
    {
        printf("Failed to receive M-Bus response frame.\n");
        return 1;
    }

    //
    // parse data and print in XML format
    //
    // if (debug)
    // {
    //     mbus_frame_print(&reply);
    // }

    // if (mbus_frame_data_parse(&reply, &reply_data) == -1)
    // {
    //     printf("M-bus data parse error.\n");
    //     return 1;
    // }

    // if ((xml_result = mbus_frame_data_xml(&reply_data)) == NULL)
    // {
    //     printf("Failed to generate XML representation of MBUS frame: %s\n", mbus_error_str());
    //     return 1;
    // }
    //
    // printf("%s", xml_result);
    // free(xml_result);

    // // manual free
    // if (reply_data.data_var.record)
    // {
    //     mbus_data_record_free(reply_data.data_var.record); // free's up the whole list
    // }

    return 0;
}
