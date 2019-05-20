
/*
1. create a mbus_frame
2. set baud address
3. try to write to the usart1




*/


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

  int short2 = 0;
  int short3 = 0;
  int short4 = short2 + short3;
  uint8_t frame[5] = {MBUS_FRAME_SHORT_START, short2, short3, short4, MBUS_FRAME_STOP}

  for (i = 0; i < 5; i++) {
    usart1_tx(&frame[i], 1);      //rewrite tx so it takes only 1 argument
  }

}

int
mbus_scan_all(int baudrate, int debug)
{
  for (int i = 0; i <= 250; i++)
  {
    mbus_scan(i, baudrate, debug);
  }
}
