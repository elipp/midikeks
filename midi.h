#ifndef MIDI_H
#define MIDI_H

typedef struct MIDIkey_t {
	int pressed;
	int velocity;
} MIDIKEY_t;

extern MIDIKEY_t keys[];

#endif
