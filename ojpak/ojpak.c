// ojpak.c : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <direct.h>
#include "pakfile.h"
#include "waveheader.h"


//=============================================================================//
// UTIL funcs go here:
//=============================================================================//

// shout-out to http://blog.acipo.com/generating-wave-files-in-c/
WaveHeader_t UTIL_MakeWaveHeader( const int sampleRate, const short numChannels, const short bitsPerSample )
{
	WaveHeader_t myHeader;

	// RIFF WAVE Header
	myHeader.chunkId[0] = 'R';
	myHeader.chunkId[1] = 'I';
	myHeader.chunkId[2] = 'F';
	myHeader.chunkId[3] = 'F';
	myHeader.format[0] = 'W';
	myHeader.format[1] = 'A';
	myHeader.format[2] = 'V';
	myHeader.format[3] = 'E';

	// Format subchunk
	myHeader.subChunk1Id[0] = 'f';
	myHeader.subChunk1Id[1] = 'm';
	myHeader.subChunk1Id[2] = 't';
	myHeader.subChunk1Id[3] = ' ';
	myHeader.audioFormat = 1; // FOR PCM
	myHeader.numChannels = numChannels; // 1 for MONO, 2 for stereo
	myHeader.sampleRate = sampleRate; // ie 44100 hertz, cd quality audio
	myHeader.bitsPerSample = bitsPerSample; // 
	myHeader.byteRate = myHeader.sampleRate * myHeader.numChannels * myHeader.bitsPerSample / 8;
	myHeader.blockAlign = myHeader.numChannels * myHeader.bitsPerSample / 8;

	// Data subchunk
	myHeader.subChunk2Id[0] = 'd';
	myHeader.subChunk2Id[1] = 'a';
	myHeader.subChunk2Id[2] = 't';
	myHeader.subChunk2Id[3] = 'a';

	// All sizes for later:
	// chuckSize = 4 + (8 + subChunk1Size) + (8 + subChubk2Size)
	// subChunk1Size is constanst, i'm using 16 and staying with PCM
	// subChunk2Size = nSamples * nChannels * bitsPerSample/8
	// Whenever a sample is added:
	//    chunkSize += (nChannels * bitsPerSample/8)
	//    subChunk2Size += (nChannels * bitsPerSample/8)
	myHeader.chunkSize = 4 + 8 + 16 + 8 + 0;
	myHeader.subChunk1Size = 16;
	myHeader.subChunk2Size = 0;

	return myHeader;
}

bool PATHSEPARATOR( char c )
{
	return c == '\\' || c == '/';
}

const char *UTIL_GetFileName( const char *in )
{
	const char *out = in + strlen( in ) - 1;
	while ( ( out > in ) && ( !PATHSEPARATOR( *( out - 1 ) ) ) )
		out--;
	return out;
}

void UTIL_StripFilename( char *path )
{
	size_t length;

	length = strlen( path ) - 1;
	if ( length <= 0 )
		return;

	while ( length > 0 &&
		!PATHSEPARATOR( path[length] ) )
	{
		length--;
	}

	path[length] = 0;
}

void UTIL_StripExtension( const char *in, char *out, int outSize )
{
	size_t end = strlen( in ) - 1;
	while ( end > 0 && in[end] != '.' && !PATHSEPARATOR( in[end] ) )
	{
		--end;
	}

	if ( end > 0 && !PATHSEPARATOR( in[end] ) && end < outSize )
	{
		size_t nChars = min( end, outSize - 1 );
		if ( out != in )
		{
			memcpy( out, in, nChars );
		}
		out[nChars] = 0;
	}
	else
	{
		if ( out != in )
		{
			strncpy( out, in, outSize );
		}
	}
}
//=============================================================================//
// UTIL funcs end
//=============================================================================//

OJSndFile_t *GetSndFile( FILE *file, size_t unWhere )
{
	OJSndFile_t *pSndFile = malloc( sizeof( OJSndFile_t ) );
	pSndFile->fileofs = (int)unWhere;

	fseek( file, unWhere, SEEK_SET );

	// First read the 4 bytes, that is the size
	fread( &pSndFile->size, 1, 4, file );

	// Next up is the filename, keep reading until we hit NUL
	const size_t size = 256;
	char *pszFileName = malloc( size );
	for ( size_t i = 0; i < size; i++ )
	{
		if ( i != 0 && pszFileName[i - 1] == '\0' )
			break;

		fread( &pszFileName[i], 1, 1, file );
	}

	pSndFile->filename = pszFileName;

	// TODO: Jump over next 8 bytes, too lazy to figure out
	fseek( file, ftell( file ) + 8, SEEK_SET );

	// Sample rate
	fread( &pSndFile->samplerate, 1, 4, file );

	// TODO: Jump over next 8 bytes, too lazy to figure out
	fseek( file, ftell( file ) + 8, SEEK_SET );

	// Raw PCM wave data, what really matters
	// Seems to be signed 16 bit PCM for voice lines at least
	char *pSndData = malloc( pSndFile->size );
	for ( int i = 0; i < pSndFile->size; i++ )
	{
		fread( &pSndData[i], 1, 1, file );
	}

	pSndFile->data = pSndData;

	fseek( file, ftell( file ) + 2, SEEK_SET );

	return pSndFile;
}

bool WriteSndFileToWav( OJSndFile_t *pSndFile, const char *filename )
{
	FILE *file = fopen( filename, "wb" );

	if ( !file )
		return false;

	WaveHeader_t header = UTIL_MakeWaveHeader( pSndFile->samplerate, 1, 16 );

	// Write header & data
	fwrite( &( header ), sizeof( WaveHeader_t ), 1, file );
	for ( int i = 0; i < pSndFile->size; i++ )
		fwrite( &pSndFile->data[i], 1, 1, file );

	fclose( file );
	return true;
}

bool ExtractSndPak( const char *filename, const char *destination )
{
	FILE *file = fopen( filename, "r+b" );

	if ( !file )
		return false;

	printf( "Unpacking...\n" );

	size_t fileSize = 0;
	fseek( file, 0, SEEK_END );
	fileSize = ftell( file );
	rewind( file );

	OJSndFile_t **ppSndFiles = calloc( 0, sizeof( OJSndFile_t* ) );

	size_t pos = 0;
	int sndBlock = 0;
	while ( 1 )
	{
		pos = ftell( file );

		if ( pos >= fileSize )
			break;

		ppSndFiles = realloc( ppSndFiles, ( ( sndBlock + 1 ) * sizeof( OJSndFile_t* ) ) );
		ppSndFiles[sndBlock] = GetSndFile( file, pos );

		sndBlock++;
	}

	for ( int i = 0; i < sndBlock; i++ )
	{
		//printf( "filename: %s\nfileofs: %d\nsize: %d\nsamplerate: %d\n\n", ppSndFiles[i]->filename, ppSndFiles[i]->fileofs, ppSndFiles[i]->size, ppSndFiles[i]->samplerate );

		printf( "Extracting %s...\n", ppSndFiles[i]->filename );

		char szWavFileName[520];
		snprintf( szWavFileName, sizeof( szWavFileName ), "%s\\%s", destination, ppSndFiles[i]->filename );

		bool success = WriteSndFileToWav( ppSndFiles[i], szWavFileName );
		if ( success )
			printf( "...done!\n" );
		else
			printf( "...FAILED!\n" );
	}

	fclose( file );
	return true;
}

int main( int argc, char *argv[] )
{
	if ( argc < 2 )
	{
		printf( "Usage: ojpak [.pak file]" );
		return 0;
	}

	const char *pszFileName = argv[1];

	printf( "Selected file: %s\n", pszFileName );

	char szPath[520];
	strncpy( szPath, pszFileName, sizeof( szPath ) );
	UTIL_StripFilename( szPath );

	char szStrippedFileName[520];
	UTIL_StripExtension( UTIL_GetFileName( pszFileName ), szStrippedFileName, sizeof( szStrippedFileName ) );

	char szDestination[520];
	snprintf( szDestination, sizeof( szDestination ), "%s\\%s_unpacked", szPath, szStrippedFileName );

	_mkdir( szDestination );

	ExtractSndPak( pszFileName, szDestination );
    return 0;
}
