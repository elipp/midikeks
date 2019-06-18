#include <stdio.h>
#include <stdlib.h>
#include <CoreMIDI/CoreMIDI.h>
#include <pthread.h>
#include <curses.h>
#include "midi.h"
#include "output.h"
#include "input.h"
#include "harmony.h"
#include "samples.h"

static void notifyproc(const MIDINotification *message, void *refCon) {

}



mqueue_t mqueue;
sound_t *sound = NULL;
fsound_t *fsound = NULL;

static int sound_enabled = 1;
static int harmony_enabled = 0;

double modulation = 1.0;

mevent_t mevent_new(int keyindex, double hz, double A, fsample_t *sample) {
    mevent_t e; 
    e.keyindex = keyindex;
    e.hz = hz;
    e.A = A;
    e.t = 0;
    e.phase = 0;
    e.sample = sample;
    e.sample_index = 0;
    return e;
}

void mqueue_init(mqueue_t *q) {
    memset(q->events, 0x0, sizeof(q->events));
}

void mqueue_add(mqueue_t *q, mevent_t *e) {
//    printf("adding %f to queue at index %d\n", e->hz, q->num_events);
    q->events[q->num_events] = *e; 
    ++q->num_events;
}

void mqueue_delete_at(mqueue_t *q, int i) {
 //   printf("num_events: %d, i: %d\n", q->num_events, i);
    if (i >= q->num_events) {
        return; // print a warning maybe
    }
    for (int j = i; j < q->num_events-1; ++j) {
        q->events[j] = q->events[j+1];
    }   
    --q->num_events;

}

void mqueue_update(mqueue_t *q) {
    for (int i = 0; i < q->num_events; ++i) {
        mevent_t *e = &q->events[i];
        if (!e->sample) {
            if (keys[e->keyindex].pressed) {
                e->A *= 0.9995;
            }
            else {
                e->A *= 0.99;
            }
        }
        else {
            if (!keys[e->keyindex].pressed) {
                e->A *= 0.995;
            }
            // else e->A = 1 ?
        }
    }
}

void mqueue_purge(mqueue_t *q) {
    for (int i = 0; i < q->num_events; ++i) {
        if (q->events[i].A < 0.001) {
            // this should work?
            mqueue_delete_at(q, i);
            mqueue_purge(q);
            return;
        }
    }
}

MIDIkey_t keys[128];
int sustain_pedal_down = 0;
static int left_pedal_down = 0;

void printPacketInfo(const MIDIPacket* packet) {
	double timeinsec = packet->timeStamp / (double)1e9;
	int i;

	if (packet->data[0] == 0xFE) return;
	printf("[%.3f]: ", timeinsec);
	for (i=0; i<packet->length; i++) {
		if (packet->data[i] < 0x7f) {
			printf("%d ", packet->data[i]);
		} else {
			printf("0x%X ", packet->data[i]);
		}
	}
	puts("");
}

static void print_currently_pressed_keys() {
	for (int m = MIDIKEY_MIN; m < MIDIKEY_MAX; ++m) {
		struct MIDIkey_t *k = &keys[m];
		if (k->pressed == 1) {
			printf("%d ", m);
		}
	}
	printf("\n");
}


enum {	
	I_UNISON = 0,
	I_MINOR2,
	I_MAJOR2,
	I_MINOR3,
	I_MAJOR3,
	I_P4,
	I_TRITONE,
	I_P5,
	I_MINOR6,
	I_MAJOR6,
	I_MINOR7,
	I_MAJOR7
};


static inline int interval_to_imask(int interval) { 
	if (interval == 0) return 0;

	else return (1 << interval);
}

static int get_interval_mask(int startindex) {
	int mask = 0;
	for (int i = startindex; i < 128; ++i) {
		if (keys[i].pressed) {
			int d = i - startindex;
			mask |= (1 << (d % 12));
		}
	}
	return mask;
}

static int get_lowest_pressed() {
	for (int i = 0; i < 128; ++i) {
		if (keys[i].pressed) return i;
	}

	return -1;
}

static int get_highest_pressed() {
    for (int i = 127; i >= 0; --i) {
        if (keys[i].pressed) return i;
    }
    return -1;
}

static void adjust_intonation(bool use_eqtemp) { // adJUST XDDDD
	static double just[] = {
		1.0,
		25.0/24.0,
		9.0/8.0,
		6.0/5.0,
		//19.0/16.0,	
		5.0/4.0,
		4.0/3.0,
		45.0/32.0,
		3.0/2.0,
		8.0/5.0,
		5.0/3.0,
		7.0/4.0,
		15.0/8.0,
	};

	int lowest = get_lowest_pressed();
	int minor7_adjust = 0;
	int p4_adjust = 0;
	int minor3_adjust = 0;
	int major6_adjust = 0;

	if (lowest == -1) { return; }

#define HAS_NOTE(interval) (imask & (interval_to_imask(interval)))

	int imask = get_interval_mask(lowest);
	if (HAS_NOTE(I_MINOR6)) {
		if (HAS_NOTE(I_P4)) {
			lowest -= I_P5;
		}
		else if (!HAS_NOTE(I_MAJOR3)) {
			lowest -= I_MAJOR3;
		}
	}
	else if (HAS_NOTE(I_P4) && HAS_NOTE(I_MAJOR6)) {
		lowest -= I_P5;
	}

	else if (HAS_NOTE(I_MINOR3)) {
		if (HAS_NOTE(I_MINOR7)) {
			minor7_adjust = 1;
			if (HAS_NOTE(I_P4)) { // minor11 sounds terrible with this p4
				p4_adjust = 1;
			}
		}
		else { 
			minor3_adjust = 1;
		}
	} else if (HAS_NOTE(I_MAJOR3) && HAS_NOTE(I_MAJOR6)) {
		major6_adjust = 1;
	}
	else if (HAS_NOTE(I_MAJOR2) && HAS_NOTE(I_TRITONE)) {
		lowest -= I_MINOR7;
	}

	while (lowest < 21) lowest += 12;

	MIDIkey_t *lk = &keys[lowest];
	double lkorig_hz = lk->hz;
	lk->hz = midikey_to_hz(lowest);

	if (lkorig_hz != 0) { 
		double newt = (lkorig_hz * lk->t)/lk->hz;
		lk->t = newt;
	}

	for (int i = lowest + 1; i < 128; ++i) {
		MIDIkey_t *k = &keys[i];
		if (k->A > 0.001) {
			double orig_hz = k->hz;
			int interval = (i - lowest);
			int times = interval / 12;
			int modulo = interval % 12;
			if (use_eqtemp) {
				k->hz = midikey_to_hz(i);
			}
			else {
				if (modulo == I_MINOR7 && minor7_adjust) {
					k->hz = (1<<times) * lk->hz * just[I_MINOR3] * just[I_P5];
				}
				else if (modulo == I_P4 && p4_adjust) {
					k->hz = (1<<times) * lk->hz * just[I_MINOR3]*just[I_MAJOR2];
				}
				else if (modulo == I_MINOR3 && minor3_adjust) {
					k->hz = (1<<times) * lk->hz * (19.0/16.0);
				}
				else if (modulo == I_MAJOR6 && major6_adjust) {
					k->hz = (1<<times) * lk->hz * ((just[I_MAJOR2]*just[I_P5] + just[I_MAJOR3]*just[I_P4]) / 2.0);
				}
				else {
					k->hz = (1<<times) * lk->hz * just[modulo];
				}
			}

			// adjust phase so no clicking occurs
			double newt = (orig_hz * k->t)/k->hz;
			k->t = newt;

		}
		else { memset(k, 0, sizeof(*k)); }

	}

}

static void key_on(MIDIkey_t *k, UInt8 velocity) {
    if (velocity == 0) {
        k->pressed = 0;
        return;
    }

    k->pressed = 1;
    k->A = (double)velocity/(double)0x7F;
    k->A = k->A * k->A; // exponential? ok


}

static void key_on2(UInt8 keyindex, UInt8 velocity) {

    if (velocity == 0) {
        return;
    }

    double A = (double)velocity/(double)0x7F;
    A *= A; // exponential? ok

    mevent_t e = mevent_new(keyindex, midikey_to_hz(keyindex), A, 
            sound_enabled ? get_fsample(fsound, keyindex) : NULL);

    mqueue_add(&mqueue, &e);

    if (!harmony_enabled) return;

    const voicing_t *v = get_voicing(keyindex);

    int i = 0;
    while (v->members[i] != E_END) { ++i; }
    int highest = v->pitches[i-1];
    int bass = keyindex - highest;
//    printf("keyindex: %d, highest: %d, bass %d (bass + highest = %d)\n", keyindex, highest, bass, bass + highest);
    
    mevent_t ve = mevent_new(keyindex, midikey_to_hz(bass - 12), 0.8*A, 
            sound_enabled ? get_fsample(fsound, bass-12) : NULL);
 //   mqueue_add(&mqueue, &ve);

    i = 0;
    while (v->members[i] != E_END) {
        ve = mevent_new(keyindex, midikey_to_hz(bass + v->pitches[i]), 0.75*A, 
                sound_enabled ? get_fsample(fsound, bass + v->pitches[i]) : NULL);
        mqueue_add(&mqueue, &ve);
        ++i;
    }

}

static void key_off_all() {
    for (int i = KEY_MIN; i < KEY_MAX; ++i) {
		struct MIDIkey_t *k = &keys[i];
        k->pressed = 0;
    }
}

void set_keyarray_state(const MIDIPacket *packet) {
	UInt8 command = packet->data[0];
	UInt8 keyindex = 0;
	UInt8 param1 = 0;
	UInt8 velocity = 0;

	struct MIDIkey_t *k = NULL;

	switch (command) {
		case MIDI_KEYDOWN:
			keyindex = packet->data[1];
			velocity = packet->data[2];
			k = &keys[keyindex];

            key_on(k, velocity);
            key_on2(keyindex, velocity);

			break;

		case MIDI_KEYUP:
            // OK so apparently, not all midi devices use this; some of them use a KEYDOWN of velocity 0 instead (see key_on())
			keyindex = packet->data[1];
			keys[keyindex].pressed = 0;
			break;

		case MIDI_CONTROL:

#define PEDAL_UNACORDA 0x43
#define PEDAL_MIDDLE 0x42
#define PEDAL_SUSTAIN 0x40
#define MODULATION_WHEEL 0x1
#define VOLUME 0x7

			keyindex = packet->data[1];
			param1 = packet->data[2];

			if (keyindex == PEDAL_MIDDLE) {
				if (param1 == 0x7F) {
					print_currently_pressed_keys();
				}
			}
			else if (keyindex == PEDAL_SUSTAIN) {
				if (param1 == 0x7F) {
					sustain_pedal_down = 1;	
				}

				else sustain_pedal_down = 0;
			}
			else if (keyindex == PEDAL_UNACORDA) {
				if (param1 == 0x7F) {
					left_pedal_down = 1;
				}
				else left_pedal_down = 0;
			}
            else if (keyindex == MODULATION_WHEEL) {
                modulation = (double)param1 / (double)0x7F;
                //printf("modulation: %f\n", modulation);
            }
            else if (keyindex == VOLUME) {
                eqtemp_factor = eqtemp_12 + 0.05*(double)(param1 - 0x3F)/(double)0x7F; 
            }
			break;

		default:
			break;

	}
}

static void readproc(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon) {
	MIDIPacket *packet = (MIDIPacket*)pktlist->packet;
	int i;
	int count = pktlist->numPackets;
	for (i=0; i<count; i++) {
//		printPacketInfo(packet);
		set_keyarray_state(packet);
		packet = MIDIPacketNext(packet); // this is quite necessary lol
	}

	adjust_intonation(!left_pedal_down);
}


char *cfstring_to_cstr(CFStringRef aString) {
	if (aString == NULL) {
		return NULL;
	}

	CFIndex length = CFStringGetLength(aString);
	CFIndex maxSize =
		CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
	char *buffer = (char *)malloc(maxSize);
	if (CFStringGetCString(aString, buffer, maxSize,
				kCFStringEncodingUTF8)) {
		return buffer;
	}
	free(buffer); // If we failed
	return NULL;
}

static int select_MIDI_input() {


    int device_index = -1;

	MIDIClientRef cl;
	MIDIClientCreate (CFSTR("MOII"), notifyproc, NULL, &cl);

	MIDIPortRef port;
	MIDIInputPortCreate(cl, CFSTR("MOII input"), readproc, NULL, &port);

	int ndevices = MIDIGetNumberOfSources();
	printf("number of MIDI sources: %d\n", ndevices);
 
    if (ndevices < 1) {
		fprintf(stderr, "no MIDI input devices detected, exiting!\n");
		return 0;
	}
    else if (ndevices == 1) {
        MIDIEndpointRef src = MIDIGetSource(0);
        CFStringRef name;
        OSStatus err = MIDIObjectGetStringProperty(src, kMIDIPropertyName, &name);

        char *device_name = cfstring_to_cstr(name);
        if (err == noErr) {
            printf("Defaulting to input device 0 (%s) (only 1 MIDI input device available)\n", device_name);
        }

        MIDIPortConnectSource(port, src, NULL);

        device_index = 0;
        free(device_name);
    }
    
    else {
        for (int i = 0; i < ndevices; ++i) {
            MIDIEndpointRef src = MIDIGetSource(i);
            CFStringRef name;
            OSStatus err = MIDIObjectGetStringProperty(src, kMIDIPropertyName, &name);

            char *device_name = cfstring_to_cstr(name);

            if (err == noErr) {
                printf("source %d: name: %s\n", i, device_name);
            }

            printf("If you want to select this MIDI input, enter y:\n");
            char buffer[256];

            fgets(buffer, 256, stdin);

            if (strcmp(buffer, "y\n") == 0) {
                MIDIPortConnectSource(port, src, NULL);
                if (device_name) free(device_name);
                device_index = i;
                printf("Device %d selected!\n", i);
                break;
            }
        }
    }

	if (device_index == -1) {
		printf("No device selected! Closing.\n");
		return 0;
	}

    return 1;

}

void init_curses() {
    initscr(); 
    cbreak(); 
    noecho(); 
    nonl(); 
    intrflush(stdscr, FALSE); 
    keypad(stdscr, TRUE);

    mvaddstr(0, 0, "--- JOOH JUUH MIDII JA SILLEE ---");
    mvaddstr(2, 0, "* press P to toggle piano sample sound");
    mvaddstr(3, 0, "* press H to toggle automatic harmony");

    refresh();

}

void *key_loop(void * _Nullable arg) {

    for (;;) {
        int c = getch();
        switch (c) {
            case 'p':
            case 'P':
                sound_enabled = !sound_enabled;   
                break;

            case 'h':
            case 'H':
                harmony_enabled = !harmony_enabled;
                break;

            default:
                break;

        }
    }
}

int main(int argc, char *args[]) {

    srand(time(NULL));
	memset(keys, 0, sizeof(keys));
    mqueue_init(&mqueue);

	init_eqtemp_hztable();
    init_voicings();

//    sound = load_sound(SAMPLE_TYPE_RAW, "samples/16/raws/piano_%d.raw", 2);
    fsound = load_fsound(SAMPLE_TYPE_RAW, "samples/16/raws/piano_%d.raw", 2);
		
    if (!select_MIDI_input()) return 1;

	if (!init_output()) return 1;
//	if (!init_input()) return 1;
//	start_input();

    init_curses();

    pthread_t th;
    pthread_create(&th, NULL, key_loop, NULL);

	CFRunLoopRef runLoop;
	runLoop = CFRunLoopGetCurrent();
	CFRunLoopRun();

    pthread_exit(NULL);

	return 0;
}
