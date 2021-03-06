#ifndef MIDI_H
#define MIDI_H

#define MIDI_KEYUP 0x80
#define MIDI_KEYDOWN 0x90
#define MIDI_CONTROL 0xB0

#define MIDIKEY_MIN 20
#define MIDIKEY_MAX 109 

#define DUMP

#include "samples.h"

enum {
    HARMONY_DISABLED = 0,
    HARMONY_ENABLED
};

typedef struct MIDIkey_t {
	int pressed;
	int velocity;
	double t;
	double A;
	double hz; 
	double phase;
} MIDIkey_t;

typedef struct mevent_t {
    double t;
    double A;
    double hz;
    double phase;
    double decay;
    int keyindex;
    fsample_t *sample;
    int sample_index;
    // add sound here too
} mevent_t;

typedef struct mqueue_t {
    mevent_t events[256]; // that should be quite enough
    int num_events;
} mqueue_t;

extern MIDIkey_t keys[];

mevent_t mevent_new(int keyindex, double hz, double A, fsample_t *sample);

void mqueue_init(mqueue_t *q);
void mqueue_add(mqueue_t *q, mevent_t *e);
void mqueue_purge();
void mqueue_delete_at(mqueue_t *q, int i);

extern mqueue_t mqueue;
extern sound_t *sound;
extern int sustain_pedal_down;

extern double modulation;

#endif
