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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <mbus/mbus.h>

static int debug = 0;

//------------------------------------------------------------------------------
// Primary addressing scanning of mbus devices.
//------------------------------------------------------------------------------

/*

instead of mbus_serial_set_baudrate and mbus_connect_serial write our own functions

Dont need to mbus_disconnect.

NOTE: mbus_serial_set_baudrate sets baudrate just for the library. Can be
simplified or omitted.


*/





int
main(int argc, char **argv)
{
    mbus_handle *handle;
    char *device;
    int address, baudrate = 9600;
    int ret;

    //debug = 0;
    //device = RS232_PORT_1;
    //baudrate = 9600;

    if (debug)
    {
        mbus_register_send_event(&mbus_dump_send_event);
        mbus_register_recv_event(&mbus_dump_recv_event);
    }

    if ((handle = mbus_connect_serial(device)) == NULL)
    // ^^^ dont need mbus connect serial???
    {
        printf("Failed to setup connection to M-bus gateway.\n");
        return 1;
    }

    if (mbus_serial_set_baudrate(handle->m_serial_handle, baudrate) == -1)
    // ^^^
    {
        printf("Failed to set baud rate.\n");
        return 1;
    }

    if (debug)
        printf("Scanning primary addresses:\n");

    for (address = 0; address <= 250; address++)
    {
        mbus_frame reply;

        memset((void *)&reply, 0, sizeof(mbus_frame));

        if (debug)
        {
            printf("%d ", address);
            fflush(stdout);
        }

        if (mbus_send_ping_frame(handle, address) == -1)
        {
            printf("Scan failed. Could not send ping frame: %s\n", mbus_error_str());
            return 1;
        }

        ret = mbus_recv_frame(handle, &reply);

        if (ret == -1)
        {
            continue;
        }

        if (debug)
            printf("\n");

        if (ret == -2)
        {
            /* check for more data (collision) */
            while (mbus_recv_frame(handle, &reply) != -1);

            printf("Collision at address %d\n", address);

            continue;
        }

        if (mbus_frame_type(&reply) == MBUS_FRAME_TYPE_ACK)
        {
            /* check for more data (collision) */
            while (mbus_recv_frame(handle, &reply) != -1)
            {
                ret = -2;
            }

            if (ret == -2)
            {
                printf("Collision at address %d\n", address);

                continue;
            }

            printf("Found a M-Bus device at address %d\n", address);
        }
    }

    mbus_disconnect(handle);
    return 0;
}
