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

#include <stdio.h>
#include <strings.h>

#include <termios.h>
#include <errno.h>
#include <string.h>

#include <mbus/mbus.h>
#include <mbus/mbus-serial.h>

#define PACKET_BUFF_SIZE 2048

//------------------------------------------------------------------------------
/// Set up a serial connection handle.
//------------------------------------------------------------------------------
mbus_serial_handle *
mbus_serial_connect(char *device)
{
    mbus_serial_handle *handle;

    if (device == NULL)
    {
        return NULL;
    }

    if ((handle = (mbus_serial_handle *)malloc(sizeof(mbus_serial_handle))) == NULL)
    {
        printf("Failed to allocate memory for handle.\n");
        return NULL;
    }

    handle->device = device; // strdup?

    //
    // create the SERIAL connection
    //

    // Use blocking read and handle it by serial port VMIN/VTIME setting
    if ((handle->fd = open(handle->device, O_RDWR | O_NOCTTY)) < 0)
    {
        printf("Failed to open tty.");
        return NULL;
    }

    memset(&(handle->t), 0, sizeof(handle->t));
    handle->t.c_cflag |= (CS8|CREAD|CLOCAL);
    handle->t.c_cflag |= PARENB;

    // No received data still OK
    handle->t.c_cc[VMIN]  = 0;

    // Wait at most 0.2 sec.Note that it starts after first received byte!!
    // I.e. if CMIN>0 and there are no data we would still wait forever...
    //
    // The specification mentions link layer response timeout this way:
    // The time structure of various link layer communication types is described in EN60870-5-1. The answer time
    // between the end of a master send telegram and the beginning of the response telegram of the slave shall be
    // between 11 bit times and (330 bit times + 50ms).
    //
    // For 2400Bd this means (330 + 11) / 2400 + 0.05 = 188.75 ms (added 11 bit periods to receive first byte).
    // I.e. timeout of 0.2s seems appropriate for 2400Bd.

    handle->t.c_cc[VTIME] = 2; // Timeout in 1/10 sec

    cfsetispeed(&(handle->t), B2400);
    cfsetospeed(&(handle->t), B2400);

#ifdef MBUS_SERIAL_DEBUG
    printf("%s: t.c_cflag = %x\n", __PRETTY_FUNCTION__, handle->t.c_cflag);
    printf("%s: t.c_oflag = %x\n", __PRETTY_FUNCTION__, handle->t.c_oflag);
    printf("%s: t.c_iflag = %x\n", __PRETTY_FUNCTION__, handle->t.c_iflag);
    printf("%s: t.c_lflag = %x\n", __PRETTY_FUNCTION__, handle->t.c_lflag);
#endif

    tcsetattr(handle->fd, TCSANOW, &(handle->t));

    return handle;
}



//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_serial_disconnect(mbus_serial_handle *handle)
{
    if (handle == NULL)
    {
        return -1;
    }

    close(handle->fd);
    // ^^^ remove this

    free(handle);

    return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_serial_send_frame_contiki(mbus_serial_ *handle, mbus_frame *frame)
{

}

int
mbus_serial_send_frame(mbus_serial_handle *handle, mbus_frame *frame)
{
    uint8_t buff[PACKET_BUFF_SIZE];
    int len, ret;

    if (handle == NULL || frame == NULL)
    {
        return -1;
    }

    if ((len = mbus_frame_pack(frame, buff, sizeof(buff))) == -1)
    {
        printf("mbus_frame_pack failed\n");
        return -1;
    }

#ifdef MBUS_SERIAL_DEBUG
    // if debug, dump in HEX form to stdout what we write to the serial port
    printf("Dumping M-Bus frame [%d bytes]: ", len);
    for (i = 0; i < len; i++)
    {
       printf("%.2X ", buff[i]);
    }
    printf("\n");
#endif

    if ((ret = write(handle->fd, buff, len)) == len)
    // ^^^ change this for tx write
    {
        //
        // call the send event function, if the callback function is registered
        //
        if (_mbus_send_event)
                _mbus_send_event(MBUS_HANDLE_TYPE_SERIAL, buff, len);
    }
    else
    {
        printf("Failed to write frame to socket (ret = %d: %s)\n", ret, strerror(errno));
        return -1;
    }

    //
    // wait until complete frame has been transmitted
    //
    tcdrain(handle->fd);
    // ^^^ remove this thing

    return 0;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
int
mbus_serial_recv_frame(mbus_serial_handle *handle, mbus_frame *frame)
{
    char buff[PACKET_BUFF_SIZE];
    int len, remaining, nread, timeouts;

    if (handle == NULL || frame == NULL)
    {
        printf("Invalid parameter.\n");
        return -1;
    }

    memset((void *)buff, 0, sizeof(buff));

    //
    // read data until a packet is received
    //
    remaining = 1; // start by reading 1 byte
    len = 0;
    timeouts = 0;

    do {
        if ((nread = read(handle->fd, &buff[len], remaining)) == -1)
        // ^^^ change for rx read
        {
            return -1;
        }

        if (nread == 0)
        {
            timeouts++;

            if (timeouts >= 3)
            {
                // abort to avoid endless loop
                printf("Timeout\n");
                break;
            }
        }

        len += nread;

    } while ((remaining = mbus_parse(frame, buff, len)) > 0);

    if (len == 0)
    {
        // No data received
        return -1;
    }

    //
    // call the receive event function, if the callback function is registered
    //
    if (_mbus_recv_event)
        _mbus_recv_event(MBUS_HANDLE_TYPE_SERIAL, buff, len);
    // ^^^ might need to cut this

    if (remaining != 0)
    {
        // Would be OK when e.g. scanning the bus, otherwise it is a failure.
        // printf("%s: M-Bus layer failed to receive complete data.\n", __PRETTY_FUNCTION__);
        return -2;
    }

    if (len == -1)
    {
        printf("M-Bus layer failed to parse data.\n");
        return -1;
    }

    return 0;
}
