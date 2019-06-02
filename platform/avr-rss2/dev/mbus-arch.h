/**
 * \file
 *         Contiki-OS M-Bus functionality for platform avr-rss2
 * \authors
 *          Albert Asratyan asratyan@kth.se
 *          Mandar Joshi mandarj@kth.se
 *
 */

void mbus_arch_init(void);
int usart_input_byte(unsigned char c);
size_t mbus_arch_read_byte(uint16_t *buf);
size_t mbus_arch_send_byte(uint8_t *buf);

struct ringbuf16 {
  uint16_t *data;
  uint16_t mask;

  /* XXX these must be 8-bit quantities to avoid race conditions. */
  uint8_t put_ptr, get_ptr;
};

void    ringbuf16_init(struct ringbuf16 *r, uint16_t *a,
		     uint16_t size_power_of_two);

int     ringbuf16_put(struct ringbuf16 *r, uint8_t c);

int     ringbuf16_get(struct ringbuf16 *r);

int     ringbuf16_size(struct ringbuf16 *r);

int     ringbuf16_elements(struct ringbuf16 *r);
