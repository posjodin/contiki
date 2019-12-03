/*
 * Copyright (c) 2015, Copyright Robert Olsson
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
 * Author  : Robert Olsson rolss@kth.se/robert@radio-sensors.com
 * Created : 2019-07-01
 */

/*
   This is software support for Alphasense NO2-B43F sensors. The
   code is used and tested with ISB intrerface board. The could
   be usable for other Alphasense gas sensors.

   The implementation is a common app and platform specific code
   providing reading functions the Worker and Auxiliary electrodes.
   The platform code also provides the no2_temp needed for temperature
   compensation and the calibration constants.

 */

#include "contiki.h"
#include <string.h>
#include <stdio.h>
#include "no2.h"

static int debug = 1;

double
ppb2ugm3(double ppb, double mw, double t)
{
  return (ppb * 12.187 * mw) / (273.15 + t);
}
/* Appnote AAN 803-05 multiplies by 10 */
const int8_t NO2_B43F_T[9] = { -30, -20, -10, 0, 10, 20, 30, 40, 50 };  /* Temp */
const int8_t NO2_B43F_1[9] = { 13, 13, 13, 13, 10, 6, 4, 2, -15 };
const int8_t NO2_B43F_3[9] = { 10, 10, 10, 10, 10, 10, 4, -1, -40 };

enum no2_calibration_data {sid, algo, wz, az, sens, we_ez, aux_ez };

double
temp_compensation(double tg, int8_t *k)
{
  int i;
  int8_t *t;
  double c1, c2, c3, c4;

  t = (int8_t *)&NO2_B43F_T;

  if(tg <= t[0]) {
    return k[0];
  }

  if(tg >= t[8]) {
    return k[8];
  }

  for(i = 0; i < 8; i++) {
    if(tg >= t[i] && tg < t[i + 1]) {
      break;
    }
  }

  c1 = tg - t[i];       /* Temp diff */
  c2 = k[i + 1] - k[i];  /* Total interval */
  c2 = c2 / 10;          /* Interval diff */
  c3 = c1 * c2;        /* Interpol factor */
  c4 = (k[i] + c3) / 10; /* Calc and downscale final result */

  if(debug) {
    printf("NO2: T=%-5.2f %d Interpol: %-5.2f %-5.2f %-5.2f Final=%-5.3f\n",
           tg, k[i], c1, c2, c3, c4);
  }
  return c4;
}
double
no2_ppb_a1(double we_u, double we_ez, double ae_u, double ae_ez, double sens, double t)
{
  /* Formula 1 from appnote AAN 803-05 */
  double tc = temp_compensation(t, (int8_t *)&NO2_B43F_1);
  return ((we_u - we_ez) - tc * (ae_u - ae_ez)) / (sens / 1000);
}
double
no2_ppb_a3(double we_u, double we_ez, double ae_u, double ae_ez, double we_0, double ae_0, double sens, double t)
{
  /* Formula 3 from appnote AAN 803-05 */
  double tc = temp_compensation(t, (int8_t *)&NO2_B43F_3);
  return ((we_u - we_ez) - (we_0 - ae_0) - tc * (ae_u - ae_ez)) / (sens / 1000);
}
double
no2(const int16_t no2_calibration[])
{
  double a1, a3, we_u, ae_u, we_0, ae_0, tg;
  int16_t *t;

  we_u = ae_u = we_0 = ae_0 = 0;

  /* Align AD resolution according to noise level in gas sensors
     to minimize variation */

  we_u = read_we_u();
  ae_u = read_ae_u();
  clock_delay_usec(1000);
  we_u = read_we_u();
  ae_u = read_ae_u();
  t = (int16_t *)no2_calibration;

  we_0 = t[wz] - t[we_ez];
  ae_0 = t[az] - t[aux_ez];

  tg = no2_temp();

  a3 = no2_ppb_a3(we_u * 1000, t[we_ez], ae_u * 1000, t[aux_ez], we_0, ae_0, t[sens], tg);

  if(debug) {
    a1 = no2_ppb_a1(we_u * 1000, t[we_ez], ae_u * 1000, t[aux_ez], t[sens], tg);
    printf("NO2: we_u=%-6.5f ae_u=%-6.5f a1=%-5.0f a3=%-5.0f\n", we_u, ae_u, a1, a3);
  }
  return a3;
}
