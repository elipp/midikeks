#ifndef WAV_H
#define WAV_H

typedef struct WAV_hdr_t {
	int ChunkID_BE;
	int ChunkSize_LE;
	int Format_BE;
	int Subchunk1ID_BE;
	int Subchunk1Size_LE;
	short AudioFormat_LE;
	short NumChannels_LE;
	int SampleRate_LE;
	int ByteRate_LE;
	short BlockAlign_LE;
	short BitsPerSample_LE;
	int Subchunk2ID_BE;
	int Subchunk2Size_LE;
} WAV_hdr_t;


double *read_WAV(const char* filename);
int write_WAV(const char* filename, short *samples, long num_samples);

#endif
