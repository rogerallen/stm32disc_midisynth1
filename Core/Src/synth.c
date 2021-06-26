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
//  [ audio samples required ]
//        VV
//  [ Mix i polyphony
//          VV
//    [ Wavetable i ]
//          VV
//    [ Envelope i ]
//          VV
//    Mix ]
//     VV
//  [ Pan     ]
//     VV
//  [ Compressor ]
//     VV
//  [ Res Filter ]
//     VV
//  [  Reverb  ]
//     VV
//  [  Output  ]
//
#include "synth.h"
#include "synthutil.h"
#include "wavetable.h"
#include "main.h"
#include "../../Drivers/BSP/STM32F4-Discovery/stm32f4_discovery_audio.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// ======================================================================
// private defines

// defines for audio buffer sizing
#define AUDIO_BUFFER_FRAMES   256
#define AUDIO_BUFFER_CHANNELS 2
#define AUDIO_BUFFER_SAMPLES  AUDIO_BUFFER_FRAMES * AUDIO_BUFFER_CHANNELS
#define AUDIO_BUFFER_BYTES    sizeof(int16_t)*AUDIO_BUFFER_SAMPLES

#define HARDWARE_VOLUME 86

// polyphony
#define MAX_POLYPHONY 10

// ======================================================================
// private vars

// audio buffer that is sent over I2S to DAC
uint16_t audio_buffer[AUDIO_BUFFER_SAMPLES];

// temp ping-pong buffers to use for float intermediate data
float sample_buffer[2][AUDIO_BUFFER_SAMPLES];

// midi events pushes update the note
wavetable_state_t wavetables[MAX_POLYPHONY];

// ======================================================================
// private function prototypes

void reset_note(int i);
int8_t get_note(int i);
void init_audio_buffer(void);
void update_audio_buffer(uint32_t start_frame, uint32_t num_frames);

// ======================================================================
// user code

// ======================================================================
void audio_init(void)
{
  for(int i=0; i < MAX_POLYPHONY; i++) {
    wavetable_init( &(wavetables[i]) );
  }
  //reset_cur_notes();
  //init_wave_table();

  update_audio_buffer(0, AUDIO_BUFFER_FRAMES);

  if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, HARDWARE_VOLUME, SAMPLE_RATE) != AUDIO_OK) {
    Error_Handler();
  }

  // tell the chip to start DMA from audio_buffer
  BSP_AUDIO_OUT_Play(&(audio_buffer[0]), AUDIO_BUFFER_BYTES);
}


// ======================================================================
void reset_note(int i) {
  wavetable_note_off( &(wavetables[i]) ); // FIXME? freq=A4?
}

// ======================================================================
void reset_cur_notes(void)
{
  for(int i = 0; i < MAX_POLYPHONY; i++) {
    reset_note(i);
  }
}

// ======================================================================
// for pushbutton -- just activate one voice after resetting all others
void activate_one_voice(void)
{
  wavetables[0].volume = 1.0;
}

int8_t get_note(int i) {  // fixme get pitch
  return wavetables[i].pitch;
}

void note_off(uint8_t midi_cmd, uint8_t midi_param0, uint8_t midi_param1)
{
  int8_t cur_idx = MAX_POLYPHONY;
  for(int8_t j = 0; j < MAX_POLYPHONY; j++) {
    if(midi_param0 == get_note(j)) {
      cur_idx = j;
      break;
    }
  }
  if(cur_idx < MAX_POLYPHONY) {
    printf("Note off: %d %d %d\r\n", cur_idx, midi_param0, midi_param1);
    wavetable_note_off(&(wavetables[cur_idx]));
  } else {
    printf("Note off: [NOPE] %d %d\r\n", midi_param0, midi_param1);
  }
}

void note_on(uint8_t midi_cmd, uint8_t midi_param0, uint8_t midi_param1)
{
  int8_t cur_idx = MAX_POLYPHONY;
  for(int8_t j = 0; j < MAX_POLYPHONY; j++) {
    if(0 == get_note(j)) {
      // found a spot!
      cur_idx = j;
      break;
    }
  }
  if(cur_idx < MAX_POLYPHONY) {
    printf("Note on: %d %d %d\r\n", cur_idx, midi_param0, midi_param1);
    wavetable_note_on(&(wavetables[cur_idx]), midi_param0, midi_param1);
  } else {
    printf("Note on: [NOPE] %d %d\r\n", midi_param0, midi_param1);
  }
}

// ======================================================================
// using cur_phase, read from wave_table[] and update the
// audio_buffer from start to start+num_frames
void update_audio_buffer(uint32_t start_frame, uint32_t num_frames)
{

  int src = 0, dst = 1;
  memset(&(sample_buffer[src][0]), 0, sizeof(float)*AUDIO_BUFFER_SAMPLES);

  for(int note = 0; note < MAX_POLYPHONY; note++) {
    wavetable_add_samples(&(wavetables[note]), &(sample_buffer[src][0]), &(sample_buffer[dst][0]), num_frames);
    src = src == 0 ? 1 : 0;
    dst = dst == 0 ? 1 : 0;
  }

  int i = 0;
  for(int frame = start_frame; frame < start_frame+num_frames; frame++) {
    float sample0_f = sample_buffer[src][2*i];
    float sample1_f = sample_buffer[src][2*i+1];
    sample0_f = (sample0_f > 1.0) ? sample0_f = 1.0 : (sample0_f < -1.0) ? -1.0 : sample0_f;
    sample1_f = (sample1_f > 1.0) ? sample1_f = 1.0 : (sample1_f < -1.0) ? -1.0 : sample1_f;
    audio_buffer[2*frame] = float2uint16(sample0_f);
    audio_buffer[2*frame+1] = float2uint16(sample1_f);
    i++;
  }
}

// ======================================================================
// after the first half of the audio_buffer has been transferred, fill
// that portion with new samples while the second half is being played.
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
  update_audio_buffer(0, AUDIO_BUFFER_FRAMES/2);
}

// ======================================================================
// now the second half has been transferred. fill in the second half
// with new samples.
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
  update_audio_buffer(AUDIO_BUFFER_FRAMES/2, AUDIO_BUFFER_FRAMES/2);
}
