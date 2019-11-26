CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
CONTIKI_PROJECT = atca_test_contiki
all: $(CONTIKI_PROJECT)

# We use floating vars. Add library.
PRINTF_LIB_FLT  = -Wl,-u,vfprintf -lprintf_flt -lm
PRINTF_LIB_MIN  = -Wl,-u,vfprintf -lprintf_min
PRINTF_LIB      = $(PRINTF_LIB_FLT)
CLIBS           = $(PRINTF_LIB) 

MODULES += dev/atca

CUSTOM_RULE_LINK = 1

#PROJECT_OBJECTFILES += atca_command.o
PROJECT_OBJECTFILES += atca_tests_genkey.o
PROJECT_OBJECTFILES += atca_test.o
PROJECT_OBJECTFILES += test/unity.o
PROJECT_OBJECTFILES += basic/atca_basic.o
PROJECT_OBJECTFILES += basic/atca_basic_genkey.o

%.$(TARGET): %.co $(PROJECT_OBJECTFILES) $(PROJECT_LIBRARIES) contiki-$(TARGET).a 
	$(LD) $(LDFLAGS) $(TARGET_STARTFILES) ${filter-out %.a,$^} ${filter %.a,$^} $(TARGET_LIBFILES) -o $@ $(CLIBS)


CONTIKI = ../../..
include $(CONTIKI)/Makefile.include
