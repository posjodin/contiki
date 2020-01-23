Hello-xdar	
==========

WIP.

Simple LIDAR/RADAR demo for RSS2 mote.
Example uses for usart1.

Current support TF-mini with continues running.
external trigger is WIP but has some code for
setup and trigger. Reads need timeout.

Seems like the HW needs power-cycle despite the
RESET.

Example:

Dist=   27 Signal= 2612 Temp=31 ext_trigger=0
Dist=   27 Signal= 2611 Temp=31 ext_trigger=0
Dist=   27 Signal= 2619 Temp=31 ext_trigger=0
Dist=   27 Signal= 2616 Temp=31 ext_trigger=0
Dist=   27 Signal= 2615 Temp=31 ext_trigger=0

Build
-----
make TARGET=avr-rss2

Connecting to 2:nd USART (avr-rss2)
-----------------------------------
Most convenient to use the second USART and to use the first USART
for flashing and monitoring. 

http://radio-sensors.com/pictures/TFmini-rss2-connect.jpg

LIDAR needs stable 5V. This provided via the USB-TTL cable
by connecting the RED wire from the USB-TTL cable to the
white DC socket. 5V/GND is then connected to according to
the picture.

To enable second USART (USART1) a modification is needed.
http://radio-sensors.com/docs/S2/HOWTO/HOWTO.USART1

RX (green) and TX (blue) as seen in the pict.
See project_conf.h for USART SW config.