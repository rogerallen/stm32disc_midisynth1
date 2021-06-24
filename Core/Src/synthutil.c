/*
 * synthutil.c
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */

#include "synthutil.h"
#include <math.h>

// ======================================================================
// private defines
float CHROMATIC_BASE = pow(2.0f, 1.0f / 12.0f);

// ======================================================================
// convert midi pitch index to frequency in Hz.  A4 440Hz = midi note 69
float pitch_to_freq(uint8_t pitch)
{
  return 440*pow(CHROMATIC_BASE, (float)pitch - 69);
}

// ======================================================================
// float to uint16 produces two's complement values with -1.0 => 0x8001,
// 0.0 => 0x0000 and 1.0 => 0x7fff.  It also has good rounding behavior
// near 0.0 per https://www.cs.cmu.edu/~rbd/papers/cmj-float-to-int.html
inline uint16_t float2uint16(float f)
{
    return (uint16_t)(((int16_t)(32767*f + 32768.5)) - 32768);
}

