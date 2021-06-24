/*
 * synth.c
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */

#include "synth.h"
#include "synthutil.h"
#include "main.h"
#include "../../Drivers/BSP/STM32F4-Discovery/stm32f4_discovery_audio.h"
#include <math.h>
#include <stdio.h>

// ======================================================================
// private defines

// defines for wave table
#define WAVE_TABLE_LENGTH 256

// defines for audio buffer sizing
#define AUDIO_BUFFER_FRAMES   256
#define AUDIO_BUFFER_CHANNELS 2
#define AUDIO_BUFFER_SAMPLES  AUDIO_BUFFER_FRAMES * AUDIO_BUFFER_CHANNELS
#define AUDIO_BUFFER_BYTES    sizeof(int16_t)*AUDIO_BUFFER_SAMPLES

#define HARDWARE_VOLUME 86
#define SAMPLE_RATE    48000


// polyphony
#define MAX_POLYPHONY 10

// ======================================================================
// private vars

// audio buffer that is sent over I2S to DAC
uint16_t audio_buffer[AUDIO_BUFFER_SAMPLES];
// wave table that is used to update the audio_buffer
float wave_table[WAVE_TABLE_LENGTH];

// the wave table has a pointer to the current sample
float cur_wave_table_phases[MAX_POLYPHONY];
// button pushes update the note
uint8_t cur_notes[MAX_POLYPHONY];
float cur_notes_hz[MAX_POLYPHONY];
// play only when button is down
float cur_volumes[MAX_POLYPHONY];

// ======================================================================
// private function prototypes


// ======================================================================
// user code

// ======================================================================
void reset_note(int i) {
  cur_wave_table_phases[i] = 0.0;
  cur_notes[i] = 0; // only used for midi stuff
  cur_notes_hz[i] = pitch_to_freq(A4);
  cur_volumes[i] = 0.0;
}

// ======================================================================
// for pushbutton -- just activate one voice after resetting all others
void activate_one_voice(void)
{
  cur_volumes[0] = 1.0;
}

int8_t get_note(int i) {
  return cur_notes[i];
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
    cur_notes[cur_idx] = 0;
    cur_notes_hz[cur_idx] = pitch_to_freq(0);
    cur_volumes[cur_idx] = 0.0;
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
    cur_notes[cur_idx] = midi_param0;
    cur_notes_hz[cur_idx] = pitch_to_freq(midi_param0);
    cur_volumes[cur_idx] = 0.3 * (float)midi_param1/127.0;
  } else {
    printf("Note on: [NOPE] %d %d\r\n", midi_param0, midi_param1);
  }
}
// ======================================================================
void reset_cur_notes(void)
{
  for(int i = 0; i < MAX_POLYPHONY; i++) {
    reset_note(i);
  }
}

// ======================================================================
// create a wave_table with a single sine wave cycle.
// values are stored as float to make further math easy.
void init_wave_table(void)
{
  float phase_inc = (2.0f * (float)M_PI) / (float)WAVE_TABLE_LENGTH;
  float phase = 0;
  for (int i = 0; i < WAVE_TABLE_LENGTH; i++) {
    wave_table[i] = sin(phase);
    phase += phase_inc;
  }
}

// ======================================================================
void audio_init(void)
{
  reset_cur_notes();

  init_wave_table();
  update_audio_buffer(0, AUDIO_BUFFER_FRAMES);

  if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, HARDWARE_VOLUME, SAMPLE_RATE) != AUDIO_OK) {
    Error_Handler();
  }

  // tell the chip to start DMA from audio_buffer
  BSP_AUDIO_OUT_Play(&(audio_buffer[0]), AUDIO_BUFFER_BYTES);
}


// ======================================================================
// using cur_phase, read from wave_table[] and update the
// audio_buffer from start to start+num_frames
void update_audio_buffer(uint32_t start_frame, uint32_t num_frames)
{

  for(int frame = start_frame; frame < start_frame+num_frames; frame++) {
    float sample_f = 0.0;
    // add all the active samples up
    for(int note = 0; note < MAX_POLYPHONY; note++) {
      if(cur_volumes[note] > 0.0) {
        float cur_wave_table_phase_inc = (cur_notes_hz[note] / SAMPLE_RATE) * WAVE_TABLE_LENGTH;
        sample_f += cur_volumes[note] * wave_table[(uint32_t)cur_wave_table_phases[note]];
        cur_wave_table_phases[note] += cur_wave_table_phase_inc;
        if(cur_wave_table_phases[note] > WAVE_TABLE_LENGTH) {
          cur_wave_table_phases[note] -= WAVE_TABLE_LENGTH;
        }
      }
    }
    // https://www.cs.cmu.edu/~rbd/papers/cmj-float-to-int.html
    if (sample_f > 1.0) {
      sample_f = 1.0;
    } else if (sample_f < -1.0) {
      sample_f = -1.0;
    }
    uint16_t sample = float2uint16(sample_f);
    // each frame is 2 samples
    audio_buffer[2*frame] = sample;
    audio_buffer[2*frame+1] = sample;
  }
}

// ======================================================================
// after the first half of the audio_buffer has been
// transferred, fill that portion with new samples
// while the second half is being played.
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
  update_audio_buffer(0, AUDIO_BUFFER_FRAMES/2);
}

// ======================================================================
// now the second half has been transferred.
// fill in the second half with new samples
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
  update_audio_buffer(AUDIO_BUFFER_FRAMES/2, AUDIO_BUFFER_FRAMES/2);
}
