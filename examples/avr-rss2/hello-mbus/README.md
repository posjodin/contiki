Hello-mbus
==========

WIP.

Simple mbus demo from the RSS2 mote.
Example uses for usart1 at 300 BUAD

Test slave with pattern
-----------------------
Waveform test. Voltage at least 25V. Power supply pullup 470 OHM
for both VCC and GND.

buf[0] = 0b10101010;
usart1_tx(buf, 1);
  
clock_delay_usec(100);

buf[0] = 0b01010101;
usart1_tx(buf, 1);

Result captured on a oscilloscope
---------------------------------
mbus-1.png mbus-slave-test-1.png


Build
-----
make TARGET=avr-rss2


