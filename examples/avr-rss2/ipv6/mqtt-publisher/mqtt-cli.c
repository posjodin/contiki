#include <stdio.h>
#include <stdarg.h>

#define BUFSIZE 512
static char mqttbuf[BUFSIZE];
static int remaining;

void
mc_handle_serial_input(const char *line);

char *
mqtt_cli_input(const char *line) {
  remaining = BUFSIZE;
  mqttbuf[0] = 0;
  mc_handle_serial_input(line);
  return mqttbuf;
}

static void
mcprintf(const char *fmt, ...) {
  va_list args;
  int len;
	
  va_start(args, fmt);
  len = vsnprintf(&mqttbuf[BUFSIZE-remaining], remaining, fmt,  args); 
  va_end(args);
  if (len >= remaining) {
    mqttbuf[BUFSIZE-1] = 0;
    remaining = 0;
    printf("buf full\n");
  }
  else {
    remaining -= len; /* Don't count terminating null */
  }
}

/* Copied from "core/net/ip/uip-debug.c", replaced PRINTA with mcprintf */
#include "net/ip/ip64-addr.h"
static void
uip_debug_ipaddr_print(const uip_ipaddr_t *addr)
{
#if NETSTACK_CONF_WITH_IPV6
  uint16_t a;
  unsigned int i;
  int f;
#endif /* NETSTACK_CONF_WITH_IPV6 */
  if(addr == NULL) {
    mcprintf("(NULL IP addr)");
    return;
  }
#if NETSTACK_CONF_WITH_IPV6
  if(ip64_addr_is_ipv4_mapped_addr(addr)) {
    /*
     * Printing IPv4-mapped addresses is done according to RFC 4291 [1]
     *
     *     "An alternative form that is sometimes more
     *     convenient when dealing with a mixed environment
     *     of IPv4 and IPv6 nodes is x:x:x:x:x:x:d.d.d.d,
     *     where the 'x's are the hexadecimal values of the
     *     six high-order 16-bit pieces of the address, and
     *     the 'd's are the decimal values of the four
     *     low-order 8-bit pieces of the address (standard
     *     IPv4 representation)."
     *
     * [1] https://tools.ietf.org/html/rfc4291#page-4
     */
    mcprintf("::FFFF:%u.%u.%u.%u", addr->u8[12], addr->u8[13], addr->u8[14], addr->u8[15]);
  } else {
    for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
      a = (addr->u8[i] << 8) + addr->u8[i + 1];
      if(a == 0 && f >= 0) {
        if(f++ == 0) {
          mcprintf("::");
        }
      } else {
        if(f > 0) {
          f = -1;
        } else if(i > 0) {
          mcprintf(":");
        }
        mcprintf("%x", a);
      }
	}
  }
#else /* NETSTACK_CONF_WITH_IPV6 */
  mcprintf("%u.%u.%u.%u", addr->u8[0], addr->u8[1], addr->u8[2], addr->u8[3]);
#endif /* NETSTACK_CONF_WITH_IPV6 */
}
/*---------------------------------------------------------------------------*/


#define handle_serial_input mc_handle_serial_input 
#define printf(...) mcprintf(__VA_ARGS__)
#include "apps/cli/cli.c"
