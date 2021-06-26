/*
 * synth.h
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */

#ifndef INC_SYNTH_H_
#define INC_SYNTH_H_

#include <stdint.h>

// FIXME synth_xxx public fn renaming
void audio_init(void);
void reset_cur_notes(void);
void activate_one_voice(void);
void note_off(uint8_t midi_cmd, uint8_t midi_param0, uint8_t midi_param1);
void note_on(uint8_t midi_cmd, uint8_t midi_param0, uint8_t midi_param1);

#endif /* INC_SYNTH_H_ */
