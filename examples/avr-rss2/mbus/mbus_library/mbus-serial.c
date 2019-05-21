//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>

#include "sys/etimer.h"


#include <stdio.h>

#include <errno.h>
#include <string.h>

#include "mbus.h"
#include "mbus-serial.h"
#include "../usart1.h"

#define PACKET_BUFF_SIZE 2048





//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_serial_send_frame(mbus_frame *frame)
{
    uint8_t buff[PACKET_BUFF_SIZE];
    int len;

    if (frame == NULL)
    {
        return -1;
    }

    if ((len = mbus_frame_pack(frame, buff, sizeof(buff))) == -1)
    {
        printf("mbus_frame_pack failed\n");
        return -1;
    }
    printf("Packing frame success\n");

#ifdef MBUS_SERIAL_DEBUG
    // if debug, dump in HEX form to stdout what we write to the serial port
    printf("Dumping M-Bus frame [%d bytes]: ", len);
    for (i = 0; i < len; i++)
    {
       printf("%.2X ", buff[i]);
    }
    printf("\n");
#endif


    /*for (int i = 0; i < PACKET_BUFF_SIZE; i++) {
      printf("%x ",  buff[i]);
    }*/

    printf("Starting transmission");


    for (int i = 0; i < 5; i++) {
      usart1_tx(&buff[i], 1);
      // ^^^
    }
    printf("Done sending");

    // if ((ret = write(handle->fd, buff, len)) == len)
    //
    // {
    //     //
    //     // call the send event function, if the callback function is registered
    //     //
    //     if (_mbus_send_event)
    //             _mbus_send_event(MBUS_HANDLE_TYPE_SERIAL, buff, len);
    // }
    // else
    // {
    //     printf("Failed to write frame to socket (ret = %d: %s)\n", ret, strerror(errno));
    //     return -1;
    // }

    return 0;
}







//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_serial_recv_frame(mbus_frame *frame)
{
    uint8_t buff[PACKET_BUFF_SIZE];
    //int len, remaining, timeouts;

    if (frame == NULL)
    {
        printf("Invalid parameter.\n");
        return -1;
    }

    memset((void *)buff, 0, sizeof(buff));

    //
    // read data until a packet is received
    //
    // remaining = 1; // start by reading 1 byte
    // len = 0;
    // timeouts = 0;


    clock_delay_msec(400);
    int check = usart1_rx(buff, 1);
    if (check == 1) {
      printf("buf = %x\n", (uint8_t) buff[0]);
      int buff_size = 0;
      for (int i = 0; i < PACKET_BUFF_SIZE; i++) {
        if (buff[i] != 0) {
          buff_size++;
        }
      }
      mbus_parse(frame, buff, buff_size);
    }
    // ^^^


    // do {
    //     if ((nread = read(handle->fd, &buff[len], remaining)) == -1)
    //     // ^^^ change for rx read
    //     {
    //         return -1;
    //     }
    //
    //     if (nread == 0)
    //     {
    //         timeouts++;
    //
    //         if (timeouts >= 3)
    //         {
    //             // abort to avoid endless loop
    //             printf("Timeout\n");
    //             break;
    //         }
    //     }
    //
    //     len += nread;
    //
    // } while ((remaining = mbus_parse(frame, buff, len)) > 0);
    //
    // if (len == 0)
    // {
    //     // No data received
    //     return -1;
    // }

    //
    // call the receive event function, if the callback function is registered
    //
    // if (_mbus_recv_event)
    //     _mbus_recv_event(MBUS_HANDLE_TYPE_SERIAL, buff, len);
    // ^^^ might need to cut this

    // if (remaining != 0)
    // {
    //     // Would be OK when e.g. scanning the bus, otherwise it is a failure.
    //     // printf("%s: M-Bus layer failed to receive complete data.\n", __PRETTY_FUNCTION__);
    //     return -2;
    // }
    //
    // if (len == -1)
    // {
    //     printf("M-Bus layer failed to parse data.\n");
    //     return -1;
    // }

    return 0;
}
