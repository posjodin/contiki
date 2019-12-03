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
#include "apps/no2/no2.h"

/*
  ISB cable
  OP1 = Orange-Green = WE
  OP2 = Yellow - Green = AE 
*/

double
read_ae_u(void)
{
  return 0; /* Add AE reading */
}

double
read_we_u(void)
{
  return  0; /* Add WE reading */
}

double
no2_temp(void)
{
  return 22; /* Test only */
}

enum no2_calibration_data {sid, algo, wz, az, sens, we_ez, aux_ez };

/* NO2-B43F sensor calibration data */
const int16_t no2_sen_2[8] = {02, 3, 222, 210, 268, 229, 214 };
const int16_t no2_sen_3[8] = {03, 3, 228, 233, 269, 237, 238 };
const int16_t no2_sen_4[8] = {04, 3, 232, 218, 303, 237, 222 };
