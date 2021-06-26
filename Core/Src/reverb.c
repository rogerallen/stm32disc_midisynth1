/*
 * reverb.c
 *
 *  Created on: Jun 26, 2021
 *      Author: rallen
 *
 *  based on https://github.com/YetAnotherElectronicsChannel/STM32_DSP_Reverb/blob/master/code/Src/main.c
 *  and https://www.youtube.com/watch?v=nRLXNmLmHqM
 */

#include "reverb.h"
#include "string.h"

float do_comb0(reverb_state_t *self, float v);
float do_comb1(reverb_state_t *self, float v);
float do_comb2(reverb_state_t *self, float v);
float do_comb3(reverb_state_t *self, float v);
float do_allp0(reverb_state_t *self, float v);
float do_allp1(reverb_state_t *self, float v);
float do_allp2(reverb_state_t *self, float v);

// ======================================================================
void reverb_init(reverb_state_t *self, float wet, float delay)
{
  self->wet = wet;
  self->delay = delay;
  self->comb0_lim = delay/2.0 * COMB0_LEN;
  self->comb1_lim = delay/2.0 * COMB1_LEN;
  self->comb2_lim = delay/2.0 * COMB2_LEN;
  self->comb3_lim = delay/2.0 * COMB3_LEN;
  self->allp0_lim = delay/2.0 * ALLP0_LEN;
  self->allp1_lim = delay/2.0 * ALLP1_LEN;
  self->allp2_lim = delay/2.0 * ALLP2_LEN;
  memset(&(self->comb0[0]), 0, sizeof(float)*COMB0_LEN);
  memset(&(self->comb1[0]), 0, sizeof(float)*COMB1_LEN);
  memset(&(self->comb2[0]), 0, sizeof(float)*COMB2_LEN);
  memset(&(self->comb3[0]), 0, sizeof(float)*COMB3_LEN);
  memset(&(self->allp0[0]), 0, sizeof(float)*ALLP0_LEN);
  memset(&(self->allp1[0]), 0, sizeof(float)*ALLP1_LEN);
  memset(&(self->allp2[0]), 0, sizeof(float)*ALLP2_LEN);
  self->comb0_gain = 0.805;
  self->comb1_gain = 0.827;
  self->comb2_gain = 0.783;
  self->comb3_gain = 0.764;
  self->allp0_gain = 0.7;
  self->allp1_gain = 0.7;
  self->allp2_gain = 0.7;
  self->comb0_idx = 0;
  self->comb1_idx = 0;
  self->comb2_idx = 0;
  self->comb3_idx = 0;
  self->allp0_idx = 0;
  self->allp1_idx = 0;
  self->allp2_idx = 0;
}

// ======================================================================
void reverb_get_samples(reverb_state_t *self, float *in_samples, float *out_samples, int frame_count)
{
  for(int frame = 0; frame < frame_count; frame++) {
    float sample = in_samples[2*frame]; // just left sample for reverb input
    float newsample = (do_comb0(self, sample) + do_comb1(self, sample) + do_comb2(self, sample) + do_comb3(self, sample))/4.0f;
    newsample = do_allp0(self, newsample);
    newsample = do_allp1(self, newsample);
    newsample = do_allp2(self, newsample);
    newsample = (1.0-self->wet)*sample + self->wet*newsample;
    float newsample1 = (1.0-self->wet)*in_samples[2*frame+1] + self->wet*newsample;
    out_samples[2*frame] = newsample;
    out_samples[2*frame+1] = newsample1;
  }
}

inline float do_comb0(reverb_state_t *self, float v)
{
  float new_v = self->comb0[self->comb0_idx]*(self->comb0_gain) + v;
  self->comb0[self->comb0_idx] = new_v;
  self->comb0_idx += 1;
  if (self->comb0_idx == self->comb0_lim) {
    self->comb0_idx = 0;
  }
  return new_v;
}
inline float do_comb1(reverb_state_t *self, float v)
{
  float new_v = self->comb1[self->comb1_idx]*(self->comb1_gain) + v;
  self->comb1[self->comb1_idx] = new_v;
  self->comb1_idx += 1;
  if (self->comb1_idx == self->comb1_lim) {
    self->comb1_idx = 0;
  }
  return new_v;
}
inline float do_comb2(reverb_state_t *self, float v)
{
  float new_v = self->comb2[self->comb2_idx]*(self->comb2_gain) + v;
  self->comb2[self->comb2_idx] = new_v;
  self->comb2_idx += 1;
  if (self->comb2_idx == self->comb2_lim) {
    self->comb2_idx = 0;
  }
  return new_v;
}
inline float do_comb3(reverb_state_t *self, float v)
{
  float new_v = self->comb3[self->comb3_idx]*(self->comb3_gain) + v;
  self->comb3[self->comb3_idx] = new_v;
  self->comb3_idx += 1;
  if (self->comb3_idx == self->comb3_lim) {
    self->comb3_idx = 0;
  }
  return new_v;
}
inline float do_allp0(reverb_state_t *self, float v)
{
  float feedback = self->allp0[self->allp0_idx] + (-self->allp0_gain) * v;
  float new_v = feedback*(self->allp0_gain) + v;
  self->allp0[self->allp0_idx] = new_v;
  self->allp0_idx += 1;
  if (self->allp0_idx == self->allp0_lim) {
    self->allp0_idx = 0;
  }
  return new_v;
}
inline float do_allp1(reverb_state_t *self, float v)
{
  float feedback = self->allp1[self->allp1_idx] + (-self->allp1_gain) * v;
  float new_v = feedback*(self->allp1_gain) + v;
  self->allp1[self->allp1_idx] = new_v;
  self->allp1_idx += 1;
  if (self->allp1_idx == self->allp1_lim) {
    self->allp1_idx = 0;
  }
  return new_v;
}
inline float do_allp2(reverb_state_t *self, float v)
{
  float feedback = self->allp2[self->allp2_idx] + (-self->allp2_gain) * v;
  float new_v = feedback*(self->allp2_gain) + v;
  self->allp2[self->allp2_idx] = new_v;
  self->allp2_idx += 1;
  if (self->allp2_idx == self->allp2_lim) {
    self->allp2_idx = 0;
  }
  return new_v;
}
