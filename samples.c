#include <stdio.h>
#include <stdlib.h>
#include "samples.h"
#include "output.h"

sample_t alloc_sample(size_t filesize, int num_channels) {
    sample_t s;
    s.num_channels = num_channels;

    int num_samples = filesize/sizeof(short);

    s.samples = malloc((num_samples + 16) * sizeof(short)); // :DD
    s.num_frames = num_samples/s.num_channels;

    return s;
}

fsample_t alloc_fsample(size_t num_frames, int num_channels) {
    fsample_t s;
    s.num_channels = num_channels;

    // explanation for the following: the "raw" file format is 16-bit signed short, so num samples is filesize/sizeof(short)
    int num_samples = num_frames * num_channels;
    s.samples = malloc((num_samples + 16) * sizeof(SAMPLETYPE));  
    
    s.num_frames = num_frames;

    return s;
}


long get_filesize(FILE *fp) {
    fseek(fp, 0L, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);
    
    return sz;
}

// the format must be something like "wavs/piano_%d_normal.wav"
sound_t *load_sound(int sample_type, const char* filename_format, int num_channels) {
    char filename_buffer[256];
    sound_t *s = malloc(sizeof(sound_t));
    s->samples = malloc(89*sizeof(sample_t));

    for (int i = 0; i < 89; ++i) {
        sprintf(filename_buffer, filename_format, i);
        FILE* fp = fopen(filename_buffer, "rb");
        if (!fp) {
            printf("warning: couldn't open file %s\n", filename_buffer);
        }
        else {
            long sz = get_filesize(fp);
            s->samples[i] = alloc_sample(sz, 2);
            fread(s->samples[i].samples, 1, sz, fp);
            fclose(fp);
        }
    }

    return s;
}

static void free_sound(sound_t *s) {
    for (int i = 0; i < 89; ++i) {
        free(s->samples[i].samples);
    }

    free(s->samples);
    free(s);
}

fsound_t *load_fsound(int sample_type, const char* filename_format, int num_channels) {
    sound_t *s = load_sound(sample_type, filename_format, num_channels);

    fsound_t *fs = malloc(sizeof(fsound_t));
    fs->samples = malloc(89*sizeof(fsample_t));

    for (int i = 0; i < 89; ++i) {

        int num_frames = s->samples[i].num_frames;
        fs->samples[i] = alloc_fsample(num_frames, 2);

        SAMPLETYPE *D = fs->samples[i].samples;
        short *S = s->samples[i].samples;

        for (int j = 0; j < num_frames; ++j) {
            D[2*j] = SHORT_MAX_I * (SAMPLETYPE)S[2*j];
            D[2*j + 1] = SHORT_MAX_I * (SAMPLETYPE)S[2*j + 1];
        }
    }

    free_sound(s);
    
    return fs;
}

sample_t *get_sample(sound_t *s, int index) {
    if (index < 21) return NULL;
    return &s->samples[index-21];
}

fsample_t *get_fsample(fsound_t *s, int index) {
    if (index < 21) return NULL;
    return &s->samples[index-21];
}
