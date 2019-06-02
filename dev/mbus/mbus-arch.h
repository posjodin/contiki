/**
 * \file
 *         Arch file for M-Bus devices
 * \authors
 *          Albert Asratyan asratyan@kth.se
 *          Mandar Joshi mandarj@kth.se
 *
 */

#ifndef MBUS_ARCH_H_
#define MBUS_ARCH_H_

/* Allow platform to override LED numbering */
//#include "contiki-conf.h"

/**
 * Leds implementation
 */
void mbus_arch_init(void);
size_t mbus_arch_read_byte(uint16_t *buf);
size_t mbus_arch_send_byte(uint8_t *buf);


#endif /* LEDS_H_ */
