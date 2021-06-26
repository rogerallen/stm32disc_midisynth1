/*
 * wavetable.h
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */

#ifndef INC_WAVETABLE_H_
#define INC_WAVETABLE_H_

#include <stdint.h>

// defines for wave table
#define WAVE_TABLE_LENGTH 256

// wave table that is used to update the audio_buffer
float sine_wave_table[WAVE_TABLE_LENGTH];

typedef struct {
  float phase;
  float phase_inc;
  int8_t pitch;
  float pitch_hz;
  float volume;
} wavetable_state_t;

void wavetable_init(wavetable_state_t *self);
void wavetable_note_on(wavetable_state_t *self, int8_t pitch, int8_t velocity);
void wavetable_note_off(wavetable_state_t *self);
void wavetable_add_samples(wavetable_state_t *self, float *in_samples, float *out_samples, int frame_count);

#endif /* INC_WAVETABLE_H_ */
