/*
 * midisynth.h
 *
 *  Created on: Jun 24, 2021
 *      Author: rallen
 */

#ifndef INC_MIDISYNTH_H_
#define INC_MIDISYNTH_H_

// ======================================================================
// public function prototypes

void audio_init(void);
void reset_cur_notes(void);
void activate_one_voice(void);
void start_midi(void);

#endif /* INC_MIDISYNTH_H_ */
