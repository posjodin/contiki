/*
 * Copyright (c) 2017, Peter Sjodin, KTH Royal Institute of Technology
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 *
 * Author  : Peter Sjodin, KTH Royal Institute of Technology
 * Created : 2017-04-21
 */

#ifndef PM2105_H
#define PM2105_H

/* How often sensor process runs (sec) -- defines resolution
 * of warmup time and sample period
 */
#ifdef PM2105_CONF_PROCESS_PERIOD
#define PMS_PROCESS_PERIOD  PM2105_CONF_PROCESS_PERIOD
#else
#define PMS_PROCESS_PERIOD  5
#endif /* PM2105_CONF_PROCESS_PERIOD */

/* Default sample period - how often sensor data is collected (sec) */
#ifdef PM2105_CONF_SAMPLE_PERIOD
#define PMS_SAMPLE_PERIOD       PM2105_CONF_SAMPLE_PERIOD
#else
#define PMS_SAMPLE_PERIOD       60
#endif /* PM2105_CONF_SAMPLE_PERIOD */

/* Default warmup time before sensor data can be read (sec) */
#ifdef PM2105_CONF_WARMUP_INTERVAL
#define PMS_WARMUP_INTERVAL    PM2105_CONF_WARMUP_INTERVAL
#else
#define PMS_WARMUP_INTERVAL    30
#endif /* PM2105_WARMUP_INTERVAL */

/* Use I2C interface? */
#ifdef PM2105_CONF_SERIAL_I2C
#define PMS_SERIAL_I2C          PM2105_CONF_SERIAL_I2C
#else
#define PMS_SERIAL_I2C          1
#endif /* PMS_CONF_SERIAL_I2C */

/* Use UART interface? */
#ifdef PM2105_CONF_SERIAL_UART
#define PMS_SERIAL_UART         PM2105_CONF_SERIAL_UART
#else
#define PMS_SERIAL_UART         1
#endif /* PMS_CONF_SERIAL_UART */

#if PMS_SERIAL_UART
/* What buffer size to use */
#ifdef PM2105_CONF_UART_BUFSIZE
#define PMS_BUFSIZE             PM2105_CONF_UART_BUFSIZE
#else /* PM2105_CONF_UART_BUFSIZE */
#define PMS_BUFSIZE             128
#endif /* PM2105_CONF_UART_BUFSIZE */

/* What UART port to use */
#ifdef PM2105_CONF_UART_PORT
#define PMS_UART_PORT           PM2105_CONF_UART_PORT
#else
#define PMS_UART_PORT           RS232_PORT_0
#endif /* PMS_CONF_UART_RS232_PORT */
#endif /* PMS_SERIAL_UART */

/* Event to signal presence of new sensor data */
process_event_t pm2105_event;

void pm2105_init();
void pm2105_off();

uint16_t pm2105_pm1();
uint16_t pm2105_pm2_5();
uint16_t pm2105_pm10();
uint16_t pm2105_pm1_tsi();
uint16_t pm2105_pm2_5_tsi();
uint16_t pm2105_pm10_tsi();
uint16_t pm2105_db0_3();
uint16_t pm2105_db0_5();
uint16_t pm2105_db1();
uint16_t pm2105_db2_5();
uint16_t pm2105_db5();
uint16_t pm2105_db10();
uint8_t pm2105_status();
uint16_t pm2105_mode();
uint8_t pm2105_calibration();
uint32_t pm2105_timestamp();
void pm2105_config_sample_period(unsigned int);
void pm2105_config_warmup_interval(unsigned int);
unsigned pm2105_get_sample_period(void);
unsigned pm2105_get_warmup_interval(void);

#endif /* PM2105_H */
