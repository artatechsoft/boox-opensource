/*
 *   PCM time-domain equalizer
 *
 *   Copyright (C) 2002-2005  Felipe Rivera <liebremx at users sourceforge net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   $Id: iir.c,v 1.15 2005/10/17 01:57:59 liebremx Exp $
 */

#include <math.h>
#include "iir.h"

/* Coefficients */
sIIRCoefficients *iir_cf;

/* Volume gain 
 * values should be between 0.0 and 1.0 
 * Use the preamp from XMMS for now
 * */
float preamp[EQ_CHANNELS];

#ifdef BENCHMARK
#include "benchmark.h"
double timex = 0.0;
int count = 0;
unsigned int blength = 0;
#endif

/* 
 * Global vars
 */
int rate;
int band_count;

void set_preamp(int chn, float val)
{
  preamp[chn] = val;
}

/* Init the filters */
void init_iir()
{
  calc_coeffs();
#if 0
  band_count = cfg.band_num;
#endif

  band_count = 10;

  rate = 44100;

  iir_cf = get_coeffs(&band_count, rate);
  clean_history();
}

#ifdef ARCH_X86
/* Round function provided by Frank Klemm which saves around 100K
 * CPU cycles in my PIII for each call to the IIR function with 4K samples
 */
__inline__ int round_trick(float floatvalue_to_round)
{
  float   floattmp ;
  int     rounded_value ;

  floattmp      = (int) 0x00FD8000L + (floatvalue_to_round);
  rounded_value = *(int*)(&floattmp) - (int)0x4B7D8000L;

  if ( rounded_value != (short) rounded_value )
    rounded_value = ( rounded_value >> 31 ) ^ 0x7FFF;
  return rounded_value;
}
#endif
