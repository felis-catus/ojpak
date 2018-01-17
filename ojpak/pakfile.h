#ifndef PAKFILE_H
#define PAKFILE_H

typedef struct OJSndFile_t
{
	int size;
	char *filename;
	int unknown1;
	int desiredSampleRate;
	int defaultSampleRate;
	int unknown2;
	int unknown3;
	char *data;
} OJSndFile_t;

#endif
