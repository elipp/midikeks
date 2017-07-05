#include <stdio.h>
#include <CoreMIDI/CoreMIDI.h>
//#include <GameController/GCController.h>
#include "midi.h"
#include "output.h"

static void notifyproc(const MIDINotification *message, void *refCon) {

}

MIDIKEY_t keys[128];
static int sustain_pedal_down = 0;

#define MIDI_KEYUP 0x80
#define MIDI_KEYDOWN 0x90
#define MIDI_SUSTAINPEDAL 0xB0

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
	for (int m = 20; m < 109; ++m) {
		struct MIDIkey_t *k = &keys[m];
		if (k->pressed == 1) {
			printf("%d ", m);
		}
	}
	printf("\n");
}

static double eqtemp_hz[128];

static void set_eqtemp_hz() {
	for (int i = 0; i < 128; ++i) {
		eqtemp_hz[i] = midikey_to_hz(i);
	}
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
		if (keys[i].pressed) { 
			return i;
		}
	}

	return -1;
}

static void adjust_intonation(bool use_eqtemp) {
	static double justratios[] = {
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
	}


	double bf = midikey_to_hz(lowest);
	keys[lowest].hz = bf;

	for (int i = lowest + 1; i < 128; ++i) {
		MIDIKEY_t *k = &keys[i];
		if (k->pressed) {
			int interval = (i - lowest);
			int times = interval / 12;
			int modulo = interval % 12;
			if (use_eqtemp) {
				k->hz = midikey_to_hz(i);
			}
			else {
				if (modulo == I_MINOR7 && minor7_adjust) {
					k->hz = (1<<times) * bf * justratios[I_MINOR3] * justratios[I_P5];
				}
				else if (modulo == I_P4 && p4_adjust) {
					k->hz = (1<<times) * bf * justratios[I_MINOR3]*justratios[I_MAJOR2];
				}
				else if (modulo == I_MINOR3 && minor3_adjust) {
					k->hz = (1<<times) * bf * (19.0/16.0);
				}
				else {
					k->hz = (1<<times) * bf * justratios[modulo];
				}
			}
		}
	}

}

void set_keyarray_state(const MIDIPacket *packet) {
	UInt8 command = packet->data[0];
	UInt8 keyindex = 0;
	UInt8 param1 = 0;

	switch (command) {
		case MIDI_KEYDOWN:
			keyindex = packet->data[1];
			keys[keyindex].pressed = 1;
			keys[keyindex].t = 0;
			keys[keyindex].A = 1;
			break;

		case MIDI_KEYUP:
			keyindex = packet->data[1];
			keys[keyindex].pressed = 0;
			break;

		case MIDI_SUSTAINPEDAL:
			keyindex = packet->data[1];
			param1 = packet->data[2];
			if (keyindex == 66) { // this is middle pedal (64 for sustain pedal)
				if (param1 == 0x7F) {
					print_currently_pressed_keys();
				}
			}
			else if (keyindex == 64) {
				if (param1 == 0x7F) {
					sustain_pedal_down = 1;	
				}

				else sustain_pedal_down = 0;
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
		//printPacketInfo(packet);
		set_keyarray_state(packet);
		packet = MIDIPacketNext(packet); // this is really necessary!! lol
	}
	adjust_intonation(!sustain_pedal_down);
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

int main(int argc, char *args[]) {

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

	set_eqtemp_hz();
	
	for (int i = 0; i < ndevices; ++i) {
		MIDIEndpointRef src = MIDIGetSource(i);
		CFStringRef name;
		OSStatus err = MIDIObjectGetStringProperty(src, kMIDIPropertyName, &name);

		char *str = cfstring_to_cstr(name);

		if (err == noErr) {
			printf("source %d: name: %s\n", i, str);
		}

		MIDIPortConnectSource(port, src, NULL);

		if (str) free(str);
	}

	if (!init_output()) return 1;

	CFRunLoopRef runLoop;
	runLoop = CFRunLoopGetCurrent();
	CFRunLoopRun();

	return 0;
}
