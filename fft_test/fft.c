#include <stdio.h>
#include <stdlib.h>
#include <fftw3.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "WAV.h"

#define FFT_N (1024*64)

static fftw_complex *convert_double_to_complex(double *in) {
	fftw_complex *c = malloc(FFT_N*sizeof(fftw_complex));

	for (int i = 0; i < FFT_N; ++i) {
		c[i][0] = in[i];
		c[i][1] = 0;
	}

	return c;
}

static inline double magn(fftw_complex c) {
	return c[0] * c[0] + c[1] * c[1];
}

static double find_highest_M(fftw_complex *c) {

	double hm = 0;
	for (int i = 1; i < FFT_N/2; ++i) {
		double M = magn(c[i]);
		if (M > hm) hm = M;
	}

	return hm;

}

int do_fft(double *in, const char* output_filename) {

	fftw_complex *out_r2c;
	fftw_plan plan_r2c;

	out_r2c = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*FFT_N);

//	plan_c2c = fftw_plan_dft_1d(FFT_N, inc, out_c2c, FFTW_FORWARD, FFTW_ESTIMATE);
	plan_r2c = fftw_plan_dft_r2c_1d(FFT_N, in, out_r2c, FFTW_ESTIMATE);

	fftw_execute(plan_r2c);
	fftw_destroy_plan(plan_r2c);
	
	int rising = 0;
	double prev_magn = 0;

	int *peaks = malloc((FFT_N/2)*sizeof(int));
	memset(peaks, 0x0, (FFT_N/2)*sizeof(int));
	int num_peaks = 0;

	double HM = find_highest_M(out_r2c);

	for (int i = 1; i < FFT_N/2; ++i) {

		double M = magn(out_r2c[i]);

		if (M > prev_magn) {
			rising = 1;
		} else if (rising) {
			rising = 0;
			peaks[num_peaks] = i;

			++num_peaks;
		}
		prev_magn = M;

	}

	long num_samples = 5*44100;
	short *output = malloc(num_samples*sizeof(short));
	memset(output, 0x0, num_samples*sizeof(short));
	static const double GLOBAL_DT = 1.0/44100.0;
	for (int i = 1; i < FFT_N/2; ++i) {
//		int pind = peaks[i];
		int pind = i;

		double M = magn(out_r2c[pind]);

		if (pind == 0) break;
		M /= HM;

		if (M > 0.0005) {
			double t = 0;
			double peakhz = pind*(44100.0/(double)FFT_N);
			printf("%f Hz: M = %f\n", peakhz, M);
			for (int j = 0; j < num_samples; ++j) {
				output[j] += 0.05* M *(double)SHRT_MAX * sin(2*M_PI*peakhz*t);
				t += GLOBAL_DT;
			}
		}
	}

	fftw_free(out_r2c);
	free(peaks);
	
	write_WAV(output_filename, output, num_samples);

	free(output);

	return 1;

}


int main(int argc, char *argv[]) {

	if (argc < 2) return 0;
	const char* filename = argv[1];

	char outfile[128];
	memset(outfile, 0, sizeof(outfile));

	strcat(outfile, "./out/");
	strcat(outfile, filename);
	
	double *samples = read_WAV(argv[1]);

	if (!samples) return 1;

	do_fft(samples, outfile);

	return 0;
}
