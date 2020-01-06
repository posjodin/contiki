DEFINES+=PROJECT_CONF_H=\"project-conf.h\"
CONTIKI=../../../..

all: mqtt-publisher

MQTT=false
MQTT=true

CONTIKI_WITH_IPV6 = 1
#APPS += ping6
#APPS += cli

# We use floating vars. Add library.
PRINTF_LIB_FLT  = -Wl,-u,vfprintf -lprintf_flt -lm
PRINTF_LIB_MIN  = -Wl,-u,vfprintf -lprintf_min
PRINTF_LIB      = $(PRINTF_LIB_FLT)
CLIBS           = $(PRINTF_LIB) 


# Define to use radio module for cellular data
RADIO_MODULE=sim7020
#RADIO_MODULE=a6

# Set to true for sockets with TCP/IPv4 stack on radio module
RADIO_SOCKETS=false
RADIO_SOCKETS=true

# Set to true for IPv6 tunnel over TCP/IPv4 (experimental)
RADIO_TUNNEL=false

#
RADIO_6TO4=false

# Set to true for 6to4 with IPv4/PPP/cellular
RADIO_PPP=false
#RADIO_PPP=true

# Set to true to enable power save mode on radio module
RADIO_PSM_ENABLE=false
#RADIO_PSM_ENABLE=true

# Set to true for RPL root with uplink
RPL_BORDER_ROUTER=false
#RPL_BORDER_ROUTER=true

ifeq ($(MQTT),true)
CFLAGS += -DMQTT_CLIENT -DMQTT_CLI
PROJECT_OBJECTFILES += mqtt-rpl-pub.o mqtt-i2c-pub.o
PROJECT_OBJECTFILES += mqtt-client.o mqtt-cli.o
APPS += mqtt
APPS += cli
endif # MQTT

ifdef RADIO_MODULE
CFLAGS+= -DAT_RADIO

PROJECT_OBJECTFILES +=  at-wait.o at-radio.o
MODULES += dev/at-radio

CFLAGS += -DAT_RADIO_CONF_STATS=1

ifeq ($(RADIO_SOCKETS),true)
PROJECT_OBJECTFILES +=  tcp-socket-at-radio.o
# Use tcp-socket.h from dev/at-radio/ instead of core/net/ip/
CFLAGS += -I$(CONTIKI)/dev/at-radio
CFLAGS += -DAT_RADIO_SOCKETS
endif # RADIO_SOCKETS

ifeq ($(RADIO_TUNNEL),true)
CFLAGS += -DAT_TCPTUN6
CFLAGS += -DUIP_FALLBACK_INTERFACE=attun6_interface 

endif # RADIO_TUNNEL

ifeq ($(RADIO_6TO4),true)
CFLAGS += -DAT_TCPTUN6
CFLAGS += -DUIP_FALLBACK_INTERFACE=at64_interface 
apa MODULES += core/net/ip64/
endif # RADIO_TUNNEL

ifeq ($(RADIO_PPP),true)
CFLAGS += -DAT_PPP
CFLAGS += -DUIP_FALLBACK_INTERFACE=ip64_ppp_interface 
MODULES += core/net/ip64/
MODULES += dev/ppp
CFLAGS += -DDEBUG=DEBUG_PRINT
endif # RADIO_PPP

ifeq ($(RADIO_MODULE),sim7020)
PROJECT_OBJECTFILES +=  nbiot-sim7020.o nbiot-sim7020-arch.o
MODULES += dev/nbiot-sim7020
CFLAGS += -DNBIOT
ifeq ($(RADIO_PSM_ENABLE),true)
CFLAGS += -DSIM7020_PSM_ENABLE
endif #RADIO_PSM_ENABLE

else ifeq ($(RADIO_MODULE),a6)
PROJECT_OBJECTFILES +=  gprs-a6.o gprs-a6-arch.o sc16is-common.o 
MODULES += dev/gprs-a6

endif # RADIO_MODULE

else
CFLAGS += -DAT_RADIO_CONF_STATS=0
endif # RADIO_MODULE

ifeq ($(MQTT),true)
ifeq ($(RADIO_MODULE),true)
CFLAGS += -DMQTT_AT_RADIO
endif # RADIO_MODULE
CFLAGS += -DMQTT_CLIENT
MODULES += apps/mqtt
ifeq ($(RADIO_PSM_ENABLE),true)
# Disconnect TCP after each publish
CFLAGS += -DMQTT_PUBLISH_DISCONNECT
# QOS = 1 to receive PUBACK confirmation 
CFLAGS += -DMQTT_QOS_LEVEL=MQTT_QOS_LEVEL_1
endif #RADIO_PSM_ENABLE

else
PROJECT_OBJECTFILES += tcpdummy.o
endif

ifeq ($(RPL_BORDER_ROUTER),true)
CFLAGS += -DRPL_BORDER_ROUTER
endif # RPL_BORDER_ROUTER

#PROJECT_OBJECTFILES += mqtt-rpl-pub.o mqtt-i2c-pub.o mqtt-cli.o
PROJECT_OBJECTFILES += eestats.o
CONTIKI=../../../..

#CFLAGS += -DPMS5003
#CFLAGS += -DBME280
MODULES += dev/bme280
MODULES += dev/pms5003
MODULES += dev/sc16is
MODULES += dev/sht2x

CUSTOM_RULE_LINK = 1
%.$(TARGET): %.co $(PROJECT_OBJECTFILES) $(PROJECT_LIBRARIES) contiki-$(TARGET).a buildstring.h
	$(LD) $(LDFLAGS) $(TARGET_STARTFILES) ${filter-out %.a,$^} ${filter %.a,$^} $(TARGET_LIBFILES) -o $@ $(CLIBS)

DATESTR:=${shell date "+%y-%m-%d% %H:%M"}
BRANCHSTR:=${shell git symbolic-ref --short HEAD}
buildstring.h: ALWAYS
	echo \#define BUILDSTRING \"$(TARGET)/$(BRANCHSTR)/$(RELSTR)/$(DATESTR)\" >$@
.PHONY: ALWAYS
ALWAYS:

include $(CONTIKI)/Makefile.include
