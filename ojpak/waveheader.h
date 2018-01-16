#ifndef WAVFILE_H
#define WAVFILE_H

//
// shout-out to http://blog.acipo.com/generating-wave-files-in-c/
//

typedef struct WaveHeader_t
{
	// Riff Wave Header
	char chunkId[4];
	int  chunkSize;
	char format[4];

	// Format Subchunk
	char subChunk1Id[4];
	int  subChunk1Size;
	short int audioFormat;
	short int numChannels;
	int sampleRate;
	int byteRate;
	short int blockAlign;
	short int bitsPerSample;
	//short int extraParamSize;

	// Data Subchunk
	char subChunk2Id[4];
	int  subChunk2Size;

} WaveHeader_t;

#endif
