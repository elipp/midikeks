#ifndef OUTPUT_H
#define OUTPUT_H

#define PREFERRED_FRAMESIZE 16

int init_output();
double midikey_to_hz(int index);
void init_eqtemp_hztable();

extern const double TWOPI;

extern const double eqtemp_12;
extern double eqtemp_factor;

typedef double SAMPLETYPE;

extern const SAMPLETYPE SHORT_MAX;
extern const SAMPLETYPE SHORT_MAX_I;


#endif
