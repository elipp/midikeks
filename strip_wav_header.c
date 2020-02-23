#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

long getfilesize(FILE *fp) {
	fseek(fp, 0L, SEEK_END);
	long sz = ftell(fp);
	rewind(fp);
	return sz;
}

void get_stripped_filename(const char* filename, char *buffer) {
	sprintf(buffer, "%s_raw", filename);
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("No input files, exiting\n");
		return 0;
	}

	const char* filename = argv[1];
	printf("stripping WAV header from file %s\n", filename);

	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		printf("Unable to open input file %s, exiting\n", filename);
		return 1;
	}

	long fs = getfilesize(fp) - 44;
	unsigned char *buf = malloc(fs);
	
	// strip first 44 bytes of file, we assume a certain format :D
	fseek(fp, 44, SEEK_SET);
	fread(buf, 1, fs, fp);

	fclose(fp);

	char fnbuf[256];
	fnbuf[0] = '\0';
	get_stripped_filename(filename, fnbuf);

	FILE *fpo = fopen(fnbuf, "wb");
	if (!fpo) {
		printf("unable to open output file %s: %s\n", fnbuf, strerror(errno));
		return 1;
	}

	fwrite(buf, 1, fs, fpo);
	free(buf);

	printf("Successfully stripped WAV header from file %s, output filename: %s\n", filename, fnbuf);

	return 0;
}
