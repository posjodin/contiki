



int
construct_frame(mbus_frame frame)
{
  switch(frame->type) {
    case MBUS_FRAME_TYPE_ACK:
      return 0xE5;

    case MBUS_FRAME_TYPE_SHORT:


    case MBUS_FRAME_TYPE_LONG:

  }

}

int
mbus_scan(int address, int baudrate, int debug)
{
  uint8_t frame[5] = {0x10, 0x40, 0x43, 0x83, 0x16};
  uint8_t frame[5] = {MBUS_FRAME_SHORT_START, , , , }
}
