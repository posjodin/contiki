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

Exampel:

Dist=   27 Signal= 2612 Temp=31 ext_trigger=0
Dist=   27 Signal= 2611 Temp=31 ext_trigger=0
Dist=   27 Signal= 2619 Temp=31 ext_trigger=0
Dist=   27 Signal= 2616 Temp=31 ext_trigger=0
Dist=   27 Signal= 2615 Temp=31 ext_trigger=0
Dist=   27 Signal= 2615 Temp=31 ext_trigger=0
Dist=   27 Signal= 2611 Temp=31 ext_trigger=0
Dist=   27 Signal= 2619 Temp=31 ext_trigger=0
Dist=   27 Signal= 2616 Temp=31 ext_trigger=0
Dist=   27 Signal= 2612 Temp=31 ext_trigger=0


Build
-----
make TARGET=avr-rss2


