#ifndef PAKFILE_H
#define PAKFILE_H

typedef struct OJSndFile_t
{
	int fileofs;
	int size;
	const char *filename;
	int samplerate;
	char *data;
} OJSndFile_t;

#endif
