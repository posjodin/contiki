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
#include <mbus-serial-scan.h>

static int debug = 1;

//------------------------------------------------------------------------------
// Primary addressing scanning of mbus devices.
//------------------------------------------------------------------------------

/*

Search for "^^^" to find our comments

*/





int
mbus_scan()
{
    int ret;

    //debug = 0;
    //device = RS232_PORT_1;

    if (debug)
    {
        mbus_register_send_event(&mbus_dump_send_event);
        mbus_register_recv_event(&mbus_dump_recv_event);
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

        if (mbus_send_ping_frame(address) == -1)
        {
            printf("Scan failed. Could not send ping frame: %s\n", mbus_error_str());
            return 1;
        }

        ret = mbus_recv_frame(&reply);

        if (ret == -1)
        {
            continue;
        }

        if (debug)
            printf("\n");

        if (ret == -2)
        {
            /* check for more data (collision) */
            while (mbus_recv_frame(&reply) != -1);

            printf("Collision at address %d\n", address);

            continue;
        }

        if (mbus_frame_type(&reply) == MBUS_FRAME_TYPE_ACK)
        {
            /* check for more data (collision) */
            while (mbus_recv_frame(&reply) != -1)
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


    return 0;
}
