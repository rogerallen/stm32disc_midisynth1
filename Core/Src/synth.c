/*
 * synth.c
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */
//
// Control Rate interface:
//  [midi note on/off]
//     V           V
//  [Envelope i] [Wavetable i]
//
// Audio Rate interface:
//  [ audio samples required ] X
//        VV                   X
//  [ Mix i polyphony          X
//          VV                 X
//    [ Wavetable i ]          X
//          VV                 X
//    [ Envelope i ]           X
//          VV                 X
//    Mix ]                    X
//     VV                      _
//  [ Pan     ]                _
//     VV                      _
//  [ Compressor ]             _
//     VV                      _
//  [ Res Filter ]             X
//     VV                      X
//  [  Reverb  ]               _
//     VV                      _
//  [  Output  ]               X
//
#include "synth.h"
#include "synthutil.h"
#include "wavetable.h"
#include "adsr.h"
#include "biquad.h"
#include "reverb.h"
#include "main.h"
#include "../../Drivers/BSP/STM32F4-Discovery/stm32f4_discovery_audio.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ======================================================================
// private defines

// defines for audio buffer sizing
#define AUDIO_BUFFER_FRAMES   256
#define AUDIO_BUFFER_CHANNELS 2
#define AUDIO_BUFFER_SAMPLES  AUDIO_BUFFER_FRAMES * AUDIO_BUFFER_CHANNELS
#define AUDIO_BUFFER_BYTES    sizeof(int16_t)*AUDIO_BUFFER_SAMPLES

#define HARDWARE_VOLUME 86

// polyphony
// *WARNING* use 2 voices max for Debug builds (3 is right on the hairy edge)
// release build can easily do 10
#define MAX_POLYPHONY 10

// ======================================================================
// private vars

float synth_time = 0;

// audio buffer that is sent over I2S to DAC
uint16_t audio_buffer[AUDIO_BUFFER_SAMPLES];

// synthesis blocks
wavetable_state_t  wavetables[MAX_POLYPHONY];
adsr_state_t       envelopes[MAX_POLYPHONY];
sf_biquad_state_st rlpf;
reverb_state_t     *reverb;

// ======================================================================
// private function prototypes

void init_audio_buffer(void);
void update_audio_buffer(uint32_t start_frame, uint32_t num_frames);

// ======================================================================
// user code

// ======================================================================
void synth_init(void)
{
  float attack = 0.4;
  float decay = 0.1;
  float sustain = 0.8;
  float release = 0.5;
  for(int i=0; i < MAX_POLYPHONY; i++) {
    wavetable_init( &(wavetables[i]) );
    adsr_init( &(envelopes[i]), attack, decay, sustain, release, 0.3);
  }

  float cutoff = 600.0;
  float resonance = 5.0;
  sf_lowpass(&rlpf, FRAME_RATE, cutoff, resonance);

  float wet = 0.75;
  float delay = 1.50;
  reverb = (reverb_state_t *)malloc(sizeof(reverb_state_t));
  if(reverb == 0) {
    Error_Handler();
  }
  reverb_init(reverb, wet, delay);

  update_audio_buffer(0, AUDIO_BUFFER_FRAMES);

  if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, HARDWARE_VOLUME, FRAME_RATE) != AUDIO_OK) {
    Error_Handler();
  }

  // tell the chip to start DMA from audio_buffer
  BSP_AUDIO_OUT_Play(&(audio_buffer[0]), AUDIO_BUFFER_BYTES);
}


// ======================================================================
void synth_all_notes_off(void)
{
  for(int i = 0; i < MAX_POLYPHONY; i++) {
    wavetable_note_off( &(wavetables[i]) );
    adsr_reset(&(envelopes[i]));
  }
}

// ======================================================================
void note_off(uint8_t midi_cmd, uint8_t midi_param0, uint8_t midi_param1)
{
  int8_t cur_idx = MAX_POLYPHONY;
  for(int8_t j = 0; j < MAX_POLYPHONY; j++) {
    if(midi_param0 == wavetables[j].pitch) {
      if(1 == adsr_releasing(&(envelopes[j]), synth_time)) {
        // release already in progress here, keep looking
        continue;
      }
      cur_idx = j;
      break;
    }
  }
  if(cur_idx < MAX_POLYPHONY) {
    printf("Note off: %d %d %d\r\n", cur_idx, midi_param0, midi_param1);
    adsr_note_off(&(envelopes[cur_idx]), synth_time);
  } else {
    printf("Note off: [NOPE] %d %d\r\n", midi_param0, midi_param1);
  }
}

// ======================================================================
void note_on(uint8_t midi_cmd, uint8_t midi_param0, uint8_t midi_param1)
{
  int8_t cur_idx = MAX_POLYPHONY;
  for(int8_t j = 0; j < MAX_POLYPHONY; j++) {
    if(0 == adsr_active(&(envelopes[j]), synth_time)) {
      // found a spot!
      cur_idx = j;
      break;
    }
  }
  if(cur_idx < MAX_POLYPHONY) {
    printf("Note on:  %d %d %d\r\n", cur_idx, midi_param0, midi_param1);
    wavetable_note_on(&(wavetables[cur_idx]), midi_param0, midi_param1);
    adsr_note_on(&(envelopes[cur_idx]), midi_param1, synth_time);
  } else {
    printf("Note on:  [NOPE] %d %d\r\n", midi_param0, midi_param1);
  }
}

// ======================================================================
// using cur_phase, read from wave_table[] and update the
// audio_buffer from start to start+num_frames
void update_audio_buffer(uint32_t start_frame, uint32_t num_frames)
{
  HAL_GPIO_WritePin(LED_Port, RED_LED, GPIO_PIN_SET);
  // temp buffers to use for float intermediate data
  static float sample_buffer[3][AUDIO_BUFFER_SAMPLES];

  memset(&(sample_buffer[0][0]), 0, sizeof(float)*AUDIO_BUFFER_SAMPLES);
  memset(&(sample_buffer[1][0]), 0, sizeof(float)*AUDIO_BUFFER_SAMPLES);
  // Osc + Env -> buf1
  for(int note = 0; note < MAX_POLYPHONY; note++) {
    wavetable_get_samples(&(wavetables[note]), &(sample_buffer[0][0]), num_frames);
    adsr_get_samples(&(envelopes[note]), &(sample_buffer[0][0]), num_frames, synth_time);
    for(int i = 0; i < num_frames; i++) {
      sample_buffer[1][2*i] += sample_buffer[0][2*i];
      sample_buffer[1][2*i+1] += sample_buffer[0][2*i+1];
    }
  }
  // RLPF buf1 -> buf2
  sf_biquad_process(&rlpf, num_frames, (sf_sample_st *)&(sample_buffer[1][0]), (sf_sample_st *)&(sample_buffer[2][0]));

  // Reverb buf2 -> buf1
  reverb_get_samples(reverb, &(sample_buffer[2][0]), &(sample_buffer[1][0]), num_frames);

  // convert buf1 -> uint16 output buffer
  int i = 0;
  int outidx = 1;
  for(int frame = start_frame; frame < start_frame+num_frames; frame++) {
    float sample0_f = sample_buffer[outidx][2*i];
    float sample1_f = sample_buffer[outidx][2*i+1];
    sample0_f = (sample0_f > 1.0) ? sample0_f = 1.0 : (sample0_f < -1.0) ? -1.0 : sample0_f;
    sample1_f = (sample1_f > 1.0) ? sample1_f = 1.0 : (sample1_f < -1.0) ? -1.0 : sample1_f;
    audio_buffer[2*frame] = float2uint16(sample0_f);
    audio_buffer[2*frame+1] = float2uint16(sample1_f);
    i++;
  }
  HAL_GPIO_WritePin(LED_Port, RED_LED, GPIO_PIN_RESET);
}

// ======================================================================
// after the first half of the audio_buffer has been transferred, fill
// that portion with new samples while the second half is being played.
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
  update_audio_buffer(0, AUDIO_BUFFER_FRAMES/2);
  synth_time += (float)(AUDIO_BUFFER_FRAMES/2)/FRAME_RATE;
}

// ======================================================================
// now the second half has been transferred. fill in the second half
// with new samples.
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
  update_audio_buffer(AUDIO_BUFFER_FRAMES/2, AUDIO_BUFFER_FRAMES/2);
  synth_time += (float)(AUDIO_BUFFER_FRAMES/2)/FRAME_RATE;
}

// ======================================================================
void BSP_AUDIO_OUT_Error_CallBack(void)
{
  printf("AUDIO OUT Error\r\n");
}

