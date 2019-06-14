#ifndef HARMONY_H
#define HARMONY_H

enum EXTENSIONS {
   E_1 = 0,
   E_flat9 = 1,
   E_9 = 2,
   E_sharp9 = 3,
   E_min3 = 3,
   E_maj3 = 4,
   E_11 = 5,
   E_4 = 5,
   E_sharp4 = 6, 
   E_sharp11 = 6, 
   E_flat5 = 6,
   E_5 = 7,
   E_sharp5 = 8,
   E_flat13 = 8,
   E_6 = 9,
   E_13 = 9,
   E_min7 = 10,
   E_maj7 = 11,
   E_SKIP = 0xfe, // skip an octave
   E_END = 0xff,
};

typedef struct voicing_t {
    enum EXTENSIONS members[16]; // 16-note voicings XD
    int pitches[16];
} voicing_t;

void init_voicings();
const voicing_t *get_voicing(int bass_melody_distance);

#endif
