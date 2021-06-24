/*
 * midisynth.c
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */

#include "midisynth.h"
#include "usb_host.h"
#include "../../Drivers/BSP/STM32F4-Discovery/stm32f4_discovery_audio.h"
#include "../../Drivers/USBH_midi_class/Inc/usbh_MIDI.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>

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
float   CHROMATIC_BASE = pow(2.0f, 1.0f / 12.0f);

// USB MIDI buffer : max received data 64 bytes
#define RX_BUFF_SIZE 64

// polyphony
#define MAX_POLYPHONY 10

// ======================================================================
// private vars

// audio buffer that is sent over I2S to DAC
uint16_t audio_buffer[AUDIO_BUFFER_SAMPLES];
// wave table that is used to update the audio_buffer
float wave_table[WAVE_TABLE_LENGTH];
// set up a small chromatic scale to use
typedef enum { C4 = 60, Cs4, D4, Ds4, E4, F4, Fs4, G4, Gs4, A4, As4, B4 } note_t;

extern USBH_HandleTypeDef hUsbHostFS;

uint8_t MIDI_RX_Buffer[RX_BUFF_SIZE]; // MIDI reception buffer

uint8_t cur_polyphony = 0;
// the wave table has a pointer to the current sample
float cur_wave_table_phases[MAX_POLYPHONY];
// button pushes update the note
uint8_t cur_notes[MAX_POLYPHONY];
float cur_notes_hz[MAX_POLYPHONY];
// play only when button is down
float cur_volumes[MAX_POLYPHONY];

// ======================================================================
// private function prototypes

float note_to_freq(uint8_t note);
void init_wave_table(void);
void init_audio_buffer(void);
uint16_t float2uint16(float f);
void update_audio_buffer(uint32_t start_frame, uint32_t num_frames);
void decode_midi(uint16_t i, uint8_t midi_cmd, uint8_t midi_param0, uint8_t midi_param1);

// ======================================================================
// user code

// ======================================================================
// for pushbutton -- just activate one voice after resetting all others
void activate_one_voice(void)
{
  cur_volumes[0] = 1.0;
}

// ======================================================================
// start the process of receiving midi info into the MIDI_RX_Buffer
void start_midi(void)
{
  printf("start_midi\r\n");
  USBH_MIDI_Receive(&hUsbHostFS, MIDI_RX_Buffer, RX_BUFF_SIZE);
}

// ======================================================================
// convert midi note index to frequency in Hz.  A4 440Hz = midi note 69
float note_to_freq(uint8_t note)
{
  return 440*pow(CHROMATIC_BASE, (float)note - 69);
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
// float to uint16 produces two's complement values with -1.0 => 0x8001,
// 0.0 => 0x0000 and 1.0 => 0x7fff.  It also has good rounding behavior
// near 0.0 per https://www.cs.cmu.edu/~rbd/papers/cmj-float-to-int.html
inline uint16_t float2uint16(float f)
{
    return (uint16_t)(((int16_t)(32767*f + 32768.5)) - 32768);
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
void reset_cur_notes(void)
{
  for(int i = 0; i < MAX_POLYPHONY; i++) {
    cur_wave_table_phases[i] = 0.0;
    cur_notes[i] = 0; // only used for midi stuff
    cur_notes_hz[i] = note_to_freq(A4);
    cur_volumes[i] = 0.0;
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
// decode midi input, react to note on/off commands
void decode_midi(uint16_t i, uint8_t midi_cmd, uint8_t midi_param0, uint8_t midi_param1)
{
  int8_t cur_idx = MAX_POLYPHONY;
  switch(midi_cmd & 0xf0) {
  case 0x80: // Note off
    for(int8_t j = 0; j < MAX_POLYPHONY; j++) {
      if(midi_param0 == cur_notes[j]) {
        cur_idx = j;
        break;
      }
    }
    if(cur_idx < MAX_POLYPHONY) {
      printf("Note off: %d %d %d\r\n", cur_idx, midi_param0, midi_param1);
      cur_notes[cur_idx] = 0;
      cur_notes_hz[cur_idx] = note_to_freq(0);
      cur_volumes[cur_idx] = 0.0;
    } else {
      printf("Note off: [NOPE] %d %d\r\n", midi_param0, midi_param1);
    }
    break;
  case 0x90: // Note on
    for(int8_t j = 0; j < MAX_POLYPHONY; j++) {
      if(0 == cur_notes[j]) {
        // found a spot!
        cur_idx = j;
        break;
      }
    }
    if(cur_idx < MAX_POLYPHONY) {
      printf("Note on: %d %d %d\r\n", cur_idx, midi_param0, midi_param1);
      cur_notes[cur_idx] = midi_param0;
      cur_notes_hz[cur_idx] = note_to_freq(midi_param0);
      cur_volumes[cur_idx] = 0.3 * (float)midi_param1/127.0;
    } else {
      printf("Note on: [NOPE] %d %d\r\n", midi_param0, midi_param1);
    }

    break;
  case 0xa0: // Aftertouch
  case 0xB0: // Continuous controller
  case 0xC0: // Patch change
  case 0xD0: // Channel Pressure
  case 0xE0: // Pitch bend
  case 0xF0: // (non-musical commands)
    printf("%d: %02x %02x %02x\r\n", i, midi_cmd, midi_param0, midi_param1);
    printf("command not handled\r\n");
    break;
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

// ======================================================================
void USBH_MIDI_ReceiveCallback(USBH_HandleTypeDef *phost)
{
  // each USB midi package is 4 bytes long
  uint16_t numberOfPackets = USBH_MIDI_GetLastReceivedDataSize(&hUsbHostFS) / 4;
  //printf("midi received %d packets.\r\n", numberOfPackets);
  for(uint16_t i = 0; i < numberOfPackets; ++i) {
    uint8_t cin_cable   = MIDI_RX_Buffer[4*i+0];
    uint8_t midi_cmd    = MIDI_RX_Buffer[4*i+1];
    uint8_t midi_param0 = MIDI_RX_Buffer[4*i+2];
    uint8_t midi_param1 = MIDI_RX_Buffer[4*i+3];
    if(cin_cable == 0) {
      continue;
    }
    decode_midi(i, midi_cmd, midi_param0, midi_param1);
  }
  // start a new reception
  USBH_MIDI_Receive(&hUsbHostFS, MIDI_RX_Buffer, RX_BUFF_SIZE);
}

