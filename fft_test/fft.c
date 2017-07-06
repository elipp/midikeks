#include <stdio.h>
#include <stdlib.h>
#include <fftw3.h>

#include "WAV.h"

#define FFT_N (1024*512)

int do_fft(double *in) {

	fftw_complex *out;
	fftw_plan my_plan;

	out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*FFT_N);
	my_plan = fftw_plan_dft_r2c_1d(FFT_N, in, out, FFTW_ESTIMATE);
	fftw_execute(my_plan);
	fftw_destroy_plan(my_plan);
	
	int rising = 0;
	double prev_magn = 0;

	struct
	int *peaks = malloc(FFT_N*sizeof(int));
	int num_peaks = 0;

	for (int i = 1; i < FFT_N; ++i) { // bin 0 is apparently something else
		double r = out[i][0];
		double c = out[i][1];

		double M = r*r + c*c;

		if (M > prev_magn) {
			rising = 1;
		} else if (rising) {
			rising = 0;
			peaks[num_peaks] = i;
			++num_peaks;
		}

		prev_magn = M;
	}

	for (int i = 0; i < num_peaks; ++i) {
		double peakhz = peaks[i]*(44100.0/(double)FFT_N);
		printf("peak at %.5f Hz\n", peakhz);
	}

	fftw_free(out);
	free(peaks);

	return 1;

}


int main(int argc, char *argv[]) {
	
	double *samples = read_WAV("a.wav");

	if (!samples) return 1;

	do_fft(samples);

	return 0;
}
