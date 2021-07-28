/*
 * wavetable.c
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */

#include "wavetable.h"
#include "synthutil.h"
#include <math.h>
#include <string.h>

// ======================================================================
// private defines

// ======================================================================
// private vars
static uint8_t wavetables_initialized = 0;

// ======================================================================
// private prototypes & functions
void wavetable_sine_init(void);
void wavetable_saw_init(void);

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

void wavetable_saw_init(void)
{
  memset(saw_wave_table, 0, sizeof(float) * WAVE_TABLE_LENGTH);
  float num_octaves = (int)(FRAME_RATE / 2.0 / 440.0);
  for (int octave = 1; octave < num_octaves; octave++) {
    float phase_inc = (octave * 2.0 * (float)M_PI) / (float)WAVE_TABLE_LENGTH;
    float phase = 0;
    float sign = (octave & 1) ? -1.0f : 1.0f;
    for (int i = 0; i < WAVE_TABLE_LENGTH; i++) {
      saw_wave_table[i] += (sign * sin(phase) / octave) * (2.0f / (float)M_PI);
      phase += phase_inc;
    }
  }
}

// ======================================================================
// public functions


// ======================================================================
void wavetable_init(wavetable_state_t *self, uint8_t wave)
{
  if(!wavetables_initialized) {
    wavetable_sine_init();
    wavetable_saw_init();
    wavetables_initialized = 1;
  }
  self->wave      = wave;
  self->phase     = 0;
  self->pitch     = 0;
  self->pitch_hz  = pitch_to_freq(A4);
  self->phase_inc = (self->pitch_hz / FRAME_RATE) * WAVE_TABLE_LENGTH;
}

// ======================================================================
void wavetable_note_on(wavetable_state_t *self, int8_t pitch, int8_t velocity)
{
  self->phase     = 0;
  self->pitch     = pitch;
  self->pitch_hz  = pitch_to_freq(pitch);
  self->phase_inc = (self->pitch_hz / FRAME_RATE) * WAVE_TABLE_LENGTH;
}

// ======================================================================
void wavetable_note_off(wavetable_state_t *self)
{
  self->phase     = 0;
  self->pitch     = 0;
  self->pitch_hz  = pitch_to_freq(A4);
  self->phase_inc = (self->pitch_hz / FRAME_RATE) * WAVE_TABLE_LENGTH;
}

// ======================================================================
void wavetable_get_samples(wavetable_state_t *self, float *out_samples, int frame_count)
{
  for(int frame = 0; frame < frame_count; frame++) {
    float sample_f;
    switch(self->wave) {
    case 0:
      sample_f = sine_wave_table[(uint32_t)self->phase];
      break;
    case 1:
    default:
      sample_f = saw_wave_table[(uint32_t)self->phase];
      break;
    }
    out_samples[2*frame]   = sample_f;
    out_samples[2*frame+1] = sample_f;
    self->phase += self->phase_inc;
    if(self->phase > WAVE_TABLE_LENGTH) {
      self->phase -= WAVE_TABLE_LENGTH;
    }
  }
}
