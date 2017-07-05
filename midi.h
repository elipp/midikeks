#ifndef MIDI_H
#define MIDI_H

typedef struct MIDIkey_t {
	int pressed;
	int velocity;
	double t;
	double A;
	double hz; 
	double phase;
} MIDIKEY_t;

extern MIDIKEY_t keys[];

#endif
