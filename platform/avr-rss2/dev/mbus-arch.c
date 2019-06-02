/**
 * \file
 *         Contiki-OS M-Bus functionality for avr-rss2
 * \authors
 *          Albert Asratyan asratyan@kth.se
 *          Mandar Joshi mandarj@kth.se
 *
 */


#include "contiki.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "dev/rs232.h"
#include <sys/cc.h>
#include "mbus-arch.h"

/*---------------------------------------------------------------------------*/

#define BUFSIZE 256
struct ringbuf16 rxbuf;
uint16_t rxbuf_data[BUFSIZE];
uint8_t usart = RS232_PORT_1;


/*---------------------------------------------------------------------------*/
void
ringbuf16_init(struct ringbuf16 *r, uint16_t *dataptr, uint16_t size)
{
  r->data = dataptr;
  r->mask = size - 1;
  r->put_ptr = 0;
  r->get_ptr = 0;
}
/*---------------------------------------------------------------------------*/
int
ringbuf16_put(struct ringbuf16 *r, uint8_t c)
{
  /* Check if buffer is full. If it is full, return 0 to indicate that
     the element was not inserted into the buffer.
     XXX: there is a potential risk for a race condition here, because
     the ->get_ptr field may be written concurrently by the
     ringbuf16_get() function. To avoid this, access to ->get_ptr must
     be atomic. We use an uint8_t type, which makes access atomic on
     most platforms, but C does not guarantee this.
  */
  if(((r->put_ptr - r->get_ptr) & r->mask) == r->mask) {
    return 0;
  }
  /*
   * CC_ACCESS_NOW is used because the compiler is allowed to reorder
   * the access to non-volatile variables.
   * In this case a reader might read from the moved index/ptr before
   * its value (c) is written. Reordering makes little sense, but
   * better safe than sorry.
   */
  CC_ACCESS_NOW(uint8_t, r->data[r->put_ptr]) = c;
  CC_ACCESS_NOW(uint8_t, r->put_ptr) = (r->put_ptr + 1) & r->mask;
  return 1;
}
/*---------------------------------------------------------------------------*/
int
ringbuf16_get(struct ringbuf16 *r)
{
  uint8_t c;
  /* Check if there are bytes in the buffer. If so, we return the
     first one and increase the pointer. If there are no bytes left, we
     return -1.
     XXX: there is a potential risk for a race condition here, because
     the ->put_ptr field may be written concurrently by the
     ringbuf16_put() function. To avoid this, access to ->get_ptr must
     be atomic. We use an uint8_t type, which makes access atomic on
     most platforms, but C does not guarantee this.
  */
  if(((r->put_ptr - r->get_ptr) & r->mask) > 0) {
    /*
     * CC_ACCESS_NOW is used because the compiler is allowed to reorder
     * the access to non-volatile variables.
     * In this case the memory might be freed and overwritten by
     * increasing get_ptr before the value was copied to c.
     * Opposed to the put-operation this would even make sense,
     * because the register used for mask can be reused to save c
     * (on some architectures).
     */
    c = CC_ACCESS_NOW(uint8_t, r->data[r->get_ptr]);
    CC_ACCESS_NOW(uint8_t, r->get_ptr) = (r->get_ptr + 1) & r->mask;
    return c;
  } else {
    return -1;
  }
}
/*---------------------------------------------------------------------------*/
int
ringbuf16_size(struct ringbuf16 *r)
{
  return r->mask + 1;
}
/*---------------------------------------------------------------------------*/
int
ringbuf16_elements(struct ringbuf16 *r)
{
  return (r->put_ptr - r->get_ptr) & r->mask;
}
/*---------------------------------------------------------------------------*/


int
usart_input_byte(unsigned char c)
{
  ringbuf16_put(&rxbuf, c);
  return 1;
}


/*---------------------------------------------------------------------------*/
void
mbus_arch_init() {
  ringbuf16_init(&rxbuf, rxbuf_data, sizeof(rxbuf_data));
  rs232_set_input(usart, usart_input_byte);
}

/*---------------------------------------------------------------------------*/
size_t
mbus_arch_read_byte(uint16_t *buf) {
  int c;

  c = ringbuf16_get(&rxbuf);

  if (c == -1)
    return 0;
  *buf = (uint8_t) c;
  return 1;
}
/*---------------------------------------------------------------------------*/
size_t
mbus_arch_send_byte(uint8_t *buf) {
  uint8_t p = *buf;
  rs232_send(usart, p);
  return 1;
}
/*---------------------------------------------------------------------------*/
