/*
 * synthutil.h
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */

#ifndef INC_SYNTHUTIL_H_
#define INC_SYNTHUTIL_H_

#include <stdint.h>

#define SAMPLE_RATE 48000

// set up a small chromatic scale to use
typedef enum { C4 = 60, Cs4, D4, Ds4, E4, F4, Fs4, G4, Gs4, A4, As4, B4 } pitch_t;

float pitch_to_freq(uint8_t pitch);
uint16_t float2uint16(float f);

#endif /* INC_SYNTHUTIL_H_ */
