CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = hello-mbus
all: $(CONTIKI_PROJECT)

# We use floating vars. Add library.
PRINTF_LIB_FLT  = -Wl,-u,vfprintf -lprintf_flt -lm
PRINTF_LIB_MIN  = -Wl,-u,vfprintf -lprintf_min
PRINTF_LIB      = $(PRINTF_LIB_FLT)
CLIBS           = $(PRINTF_LIB)

MODULES += dev/bme280

CUSTOM_RULE_LINK = 1

PROJECT_OBJECTFILES += usart1.o mbus_library/mbus.o mbus_library/mbus-protocol.o mbus_library/mbus-protocol-aux.o mbus_library/mbus-serial.o mbus_library/functions/mbus-serial-scan.o mbus_library/functions/mbus-serial-request-data.o

%.$(TARGET): %.co $(PROJECT_OBJECTFILES) $(PROJECT_LIBRARIES) contiki-$(TARGET).a
	$(LD) $(LDFLAGS) $(TARGET_STARTFILES) ${filter-out %.a,$^} ${filter %.a,$^} $(TARGET_LIBFILES) -o $@ $(CLIBS)



CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
