Getting Started with Contiki using avr-rss2
===========================================

Document version
----------------
V1.1 2019-04-24

This guide's aim is to help you start using Contiki for RSS2 boards
The platform supports different AtMega-RF boards:

* Based on MCU AtMega256RFR2 (Current. Rev2.3 and Rev2.4)
* Based on MCU AtMega128RFR2
* Based on MCU AtMega128RFA1 

Boards all looks the same. AtMega256RFR2 boards are labeled with "Made in EU".
The platform and CPU code, are common for both boards, can be found under 
`$(CONTIKI)/platfrom/avr-rssa`. The port was developed and tested with RSS2.
This guide assumes that you have basic understanding of how to use the 
command line and perform basic admin tasks on UNIX family OSs. You should
also have understanding about the Contiki OS design as well have C 
programming skills.

Board Features
---------------
* Chip Antenna. Supercardiod.
* Robust radio performance. Up to 300 meter line-of-sight.
* Unique EUI64 address via chip combined with EEPROM
* Rev 2.3 Onboard temp sensor DS18B20,
* Rev 2.4 Onboard BME280 Temp/RH/Pressure
* Light Sensor.
* 32kHz RTC clock xtal
* Comparator chip and input. 
* Progammable power FET, (relay) for power on/off of sensors.
* DC input via LDO up to 25V.
* Standard. 6-pin TTL-USB header for FTDI cable for USART.
* PCB formfactor for cheap project box G.40X IP54
* Power/current use:
  * RX ~10mA (Full RPC AtMegaXXRFR2). 
  * Sleep ~45uA @ 16MHz XTAL
  * Sleep ~15uA @  8MHz using internal oscillator
* Preprogammed Atmel standard bootloader. 
* CE certified by test institute.

USART 
-----
The board has one USART via the 6-pin TTL-USB adapter, The default
baudrate is 38400 bps. This speed gives the lowest error with respect 
of the used clock frequency used internally. It's possible to use higher
speeds as is 250k and 500k baud which gives 0% Error with 16MHz clock. 
(An addtional USART is on the chip but as default used by interrupt pins)

Contiki Port Features
---------------------
The platform has the following key features:
* Standard, E64 address from built-in chip. Chip also has a 128bit ID.
* Support for Atmel RPC (Reduced Power Consumption) for AtMegaXXXRFR2. 
* Stable port since many years.

Toolchain
---------
The Atmel toolcahin is available in most operating systems. 

For a full toolchain and easy installation on Ubuntu:

apt-get install gcc-avr avr-libc avrdude

Toolchain alternative I
-----------------------
Otherwise if OS packages does not yet support the new AtMega256RfR2 MCU. 
An option is to use toolchain from Atmels web-site.

###### For Linux 

1. See http://www.atmel.com/tools/ATMELAVRTOOLCHAINFORLINUX.aspx
2. Download the proper 8-bit platform 32 or 64 bit.
3. Unpack under `/usr/local`
4. Add to your search PATH. For example add to `.bashrc`: `export PATH=$PATH:/usr/local/avr8-gnu-toolchain-linux_x86_64/bin` (for 64 bit systems) or `export PATH=$PATH:/usr/local/avr8-gnu-toolchain-linux_x86/bin` (for 32 birt systems).
5. For flash programming you'll need `avrdude`. It can be installed with the command
`apt-get install avrdude`

Toolchain alternative II
------------------------
You might also consider Instant Contiki.


Toolchain Windows
-----------------
Goes here


Contiki platform
-----------------
avr-rss2


Contiki build TARGET
--------------------
    make TARGET=avr-rss2

For older AtMega128RFR2 boards:

make TARGET=avr-rss2 MCU=atmega128rfr2

For older AtMega128RFA1 boards:

make TARGET=avr-rss2 MCU=atmega128rfa1


Flashing/Programming hardware
------------------------------
Programming using avrdude using serial bootloader. (TTL-USB cable)
Press the RESET button. The bootloader with wait for boot commands 
for 3 seconds.

Flashing commnad line example 256k MCU:
    avrdude -p m256rfr2 -c stk500v2 -P /dev/ttyUSB0 -b 115200 -e -U flash:w:hello-world.avr-rss2 

Note! Older bootloader needs 38400 (Yellow LED

Flashing older 128k MCU:
    avrdude -p m128rfa1 -c avr109 -P /dev/ttyUSB0 -b 38400 -e -U flash:w:hello-world.avr-rss2 


Example of tested applications
------------------------------
* `examples/hello-world`
* `examples/rime`
* `examples/ipv6/multicast`
* `examples/example-shell`
* `examples/powertrace`
* 'examples/sensniff'
* `examples/ipv6/rpl-udp`
* `examples/ipv6/simple-udp-rpl`
* 'examples/ipv6/rpl-tsch"

* 'examples/avr-rss2/timer-test'
* 'examples/avr-rss2/hello-sensors'
* 'examples/avr-rss2/tsch'
* 'examples/avr-rss2/ipv6/tsch-udp'
* 'examples/avr-rss2/ipv6/rpl-border-router'
* 'examples/avr-rss2/ipv6/rpl-udp-report'
* 'examples/avr-rss2/ipv6/coap-publisher'


Note that the shell example needs file `symbols.c` to be added to project also seems like
in `core/dev/serial-line.c` the function `process_poll` must be replaced with `process_post`:

     /* Wake up consumer process */
    -  process_poll(&serial_line_process);
    +  process_post(&serial_line_process, 0, 0);

Platform tutorial applications
-----------------------------
Example to read out various sensors, leds, serial numbers, and so on:
[platform/avr-rss2/examples/hello-sensors/](examples/hello-sensors/).

The previous example uses the `sensd` data encoding. An example that uses UDP through 6lowpan is here:
[platform/avr-rss2/examples/ipv6/rpl-udp-report](examples/ipv6/rpl-udp-report).

An Ethernet gateway with ip64 NAT, based on Contiki `core/net/ip64` code:
[platform/avr-rss2/examples/ipv6/rpl-border-router/](examples/ipv6/rpl-border-router/).

Wireshark in-air realtime protocol decoding. examples/sensniff with sensniff
python script creating a named pipe as wireshark input. Be aware of 
restrictions in compression format.

Contiki Regressions tests
--------------------------
Travis compile regression test for the platform: 
[regression-tests/23-compile-avr](../../regression-tests/23-compile-avr).

Board approvals
---------------
 Summary: CE approved Radio Equipment Directive (RED) 2014/53/EU

Rev 2.4
* Safety: IEC 60950-1:2005 2nd Edition +Am 1:2009 +Am 2:2013
* RF: ETSI EN 300 328 V2.1.1 (2016-11)
* EMC: Draft ETSI EN 301 489-1 V2.2.0 (2017-03), 
  Draft ETSI EN 301 489-17 V3.2.0 (2017-03)
* EMF: EN 62479:2010
* Human exposure to electromagnetic fields: EN 62479:2010 

Rev 2.3
* R&TTE 73/23/EEC, 89/336/EEC and 99/5/EC
* Safety: EN 60950-1:2006 + A12: 2011
* RF: ETSI EN 300 328 V1.7.1 (2006-10)
* EMC: ETSI EN 301 489-1 V1.9.2 (2011-09), ETSI EN 301 489-17 V2.2.1 (2012-09)
* EMF: EN 62479:2010
* Human exposure to electromagnetic fields: EN 62479:2010 

Commercial availability
------------------------
Through vendor and though resellers. Note board is will only available 
were CE approval is covered. This may be further restricted by WEEE.
Contact vendor. For research legislation is more relaxed in most 
countries.

References
----------
AtMega64/128/256/RFR2 chip documentation available via Atmel.
Schematics and boards description. Available via Radio-Senors
Smart Reduced Power Consumption Techniques. AT02594 available via Atmel.

Board (Rev2.4) anatomy with connectors:
http://radio-sensors.com/pictures/s2-2.4-front-edited.jpg
http://radio-sensors.com/pictures/s2-2.4-back-port-edited.jpg

Vendor info
-----------
http://radio-sensors.com/

Maintainer
----------
Robert Olsson <robert@radio-sensors.com>
