#ifndef OUTPUT_H
#define OUTPUT_H

int init_output();
double midikey_to_hz(int index);
void init_eqtemp_hztable();

extern const double TWOPI;

extern const double eqtemp_12;
extern double eqtemp_factor;


#endif
