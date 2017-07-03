#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>

#include "midi.h"

//typedef OSStatus (*AURenderCallback)(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);

#define PREFERRED_FRAMESIZE 64

static const double eqtemp_factor = 1.0594630943592953;
//static const double eqtemp_factor = 1.055;
static const double twopi = 6.2831853071795865;

static const double dt = 1.0/44100.0;

static inline double waveform_sine(double freq, double t, double phi) {
	return sin(freq * twopi * t + phi);
}

static double waveform_triangle(double freq, double t, double phi) {
	float p = (twopi*freq*t + phi)/twopi;
	float phase = p - floor(p);
	return phase < 0.5 ? (-4*phase + 1) : (4*phase - 3);
}

static inline double waveform_sine_limit(double freq, double t, double phi, double limit_abs) {
	double h = (27.5/freq);
	double h2 = 0.5*h;
	double d = sin(freq * twopi * t + phi) + 
		h * sin(2*freq*twopi*t + phi)
		+h * sin(3*freq*twopi*t + phi)
		+h2 * waveform_triangle(4*freq, t, phi);

	if (d > limit_abs) {
		d = limit_abs;
	}
	if (d < -limit_abs) {
		d = -limit_abs;
	}
	return d;
}




static short *waveform_precalculated;

static short *precalculate_waveform() {
	short* w = malloc(44100*sizeof(short));
	
	for (int i = 0; i < 44100; ++i) {
		w[i] = waveform_triangle(1, i*dt, 0);
	}

	return w;
}

static short lerp_waveform(double freq, double t, double phi) {
	double step = 44100.0/freq;
	double s = twopi*t + phi;
	
	return 0;
}

float midikey_to_hz(int index) {
	// key 21 maps to lowest A (27.5 Hz @ A=440Hz)
	// key 108 maps to highest C

	return 27.5 * pow(eqtemp_factor, index - 21);
}

double frand(double fMin, double fMax) {
	double f = (double)rand() / RAND_MAX;
	return fMin + f * (fMax - fMin);
}

float midikey_to_hz_random(int index) {
	static float hz[128];
	static int initialized = 0;

	if (!initialized) {
		hz[0] = 27.5;
		for (int i = 1; i < 128; ++i) {
			hz[i] = hz[i-1] * frand(0.94, 1.12);
		}
		initialized = 1;
	}

	return hz[index];

}

static short *dumpdata;
static int dumpoffset = 0;
static int dumping = 0;

#define DUMPSAMPLES 96000

static int start_dumping() {
	if (dumping == 0) {
		printf("starting dump!\n");
		dumpdata = malloc(DUMPSAMPLES*sizeof(short));
		dumping = 1;
	}
	return 0;
}

static int stop_dumping() {
	if (dumping == 1) {
		printf("stopping dump\n");
		FILE *fp = fopen("dumpfile.dat", "wb");
		fwrite(dumpdata, sizeof(short), DUMPSAMPLES, fp); 
		fclose(fp);
		free(dumpdata);
		dumping = 2;
	}

	return 1;
}

static int append_to_dump(short *buffer, int num_samples) {
	if (dumping != 1) return 0;
	memcpy(&dumpdata[dumpoffset], buffer, num_samples*sizeof(short));
	dumpoffset += num_samples;
	return dumpoffset;
}

static int get_current_dumpoffset() {
	return dumpoffset;
}

static OSStatus rcallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
	//printf("%f\n", inTimeStamp->mSampleTime);

	AudioBuffer *b = &ioData->mBuffers[0];

	short *samples = (short*)b->mData;
	memset(samples, 0, b->mDataByteSize);

	static const double A = 0.20 * 32768.0;
	int num_keys = 0;

	for (int m = 20; m < 109; ++m) {
		struct MIDIkey_t *k = &keys[m];
		if (k->A > 0.05) {
//			start_dumping();
			for (int i = 0; i < PREFERRED_FRAMESIZE; ++i) {
				//double tv = 0.0003*sin(5*twopi*t);
				samples[i] += A * k->A * waveform_sine_limit(midikey_to_hz(m), k->t, 0, 0.5);
//				samples[i] += A * k->A * waveform_sine(midikey_to_hz(m), k->t, 0);
				k->t += dt;
				//k->t = (k->t > 1.0) ? k->t - 1 : k->t; // not sure why this wouldn't work O_O
			}
			if (!k->pressed) {
				k->A *= 0.95;
			}
			++num_keys;
		}
	}

//	if (get_current_dumpoffset() + PREFERRED_FRAMESIZE >= DUMPSAMPLES) stop_dumping();
//	if (dumping == 1) append_to_dump(samples, PREFERRED_FRAMESIZE);

	return noErr;
}

enum format_type {
	FMT_NULL,
	FMT_S16_LE,
	FMT_S16_BE,
	FMT_S32_LE,
	FMT_S32_BE,
	FMT_FLOAT
};

struct CoreAudioFormatDescription{
	enum format_type type;
	int bits_per_sample;
	int bytes_per_sample;
	unsigned int flags;
};
static struct CoreAudioFormatDescription format_map[] = {
	{FMT_S16_LE, 16, sizeof (int16_t), kAudioFormatFlagIsSignedInteger},
	{FMT_S16_BE, 16, sizeof (int16_t), kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsBigEndian},
	{FMT_S32_LE, 32, sizeof (int32_t), kAudioFormatFlagIsSignedInteger},
	{FMT_S32_BE, 32, sizeof (int32_t), kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsBigEndian},
	{FMT_FLOAT,  32, sizeof (float),   kAudioFormatFlagIsFloat},
	{FMT_NULL, 0, 0, 0},
};

static AudioComponent output_comp;
static AudioComponentInstance output_instance;

static bool init (void) {

	/* open the default audio device */
	AudioComponentDescription desc;
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_DefaultOutput;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;

	output_comp = AudioComponentFindNext (NULL, &desc);
	if (!output_comp) {
	       	fprintf (stderr, "Failed to open default audio device.\n");
		return false;
	}

	if (AudioComponentInstanceNew(output_comp, &output_instance)) {
		fprintf (stderr, "Failed to open default audio device.\n");
		return false;
	}

	return true;
}

static void cleanup (void) {
	AudioUnitUninitialize(output_instance);
}

static bool open_audio (int format, int rate, int channels, AURenderCallbackStruct *callback) {
	struct CoreAudioFormatDescription *it = &format_map[0];
	struct CoreAudioFormatDescription *m = NULL;

	while (it->type != FMT_NULL) {
		if (it->type == format) {
			m = it;
			break;
		}
		++it;
	}

	if (!m) {
		fprintf (stderr, "The requested audio format %d is unsupported.\n", format);
		return false;
	}

	if (AudioUnitInitialize (output_instance)) {
		fprintf (stderr, "Unable to initialize audio unit instance\n");
		return false;
	}

	AudioStreamBasicDescription streamFormat;
	streamFormat.mSampleRate = rate;
	streamFormat.mFormatID = kAudioFormatLinearPCM;
	streamFormat.mFormatFlags = m->flags;
	streamFormat.mFramesPerPacket = 1;
	streamFormat.mChannelsPerFrame = channels;
	streamFormat.mBitsPerChannel = m->bits_per_sample;
	streamFormat.mBytesPerPacket = channels * m->bytes_per_sample;
	streamFormat.mBytesPerFrame = channels * m->bytes_per_sample;

	printf ("Stream format:\n");
	printf (" Channels: %d\n", streamFormat.mChannelsPerFrame);
	printf (" Sample rate: %f\n", streamFormat.mSampleRate);
	printf (" Bits per channel: %d\n", streamFormat.mBitsPerChannel);
	printf (" Bytes per frame: %d\n", streamFormat.mBytesPerFrame);

	if (AudioUnitSetProperty (output_instance, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &streamFormat, sizeof(streamFormat)))
	{
		fprintf (stderr, "Failed to set audio unit input property.\n");
		return false;
	}

	if (AudioUnitSetProperty (output_instance, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, callback, sizeof (AURenderCallbackStruct))) {
		fprintf (stderr, "Unable to attach an IOProc to the selected audio unit.\n");
		return false;
	}
	
	UInt32 framesize = PREFERRED_FRAMESIZE;

	if (AudioUnitSetProperty(output_instance, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0, &framesize, sizeof(UInt32))) {
		fprintf(stderr, "SetProperty for kAudioDevicePropertyBufferFrameSize failed :(\n");
		return false;

	}

	if (AudioOutputUnitStart (output_instance)) {
		fprintf (stderr, "Unable to start audio unit.\n");
		return false;
	}

	return true;
}

static void close_audio (void) {
	AudioOutputUnitStop (output_instance);
}

int init_output() {
	if (!init()) return 0;

	AURenderCallbackStruct cb;
	memset(&cb, 0, sizeof(cb));

	cb.inputProc = rcallback;
	cb.inputProcRefCon = NULL;

	if (!open_audio(FMT_S16_LE, 44100, 1, &cb)) return 0;

	return 1;
}


//int main(int argc, char* argv[]) {
//
//	CFRunLoopRef runLoop;
//	runLoop = CFRunLoopGetCurrent();
//	CFRunLoopRun();
//}
//
