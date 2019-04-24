DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI=../../../..

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

# Use a radio module for mobile data?
#RADIO_MODULE=sim7020
RADIO_MODULE=a6

# Use TCP/IP stack on radio module? 
RADIO_SOCKETS=true

ifdef RADIO_MODULE

PROJECT_OBJECTFILES +=  at-wait.o at-radio.o
MODULES += dev/at-radio
CFLAGS += -DMQTT_AT_RADIO
CFLAGS += -DAT_RADIO_CONF_STATS=1

ifeq ($(RADIO_SOCKETS),true)
PROJECT_OBJECTFILES +=  tcp-socket-at-radio.o
# Use tcp-socket.h from dev/at-radio/ instead of core/net/ip/
CFLAGS += -I$(CONTIKI)/dev/at-radio
CFLAGS += -DAT_RADIO_SOCKETS
endif # RADIO_SOCKETS

ifeq ($(RADIO_MODULE),sim7020)

PROJECT_OBJECTFILES +=  nbiot-sim7020.o nbiot-sim7020-arch.o
MODULES += dev/nbiot-sim7020
CFLAGS += -DNBIOT

else ifeq ($(RADIO_MODULE),a6)

PROJECT_OBJECTFILES +=  gprs-a6.o gprs-a6-arch.o sc16is-common.o 
MODULES += dev/gprs-a6

endif

else
CFLAGS += -DAT_RADIO_CONF_STATS=0
endif # RADIO_MODULE


PROJECT_OBJECTFILES += mqtt-rpl-pub.o mqtt-i2c-pub.o mqtt-cli.o
CONTIKI=../../../..

#CFLAGS += -DPMS5003
#CFLAGS += -DBME280
MODULES += dev/bme280
MODULES += dev/pms5003
MODULES += dev/sc16is

CUSTOM_RULE_LINK = 1
%.$(TARGET): %.co $(PROJECT_OBJECTFILES) $(PROJECT_LIBRARIES) contiki-$(TARGET).a 
	$(LD) $(LDFLAGS) $(TARGET_STARTFILES) ${filter-out %.a,$^} ${filter %.a,$^} $(TARGET_LIBFILES) -o $@ $(CLIBS)

include $(CONTIKI)/Makefile.include
