DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
all: mqtt-at-radio

CONTIKI_WITH_IPV6 = 1

APPS += mqtt
APPS += ping6
APPS += cli

# We use floating vars. Add library.
PRINTF_LIB_FLT  = -Wl,-u,vfprintf -lprintf_flt -lm
PRINTF_LIB_MIN  = -Wl,-u,vfprintf -lprintf_min
PRINTF_LIB      = $(PRINTF_LIB_FLT)
CLIBS           = $(PRINTF_LIB) 

#RADIO_OBJECTFILES +=  gprs-a6.o gprs-a6-arch.o sc16is-common.o 
RADIO_OBJECTFILES +=  nbiot-sim7020.o
RADIO_OBJECTFILES += nbiot-sim7020-arch.o
PROJECT_OBJECTFILES += $(RADIO_OBJECTFILES)

PROJECT_OBJECTFILES += mqtt-rpl-pub.o mqtt-i2c-pub.o mqtt-cli.o
PROJECT_OBJECTFILES +=  tcp-socket-at-radio.o
PROJECT_OBJECTFILES +=  at-wait.o at-radio.o

CONTIKI=../../../..

#CFLAGS += -DPMS5003
#CFLAGS += -DBME280
CFLAGS += -DMQTT_AT_RADIO
CFLAGS += -DAT_RADIO_SOCKETS
CFLAGS += -I$(CONTIKI)/dev/at-radio
MODULES += dev/bme280
MODULES += dev/pms5003
MODULES += dev/sc16is
MODULES += dev/at-radio
#MODULES += dev/gprs-a6
MODULES += dev/nbiot-sim7020

#at_wait.o: at-wait.h project-conf.h Makefile
#gprs-a6.o sc16is-common.o: at-wait.h at-radio.h project-conf.h Makefile
#nbiot-sim7020.o: at-wait.h at-radio.h project-conf.h Makefile

CUSTOM_RULE_LINK = 1
%.$(TARGET): %.co $(PROJECT_OBJECTFILES) $(PROJECT_LIBRARIES) contiki-$(TARGET).a 
	$(LD) $(LDFLAGS) $(TARGET_STARTFILES) ${filter-out %.a,$^} ${filter %.a,$^} $(TARGET_LIBFILES) -o $@ $(CLIBS)

CONTIKI=../../../..
include $(CONTIKI)/Makefile.include
