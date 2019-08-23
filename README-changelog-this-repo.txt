
New/updates
-----------
bme280 bug fixes
new bme680 as bme280 with indoor AQ 
new mcp342x ADC 18bit 1-4 ports
new pms5003 AQ sensor
wip ppp to be used with the AT-radio framework
new sc16is NXP I2C-to-UART-GPIO
new sht2x Sensiron sensors T/RH

MQTT is bug fixed and hardened
new CoAP publish subscribe -- CoAP broker according to IETF 
    Also a companion broker from students in C++
    Rudimentary Android app for CoAP pubsub

NO2 support for Alphasense gas sensors

new sound meter support sen0232 in avr-rss2/dev/sen0232-gslm.c
    but should be generic

wip generic interface for AT-command based radios including
    NB-IoT and GPRS. Contiki interleaves with the radio module
    stack. ppp and raw IP is planned. Peter has the details.
    Work in collaboration with 3GPP experts from KTH@wireless

wip Generic framwork for MCU sleep regions to enable MCU deep sleep
    but avoid deep sleep during critical operations as I2C
    communication etc.

new Numeric of patches for Atmel radio/platforms.

PR  waiting for wired mbus and Kamstrup water sensors.

PR  waiting CBORE coding project.
