/*
 * synthutil.h
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */

#ifndef INC_SYNTHUTIL_H_
#define INC_SYNTHUTIL_H_

#include <stdint.h>

// defines for wave table
#define WAVE_TABLE_LENGTH 256

// defines for audio buffer sizing
#define AUDIO_BUFFER_FRAMES   256
#define AUDIO_BUFFER_CHANNELS 2
#define AUDIO_BUFFER_SAMPLES  AUDIO_BUFFER_FRAMES * AUDIO_BUFFER_CHANNELS
#define AUDIO_BUFFER_BYTES    sizeof(int16_t)*AUDIO_BUFFER_SAMPLES

#define HARDWARE_VOLUME 86
#define SAMPLE_RATE    48000

// polyphony (FIXME dynamic alloc of synths)
#define MAX_POLYPHONY 10

// set up a small chromatic scale to use
typedef enum { C4 = 60, Cs4, D4, Ds4, E4, F4, Fs4, G4, Gs4, A4, As4, B4 } note_t;  // FIXME pitch

float note_to_freq(uint8_t note); // FIXME pitch
uint16_t float2uint16(float f);


#endif /* INC_SYNTHUTIL_H_ */
