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
#include "sys/etimer.h"
#include <unistd.h>
#include <string.h>
#include "dev/watchdog.h"
#include <stdio.h>
#include "mbus-serial-scan.h"

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
    int address = 0;

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
            printf("\n");
            printf("Trying to connect to:\n");
            printf("%d\n", address);
            fflush(stdout);
        }

        //printf("before send ping frame\n");
        if (mbus_send_ping_frame(address) == -1)
        {
            printf("Scan failed. Could not send ping frame: %s\n", mbus_error_str());
            return 1;
        }
        //printf("before recv frame\n");
        ret = mbus_recv_frame(&reply);

        if (ret == -1)
        {
            continue;
        }

        // if (debug)
        //     printf("\n");

        if (ret == -2)
        {
            /* check for more data (collision) */
            while (mbus_recv_frame(&reply) != -1);

            printf("Collision at address %d\n", address);

            continue;
        }

        //printf("before checking frame type\n");
        printf("%d\n", mbus_frame_type(&reply));
        if (mbus_frame_type(&reply) == MBUS_FRAME_TYPE_ACK)
        {
            printf("Received an ACK.\n");
            /* check for more data (collision) */
            //while (mbus_recv_frame(&reply) != -1)
            //{
            //    ret = -2;
            //}

            //if (ret == -2)
            //{
            //    printf("Collision at address %d\n", address);

//                continue;
  //          }

            printf("Found a M-Bus device at address %d\n", address);
        }
        //printf("after frame type\n");
        watchdog_periodic();
        //clock_delay_msec(100);
    }



    return 0;
}
