/*
 * reverb.h
 *
 *  Created on: Jun 26, 2021
 *      Author: rallen
 *
 * based on https://github.com/YetAnotherElectronicsChannel/STM32_DSP_Reverb/blob/master/code/Src/main.c
 * and https://www.youtube.com/watch?v=nRLXNmLmHqM
 *
 * Block diagram / Algorithm
 * 4 parallel Comb Filters:
 *   Gain: 0.805, Delay: 36.04ms
 *   Gain: 0.827, Delay: 31.12
 *   Gain: 0.783, Delay: 40.44
 *   Gain: 0.764, Delay: 44.92
 * Sum & Mul by 0.25
 * AllPass Gain: 0.7, Delay 5ms
 * AllPass Gain: 0.7, Delay 1.68
 * AllPass Gain: 0.7, Delay 0.48
 * this is "wet" output
 * lerp between "wet" and input
 *
 * FRAME_RATE = 48000
 * comb0: 36.040ms -> 1730 frames
 * comb1: 31.120ms -> 1494 frames
 * comb2: 40.440ms -> 1941 frames
 * comb3: 44.920ms -> 2156 frames
 * allp0:  5.000ms -> 240 frames
 * allp1:  1.680ms -> 81 frames
 * allp2:  0.480ms -> 23 frames
 * total frame count = 7665 or 30660 bytes
 */

#ifndef INC_REVERB_H_
#define INC_REVERB_H_

// Allow for delay scale < 2
// Note: Can't do stereo.  Hits memory limits.
#define MAX_DELAY 2
#define COMB0_LEN 1730*MAX_DELAY
#define COMB1_LEN 1494*MAX_DELAY
#define COMB2_LEN 1941*MAX_DELAY
#define COMB3_LEN 2156*MAX_DELAY
#define ALLP0_LEN 240*MAX_DELAY
#define ALLP1_LEN 81*MAX_DELAY
#define ALLP2_LEN 23*MAX_DELAY

typedef struct {
  float wet;
  float delay; // delay scale factor
  int comb0_lim, comb1_lim, comb2_lim, comb3_lim;
  int allp0_lim, allp1_lim, allp2_lim;
  float comb0[COMB0_LEN], comb1[COMB1_LEN], comb2[COMB2_LEN], comb3[COMB3_LEN];
  float allp0[ALLP0_LEN], allp1[ALLP1_LEN], allp2[ALLP2_LEN];
  float comb0_gain, comb1_gain, comb2_gain, comb3_gain;
  float allp0_gain, allp1_gain, allp2_gain;
  int comb0_idx, comb1_idx, comb2_idx, comb3_idx;
  int allp0_idx, allp1_idx, allp2_idx;
} reverb_state_t;

void reverb_init(reverb_state_t *self, float wet, float delay);
void reverb_get_samples(reverb_state_t *self, float *in_samples, float *out_samples, int frame_count);

#endif /* INC_REVERB_H_ */
