/*
 * midisynth.c
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */

#include "midisynth.h"
#include "synth.h"
#include "synthutil.h"
#include "usb_host.h"
#include "../../Drivers/USBH_midi_class/Inc/usbh_MIDI.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>

// ======================================================================
// private defines

// USB MIDI buffer : max received data 64 bytes
#define RX_BUFF_SIZE 64

// ======================================================================
// private vars

extern USBH_HandleTypeDef hUsbHostFS;

uint8_t MIDI_RX_Buffer[RX_BUFF_SIZE]; // MIDI reception buffer

// ======================================================================
// private function prototypes

void decode_midi(uint16_t i, uint8_t midi_cmd, uint8_t midi_param0, uint8_t midi_param1);

// ======================================================================
// user code

// ======================================================================
// start the process of receiving midi info into the MIDI_RX_Buffer
void start_midi(void)
{
  printf("start_midi\r\n");
  USBH_MIDI_Receive(&hUsbHostFS, MIDI_RX_Buffer, RX_BUFF_SIZE);
}


// ======================================================================
void reset_cur_notes(void)
{
  for(int i = 0; i < MAX_POLYPHONY; i++) {
    reset_note(i);
  }
}

// ======================================================================
// decode midi input, react to note on/off commands
void decode_midi(uint16_t i, uint8_t midi_cmd, uint8_t midi_param0, uint8_t midi_param1)
{
  int8_t cur_idx = MAX_POLYPHONY;
  switch(midi_cmd & 0xf0) {
  case 0x80: // Note off
    for(int8_t j = 0; j < MAX_POLYPHONY; j++) {
      if(midi_param0 == get_note(j)) {
        cur_idx = j;
        break;
      }
    }
    if(cur_idx < MAX_POLYPHONY) {
      printf("Note off: %d %d %d\r\n", cur_idx, midi_param0, midi_param1);
      note_off(cur_idx);

    } else {
      printf("Note off: [NOPE] %d %d\r\n", midi_param0, midi_param1);
    }
    break;
  case 0x90: // Note on
    for(int8_t j = 0; j < MAX_POLYPHONY; j++) {
      if(0 == get_note(j)) {
        // found a spot!
        cur_idx = j;
        break;
      }
    }
    if(cur_idx < MAX_POLYPHONY) {
      printf("Note on: %d %d %d\r\n", cur_idx, midi_param0, midi_param1);
      note_on(cur_idx, midi_param0, midi_param1);
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

