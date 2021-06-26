/*
 * wavetable.c
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */

#include "wavetable.h"
#include "synthutil.h"
#include <math.h>

// ======================================================================
// private defines

// ======================================================================
// private vars
static uint8_t sine_initialized = 0;

// ======================================================================
// private prototypes & functions
void wavetable_sine_init(void);

// ======================================================================
// create a wave_table with a single sine wave cycle.
// values are stored as float to make further math easy.
void wavetable_sine_init(void)
{
  float phase_inc = (2.0f * (float)M_PI) / (float)WAVE_TABLE_LENGTH;
  float phase = 0;
  for (int i = 0; i < WAVE_TABLE_LENGTH; i++) {
    sine_wave_table[i] = sin(phase);
    phase += phase_inc;
  }
}

// ======================================================================
// public functions


// ======================================================================
void wavetable_init(wavetable_state_t *self)
{
  if(!sine_initialized) {
    wavetable_sine_init();
    sine_initialized = 1;
  }
  self->phase     = 0;
  self->pitch     = 0;
  self->pitch_hz  = pitch_to_freq(A4);
  self->phase_inc = (self->pitch_hz / SAMPLE_RATE) * WAVE_TABLE_LENGTH;
}

// ======================================================================
void wavetable_note_on(wavetable_state_t *self, int8_t pitch, int8_t velocity)
{
  self->phase     = 0;
  self->pitch     = pitch;
  self->pitch_hz  = pitch_to_freq(pitch);
  self->phase_inc = (self->pitch_hz / SAMPLE_RATE) * WAVE_TABLE_LENGTH;
}

// ======================================================================
void wavetable_note_off(wavetable_state_t *self)
{
  self->phase     = 0;
  self->pitch     = 0;
  self->pitch_hz  = pitch_to_freq(A4);
  self->phase_inc = (self->pitch_hz / SAMPLE_RATE) * WAVE_TABLE_LENGTH;
}

// ======================================================================
void wavetable_get_samples(wavetable_state_t *self, float *in_samples, float *out_samples, int frame_count)
{
  for(int frame = 0; frame < frame_count; frame++) {
    float sample_f = sine_wave_table[(uint32_t)self->phase];
    out_samples[2*frame]   = in_samples[2*frame] + sample_f;
    out_samples[2*frame+1] = in_samples[2*frame+1] + sample_f;
    self->phase += self->phase_inc;
    if(self->phase > WAVE_TABLE_LENGTH) {
      self->phase -= WAVE_TABLE_LENGTH;
    }
  }
}
