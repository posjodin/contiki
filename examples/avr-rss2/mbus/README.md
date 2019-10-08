# Contiki M-Bus

M-Bus library for contiki, adapted for platform avr-rss2.

## Getting Started

### Prerequisites

Will need to install the following software before proceeding:
avrdude command line tool to flash the sensor node
minicom to see the output from the sensor node


```
apt-get install gcc-avr avr-libc avrdude
apt-get install minicom

```

### Installing

After 'make', press the RESET button, and run the second command.

```
make TARGET=avr-rss2

Flashing commnad line example 256k MCU: avrdude -p m256rfr2 -c stk500v2 -P /dev/ttyUSB0 -b 115200 -e -U flash:w:hello-mbus.avr-rss2
```

Tested at 9600 baud, can be changed in the conf file

```
#define USART1_CONF_BAUD_RATE   USART_BAUD_9600
```

Examples: Received long frame

## Deployment

Needs M-Bus Master board to be connected to the sensor node and the water meter. Schematic for a generic M-Bus master can be found on [libmbus](https://github.com/rscada/libmbus/blob/master/hardware/MBus_USB.pdf)

## Authors

* **Albert Asratyan** - [Goradux](https://github.com/goradux)
* **Mandar Joshi** - [mandaryoshi](https://github.com/mandaryoshi)
