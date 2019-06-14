#include <stdio.h>
#include <stdlib.h>
#include "samples.h"

sample_t alloc_sample(size_t filesize, int num_channels) {
    sample_t s;
    s.num_channels = num_channels;
    s.samples = malloc((filesize/sizeof(short)) * sizeof(short) + 16); // :DD
    s.num_samples = filesize/s.num_channels;

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

sample_t *get_sample(sound_t *s, int index) {
    return &s->samples[index];
}
