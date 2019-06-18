#include <stdio.h>
#include <stdlib.h>

#include "harmony.h"

static voicing_t voicings[] = {
    {{ E_maj3, E_6, E_9, E_5, E_maj7, E_maj3 , E_END}},
    {{ E_4, E_min7, E_min3, E_5, E_1, E_11 , E_END}},
    {{ E_min3, E_5, E_1, E_11, E_min7, E_min3 , E_END}},
    {{ E_1, E_maj3, E_6, E_9, E_5, E_maj7 , E_END}},
    {{ E_min7, E_min3, E_5, E_1, E_11, E_6 , E_END}},
    {{ E_min3, E_5, E_1, E_11, E_min7, E_9 , E_END}},
    {{ E_min7, E_maj3, E_13, E_9, E_5, E_1 , E_END}},
    {{ E_1, E_min7, E_maj3, E_13, E_9 , E_END}},
    {{ E_min7, E_maj3, E_13, E_9, E_sharp11, E_13 , E_END}},
    {{ E_maj3, E_min7, E_9, E_sharp11, E_13, E_9 , E_END}},
    {{ E_maj3, E_min7, E_sharp9, E_flat13, E_1, E_sharp9 , E_END}},
    {{ E_min7, E_maj3, E_flat13, E_min3, E_flat13, E_1 , E_END}},
    {{ E_1, E_5, E_9, E_min3, E_min7, E_11, E_END}},
    {{ E_1, E_5, E_9, E_min3, E_11, E_min7, E_END}},
    {{ E_1, E_5, E_9, E_maj3, E_maj7, E_sharp11, E_END}},
    {{ E_maj3, E_1, E_9, E_5, E_maj7 , E_END}},
    {{ E_flat9, E_5, E_1, E_maj3, E_5, E_END}},
    {{ E_1, E_maj3, E_min7, E_9, E_sharp11, E_13, E_END}},
    {{ E_1, E_5, E_min3, E_flat5, E_maj7, E_9, E_END}},
    {{ E_min3, E_5, E_1, E_11, E_min7, E_9 , E_END}},
    {{ E_1, E_min7, E_9, E_maj3, E_sharp11, E_13, E_END}},
    {{ E_1, E_6, E_min3, E_5, E_maj7, E_END }},
    {{ E_1, E_6, E_min3, E_5, E_9, E_END }},
    {{ E_1, E_SKIP, E_maj3, E_min7, E_sharp9, E_flat5, E_END}},
    {{ E_1, E_min7, E_maj3, E_sharp5, E_sharp9, E_END }},
    {{ E_1, E_6, E_min3, E_flat5, E_9, E_END }},
    {{ E_1, E_min7, E_min3, E_9, E_END }},
    {{ E_maj3, E_1, E_9, E_5, E_1, E_END }},
    {{ E_1, E_maj3, E_6, E_9, E_5, E_END }},
    {{ E_1, E_min7, E_9, E_min3, E_5, E_END }},
    {{ E_1, E_min7, E_9, E_min3, E_5, E_min7, E_END }},
    {{ E_1, E_5, E_min3, E_5, E_min7, E_9, E_END }},


    //{{ E_1, E_5, E_min7, E_min3, E_11 , E_END}}, // this one needs to be played pretty low on the kb to sound good
    
    // all the voicings that have same bottom and top note
    //{{ E_1, E_maj3, E_min7, E_9, E_5, E_1 , E_END}},
    //{{ E_5, E_1, E_maj3, E_6, E_9, E_5 , E_END}},
//    {{ E_1, E_maj3, E_6, E_9, E_5, E_1 , E_END}},
//    {{ E_5, E_maj7, E_maj3, E_6, E_9, E_5 , E_END}},
//    {{ E_maj3, E_6, E_9, E_5, E_1, E_maj3 , E_END}},
//    {{ E_5, E_1, E_11, E_min7, E_min3, E_5 , E_END}},
//    {{ E_min7, E_min3, E_5, E_1, E_11, E_min7 , E_END}},
    //{{ E_1, E_maj3, E_min7, E_maj3, E_13, E_1 , E_END}},
//    {{ E_1, E_11, E_min7, E_min3, E_5, E_1 , E_END}},
 //   {{ E_5, E_1, E_11, E_6, E_9, E_5 , E_END}},
  //  {{ E_maj3, E_min7, E_9, E_5, E_1, E_maj3 , E_END}},
   // {{ E_min7, E_maj3, E_flat13, E_flat9, E_flat5, E_min7 , E_END}},

//    {{E_maj3, E_6, E_9, E_END }},
//    {{E_maj3, E_5, E_maj7, E_9, E_END}},
//    {{E_maj3, E_13, E_min7, E_9, E_END}},
//    {{E_min7, E_9, E_maj3, E_13, E_END}},
//    {{E_min7, E_flat9, E_maj3, E_13 , E_END}},
//    {{E_maj3, E_sharp5, E_min7, E_sharp9 , E_END}},
//    {{E_min3, E_5, E_min7, E_9 , E_END}},
//    {{E_min7, E_9, E_min3, E_5 , E_END}},
//    {{E_min3, E_5, E_6, E_9 , E_END}},
//    {{E_min3, E_5, E_maj7, E_9 , E_END}},
//    {{E_1, E_11, E_flat5, E_min7 , E_END}},
//    {{E_flat5, E_min7, E_1, E_11 , E_END}},
//    {{E_11, E_flat5, E_min7, E_9 , E_END}},
//    {{E_4, E_min7, E_9, E_END }}
//
};

void init_voicings() {
    for (int i = 0; i < sizeof(voicings)/sizeof(voicings[0]); ++i) {
        voicing_t *v = &voicings[i];
        int base = 0;
        int first = v->members[0];
        int prev = first;

        for (int j = 0; j < 16; ++j) {
            int m = v->members[j];
            if (m == E_END) { break; }
            else if (m == E_SKIP) { ++base; }
            else {

                //          printf("m+base*12 = %d, prev = %d (first = %d)\n", m + base*12, prev, first);
                while ((m + base*12 - first) < prev) {
                    ++base;
                }

                v->pitches[j] = m + base*12 - first;
                prev = v->pitches[j];
                //           printf("voicing %d, member %d = %d, base: %d => %d\n", i, j, m, base, v->pitches[j]);
            }
        }   

//        printf("----------\n");
    }

}

const voicing_t *get_voicing(int bass_melody_distance) {
    // ignore bass melody distance for now
    int r = rand() % (sizeof(voicings)/sizeof(voicings[0]));
    //printf("returning voicing %d\n", r);
    return &voicings[r];
}

