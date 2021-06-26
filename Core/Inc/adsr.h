/*
 * adsr.h
 *
 *  Created on: Jun 25, 2021
 *      Author: rallen
 */

#ifndef INC_ADSR_H_
#define INC_ADSR_H_

#include <stdint.h>

typedef struct {
  float attack;
  float decay;
  float sustain;
  float release;
  float scale;
  float max_amplitude;
  float start_time;
  float release_time;
  float cur_amplitude;
  float release_amplitude;
} adsr_state_t;

void adsr_init(adsr_state_t *self, float attack, float decay, float sustain, float release, float scale);
void adsr_reset(adsr_state_t *self);
void adsr_note_on(adsr_state_t *self, int8_t velocity, float time);
void adsr_note_off(adsr_state_t *self, float time);
void adsr_get_samples(adsr_state_t *self, float *in_samples, float *out_samples, int frame_count, float time);
int8_t adsr_active(adsr_state_t *self, float time);

#endif /* INC_ADSR_H_ */
