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

void set_keyarray_state(const MIDIPacket *packet) {
	UInt8 command = packet->data[0];
	UInt8 keyindex = 0;
	UInt8 param1 = 0;

	switch (command) {
		case MIDI_KEYDOWN:
			keyindex = packet->data[1];
			printf("keydown for %d\n", keyindex);
			keys[keyindex].pressed = 1;
			keys[keyindex].t = 0;
			keys[keyindex].A = 1;
			break;

		case MIDI_KEYUP:
			keyindex = packet->data[1];
			printf("keyup for %d\n", keyindex);
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
//		packet = MIDIPacketNext(packet);
	}
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
