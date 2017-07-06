#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "WAV.h"


static inline int eswap_s32(int n) {
	return ((n>>24)&0x000000FF) |
		((n>>8)&0x0000FF00) |
		((n<<8)&0x00FF0000) |
		((n<<24)&0xFF000000);
}

static inline short eswap_s16(short n) {
	return (n>>8) | (n<<8);
}

// for whatever reason, these are in big endian form

static const int WAV_hdr_RIFF = 0x46464952; 
//static const int WAV_hdr_RIFF = eswap_s32(0x52494646); // "RIFF"

static const int WAV_hdr_WAVE = 0x45564157;
//static const int WAV_hdr_WAVE = eswap_s32(0x57415645); // "WAVE"

static const int WAV_hdr_fmt = 0x20746d66;
//static const int WAV_hdr_fmt = eswap_s32(0x666d7420); // "fmt "

static const int WAV_hdr_data = 0x61746164;
//static const int WAV_hdr_data = eswap_s32(0x64617461); // "data"

static long get_filesize_rewind(FILE *fp) {
	fseek(fp, 0, SEEK_END);
	long o = ftell(fp);
	rewind(fp);
	return o;
}

double *read_WAV(const char* filename) { 

	FILE *fp = fopen(filename, "r");
	if (!fp) { 
		fprintf(stderr, "couldn't open file \"%s\": %s\n", filename, strerror(errno));
		return NULL; 
	}

	size_t sz = get_filesize_rewind(fp);
	WAV_hdr_t wh;

	long sbuf_bytes = sz-sizeof(WAV_hdr_t);
	long num_samples = (sbuf_bytes)/sizeof(short);

	short *buf = malloc(num_samples * sizeof(short));

	fread(&wh, sizeof(WAV_hdr_t), 1, fp);

	fread(buf, sizeof(short), num_samples, fp);
	fclose(fp);

	double *b = malloc(num_samples * sizeof(double));

	for (int i = 0; i < num_samples; ++i) {
		b[i] = (double)buf[i]/(double)SHRT_MAX;
	}

	free(buf);

	return b;
}
