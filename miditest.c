#include <stdio.h>
#include <CoreMIDI/CoreMIDI.h>

#include "midi.h"
#include "output.h"

static void notifyproc(const MIDINotification *message, void *refCon) {

}

MIDIKEY_t keys[128];

#define MIDI_KEYUP 0x80
#define MIDI_KEYDOWN 0x90

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

void set_keyarray_state(const MIDIPacket *packet) {
	UInt8 command = packet->data[0];
	if (command == MIDI_KEYDOWN) {
		UInt8 keyindex = packet->data[1];
		keys[keyindex].pressed = 1;
		keys[keyindex].t = 0;
		keys[keyindex].A = 1;
	}
	else if (command == MIDI_KEYUP) {
		UInt8 keyindex = packet->data[1];
		keys[keyindex].pressed = 0;
	}
}

static void readproc(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon) {
	MIDIPacket *packet = (MIDIPacket*)pktlist->packet;
	int i;
	int count = pktlist->numPackets;
	for (i=0; i<count; i++) {
//		printPacketInfo(packet);
		set_keyarray_state(packet);
		packet = MIDIPacketNext(packet);
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
