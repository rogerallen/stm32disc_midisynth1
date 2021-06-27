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

float comb_filter(float *buf, int *idx, float gain, int lim, float v);
float allpass_filter(float *buf, int *idx, float gain, int lim, float v);

// ======================================================================
void reverb_init(reverb_state_t *self, float wet, float delay)
{
  self->wet = wet;
  self->delay = delay;
  self->comb0_lim = delay/MAX_DELAY * COMB0_LEN;
  self->comb1_lim = delay/MAX_DELAY * COMB1_LEN;
  self->comb2_lim = delay/MAX_DELAY * COMB2_LEN;
  self->comb3_lim = delay/MAX_DELAY * COMB3_LEN;
  self->allp0_lim = delay/MAX_DELAY * ALLP0_LEN;
  self->allp1_lim = delay/MAX_DELAY * ALLP1_LEN;
  self->allp2_lim = delay/MAX_DELAY * ALLP2_LEN;
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
    float newsample = comb_filter(&(self->comb0[0]), &(self->comb0_idx), self->comb0_gain, self->comb0_lim, sample);
    newsample += comb_filter(&(self->comb1[0]), &(self->comb1_idx), self->comb1_gain, self->comb1_lim, sample);
    newsample += comb_filter(&(self->comb2[0]), &(self->comb2_idx), self->comb2_gain, self->comb2_lim, sample);
    newsample += comb_filter(&(self->comb3[0]), &(self->comb3_idx), self->comb3_gain, self->comb3_lim, sample);
    newsample /= 4;
    newsample = allpass_filter(&(self->allp0[0]), &(self->allp0_idx), self->allp0_gain, self->allp0_lim, sample);
    newsample = allpass_filter(&(self->allp1[0]), &(self->allp1_idx), self->allp1_gain, self->allp1_lim, sample);
    newsample = allpass_filter(&(self->allp2[0]), &(self->allp2_idx), self->allp2_gain, self->allp2_lim, sample);
    newsample = (1.0-self->wet)*sample + self->wet*newsample;
    float newsample1 = (1.0-self->wet)*in_samples[2*frame+1] + self->wet*newsample;
    out_samples[2*frame] = newsample;
    out_samples[2*frame+1] = newsample1;
  }
}

inline float comb_filter(float *buf, int *idx, float gain, int lim, float v)
{
  float new_v = buf[*idx]*gain + v;
  buf[*idx] = new_v;
  *idx += 1;
  if (*idx == lim) {
    *idx = 0;
  }
  return new_v;
}

inline float allpass_filter(float *buf, int *idx, float gain, int lim, float v)
{
  float feedback = buf[*idx] + (-gain) * v;
  float new_v = feedback*gain + v;
  buf[*idx] = new_v;
  *idx += 1;
  if (*idx == lim) {
    *idx = 0;
  }
  return new_v;
}
