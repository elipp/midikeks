#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>

#include "midi.h"
#include "output.h"
#include "input.h"

//typedef OSStatus (*AURenderCallback)(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);


const SAMPLETYPE SHORT_MAX = 32767.0;
const SAMPLETYPE SHORT_MAX_I = 1.0/SHORT_MAX;

const double eqtemp_12 = 1.0594630943592953;
double eqtemp_factor = eqtemp_12;

const double TWOPI = 6.2831853071795865;

static double eqtemp_hz[128];

void init_eqtemp_hztable() {
	for (int i = 21; i < 128; ++i) {
		eqtemp_hz[i] = 27.5*pow(eqtemp_factor, (i-21));
		keys[i].hz = eqtemp_hz[i];
	}

}

static const double GLOBAL_DT = 1.0/44100.0;

static inline double waveform_sine(double freq, double t, double phi) {
	return sin(freq * TWOPI * t + phi);
}

static double waveform_triangle(double freq, double t, double phi) {
	float p = (TWOPI*freq*t + phi)/TWOPI;
	float phase = p - floor(p);
	return phase < 0.5 ? (-4*phase + 1) : (4*phase - 3);
}

static inline double limit(double d, double limit_abs) {
	if (d > limit_abs) {
		d = limit_abs;
	}
	if (d < -limit_abs) {
		d = -limit_abs;
	}

	return d;
}

static inline double waveform_sine_limit(double freq, double t, double phi, double limit_abs) {
	double h = (27.5/freq);
	double h2 = 0.5*h;
	double d = sin(freq * TWOPI * t + phi)  
		- h * sin(2*freq*TWOPI*t + phi)
		+ h * sin(3*freq*TWOPI*t + phi)
		- h2 * waveform_triangle(4*freq, t, phi);

	return limit(d, limit_abs);
}

static double *sine_precalculated;

#define PRECALC_SINE_RESOLUTION (2*1024) // this is plenty. sounds pretty ok with 1024 :D

static double *precalculate_sinusoid() {
	double* w = malloc(PRECALC_SINE_RESOLUTION*sizeof(double));
	double dp = TWOPI / (double)PRECALC_SINE_RESOLUTION;
	
	for (int i = 0; i < PRECALC_SINE_RESOLUTION; ++i) {
		w[i] = sin(dp*i);
	}

	return w;
}

static double pcsin(double s) {
	double m = fmod(s, TWOPI)/TWOPI;
	int index = m * (double)(PRECALC_SINE_RESOLUTION-1); // will get 'floored', but doesn't matter probably

	return sine_precalculated[index];
}

static double pcwaveform_sine_limit(double freq, double t, double phi, double limit_abs) {
	double h = (27.5/freq);
	double h2 = 0.5*h;
	double d = pcsin(freq * TWOPI * t + phi)  
		- h * pcsin(2*freq*TWOPI*t + phi)
		+ h * pcsin(3*freq*TWOPI*t + phi)
		- h2 * waveform_triangle(4*freq, t, phi);

	return limit(d, limit_abs)/limit_abs;

}

static double pcwaveform_synthpiano(double freq, double t, double phi) {
	double T = freq*TWOPI*t;
	double d = pcsin(1*T + phi) +
		   0.75*pcsin(2*T + phi) -
		   0.20*pcsin(3*T + phi) +
		   0.23*pcsin(4*T + phi) -
		   0.02*pcsin(5*T + phi) +
		   0.03*pcsin(6*T + phi) -
		   0.003*pcsin(7*T + phi) +
		   0.003*pcsin(8*T + phi) -
		   0.006*pcsin(9*T + phi);

	return d;

}

double midikey_to_hz(int index) {
	// key 21 maps to lowest A (27.5 Hz @ A=440Hz)
	// key 108 maps to highest C
   // return 27.5*pow(eqtemp_factor, (index-21));

	return eqtemp_hz[index];
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

#define DUMPSAMPLES (44100 * 30)

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

static inline SAMPLETYPE clamp(SAMPLETYPE in, SAMPLETYPE min, SAMPLETYPE max) {
    if (in > max) return max;
    if (in < min) return min;
    return in;
}

static OSStatus rcallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
	//printf("%f\n", inTimeStamp->mSampleTime);

	AudioBuffer *b = &ioData->mBuffers[0];

	short *samples = (short*)b->mData;
//	memset(samples, 0, b->mDataByteSize);

	int num_keys = 0;

    static SAMPLETYPE fbuf[1024*2]; // the buffer is most likely never going to be over 32 samples (because of delay) so this should be fine
    memset(fbuf, 0, sizeof(fbuf));

    static short delaybuf[1024*2];

//	float *input = get_input_data();
//
//	for (int m = 20; m < 109; ++m) {
//		struct MIDIkey_t *k = &keys[m];
//		if (k->A > 0.001) {
//#ifdef DUMP
//			start_dumping();
//#endif
//			for (int i = 0; i < PREFERRED_FRAMESIZE; ++i) {
//				//double tv = 0.0003*sin(5*TWOPI*t);
//				//samples[i] += A * k->A * waveform_sine_limit(k->hz, k->t, 0, 0.5);
////				samples[i] += A * k->A * pcwaveform_sine_limit(k->hz, k->t, 0, 0.5);
//				samples[i] += 0.3 * A * k->A * pcwaveform_synthpiano(k->hz, k->t, 0);
//	//			samples[i] += 0.3 * A * k->A * input[i];
//				k->t += GLOBAL_DT;
//			}
//			
//			k->phase = k->hz * k->t * TWOPI + 0;
//
//			if (k->pressed || sustain_pedal_down) {
//				k->A *= 0.9995;
//			} else {
//				k->A *= 0.99;
//			}
//			++num_keys;
//		}
//        else {
//            k->t = 0;
//            k->phase = 0;
//        }
//
//	}

    int NUM_FRAMES = b->mDataByteSize/sizeof(short)/b->mNumberChannels;

    if (b->mNumberChannels == 1) {
        for (int m = 0; m < mqueue.num_events; ++m) {
            mevent_t *e = &mqueue.events[m];
            for (int i = 0; i < NUM_FRAMES; ++i) {
                samples[i] += 0.3 * SHORT_MAX * e->A * 
                   pcwaveform_sine_limit(e->hz, e->t, 0, modulation);
                    //pcwaveform_synthpiano(e->hz, e->t, 0);
                e->t += GLOBAL_DT;
                }
            e->phase = e->hz * e->t * TWOPI;
        }
    }

    else if (b->mNumberChannels == 2) {
        // the data is expected to be in an interleaved arrangement
        for (int m = 0; m < mqueue.num_events; ++m) {
            mevent_t *e = &mqueue.events[m];
            if (e->sample) {
	    	if (e->sample_index >= e->sample->num_frames - 1) {
			e->A = 0;
		}
		else {
			for (int i = 0; i < NUM_FRAMES; ++i) {
				// e->A for samples has a separate block :)
				SAMPLETYPE fA = 0.33 * e->A; 

				SAMPLETYPE Lval = fA * e->sample->samples[e->sample_index];
				SAMPLETYPE Rval = fA * e->sample->samples[e->sample_index+1];

				fbuf[2*i] += Lval;
				fbuf[2*i + 1] += Rval;

				e->t += GLOBAL_DT;
				e->sample_index += 2;
				if (!keys[e->keyindex].pressed) {
					//e->A *= exp(-0.0008*e->t);
					e->A *= 0.9995;
				}
			}
		}
            }
            else {
                for (int i = 0; i < NUM_FRAMES; ++i) {
                    SAMPLETYPE val = 0.5 * e->A * 
                    pcwaveform_sine_limit(e->hz, e->t, 0, modulation);
                    
                    fbuf[2*i] += val;
                    fbuf[2*i+1] += val;

                    e->t += GLOBAL_DT;

                    if (keys[e->keyindex].pressed) {
                        e->A *= 0.99998;
                    }
                    else  {
                        e->A *= 0.999;
                    }
                }
            }
            e->phase = e->hz * e->t * TWOPI;
        }

        for (int i = 0; i < b->mDataByteSize/sizeof(short)/b->mNumberChannels; ++i) {
            samples[2*i] = SHORT_MAX * (clamp(fbuf[2*i], -1.0, 1.0));
            samples[2*i + 1] = SHORT_MAX * (clamp(fbuf[2*i + 1], -1.0, 1.0));
        }

    //    memcpy(delaybuf, samples, b->mDataByteSize);

    }

    mqueue_purge(&mqueue);

#ifdef DUMP
	if (get_current_dumpoffset() + PREFERRED_FRAMESIZE >= DUMPSAMPLES) stop_dumping();
	if (dumping == 1) append_to_dump(samples, PREFERRED_FRAMESIZE);
#endif

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
static AudioComponent input_comp;
static AudioComponentInstance input_instance;


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

	sine_precalculated = precalculate_sinusoid();

	AURenderCallbackStruct cb;
	memset(&cb, 0, sizeof(cb));

	cb.inputProc = rcallback;
	cb.inputProcRefCon = NULL;

	if (!open_audio(FMT_S16_LE, 44100, 2, &cb)) return 0;

	return 1;
}


