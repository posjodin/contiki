CONTIKI_PROJECT = coap-publisher

all: $(CONTIKI_PROJECT)

PRINTF_LIB_FLT  = -Wl,-u,vfprintf -lprintf_flt -lm
PRINTF_LIB_MIN  = -Wl,-u,vfprintf -lprintf_min
PRINTF_LIB      = $(PRINTF_LIB_FLT)
CLIBS           = $(PRINTF_LIB) 

PROJECT_OBJECTFILES += no2-arch.o

# Adds printf float support to read the temperature sensor value.
CUSTOM_RULE_LINK = 1
%.$(TARGET): %.co $(PROJECT_OBJECTFILES) $(PROJECT_LIBRARIES) contiki-$(TARGET).a 
	$(LD) $(LDFLAGS) $(TARGET_STARTFILES) ${filter-out %.a,$^} ${filter %.a,$^} $(TARGET_LIBFILES) -o $@ $(CLIBS)

CFLAGS += -ffunction-sections
LDFLAGS += -Wl,--gc-sections,--undefined=_reset_vector__,--undefined=InterruptVectors,--undefined=_copy_data_init__,--undefined=_clear_bss_init__,--undefined=_end_of_init__

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

APPS += er-coap
APPS += rest-engine
APPS += coap-pubsub
APPS += no2
APPS += mcu_sleep

MODULES += dev/bme280
MODULES += dev/bme680
MODULES += dev/pms5003
MODULES += dev/pm2105
MODULES += dev/sht25


#CFLAGS += -DUIP_CONF_BUFFER_SIZE=1300
CONTIKI_WITH_IPV6 = 1
CONTIKI=../../../..
include $(CONTIKI)/Makefile.include

