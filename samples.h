#ifndef SAMPLES_H
#define SAMPLES_H

enum {
    SAMPLE_TYPE_RAW,
    SAMPLE_TYPE_WAV,
};

typedef struct sample_t {
    short *samples;
    unsigned int num_samples;
    int num_channels;
} sample_t;

typedef struct sound_t {
    sample_t *samples;
    unsigned int num_samples;
} sound_t;


sample_t alloc_sample(size_t filesize, int num_channels);

// num_channels is used for raw
sound_t *load_sound(int sample_type, const char* filename_prefix, int num_channels); 

sample_t *get_sample(sound_t *s, int index);


#endif
