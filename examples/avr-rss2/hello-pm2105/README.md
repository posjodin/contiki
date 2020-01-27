
Connecting to I2C on avr-rss2
-----------------------------
5V Power is taken from USB-UART cable with a small
arrangement as seen in picture.
http://radio-sensors.com/pictures/pm2105-connect.jpg

Program output examples
------------------------

PM is read every 30 sec: PM1.0 PM2.5 PM10. GRIMM and TSI.
Plus dustbins:  0.3 0.5 1.0 2.5 5.0 10.0

Output
-----

PM status=2 mode=3, cal=100
PM 9/0 GRIM    3    3    3 TSI    3    4    4 DB  155   55   13    0    0    0
pm: 3 3 3 tsi: 3 4 4 db: 155 55 13 0 0 0
pm: 3 3 3 tsi: 3 4 4 db: 155 55 13 0 0 0
.

Trick: Test response by adding air pollution by squeezing paper tissues

PM status=2 mode=3, cal=100
PM 10/0 GRIM  219  631 1000 TSI  386  429  471 DB 2199 1511 1017   78   27   22
pm: 219 631 1000 tsi: 386 429 471 db: 2199 1511 1017 78 27 22
pm: 219 631 1000 tsi: 386 429 471 db: 2199 1511 1017 78 27 22
.

PM status=2 mode=3, cal=100
PM 11/0 GRIM    2    5   11 TSI    1    2    2 DB  142   46    8    0    0    0
pm: 2 5 11 tsi: 1 2 2 db: 142 46 8 0 0 0
pm: 2 5 11 tsi: 1 2 2 db: 142 46 8 0 0 0
.
