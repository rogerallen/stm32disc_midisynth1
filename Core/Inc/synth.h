/*
 * synth.h
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */

#ifndef INC_SYNTH_H_
#define INC_SYNTH_H_

#include <stdint.h>

void audio_init(void);
void reset_cur_notes(void);
void reset_note(int i);
void activate_one_voice(void);
int8_t get_note(int i);
void init_wave_table(void);
void init_audio_buffer(void);
void update_audio_buffer(uint32_t start_frame, uint32_t num_frames);
void note_off(uint8_t midi_cmd, uint8_t midi_param0, uint8_t midi_param1);
void note_on(uint8_t midi_cmd, uint8_t midi_param0, uint8_t midi_param1);


#endif /* INC_SYNTH_H_ */
