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

#include "winlite.h"


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

const char *UTIL_GetFileExtension( const char *path )
{
	const char *src;

	src = path + strlen( path ) - 1;

	while ( src != path && *( src - 1 ) != '.' )
		src--;

	if ( src == path || PATHSEPARATOR( *src ) )
	{
		return NULL;
	}

	return src;
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

OJSndFile_t *CreateSndFile( FILE *file, size_t unWhere )
{
	OJSndFile_t *pSndFile = malloc( sizeof( OJSndFile_t ) );

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

	// TODO: 
	fread( &pSndFile->unknown1, 1, 4, file );
	fseek( file, 4, SEEK_CUR );

	// Sample rate
	// Works in a weird way, if sample rate IS NOT 44100, have additional block for it.
	// Yet it still keeps the 44100 block
	int sampleRate;
	fread( &sampleRate, 1, 4, file );
	if ( sampleRate < 44100 )
	{
		pSndFile->desiredSampleRate = sampleRate;
		fread( &pSndFile->defaultSampleRate, 1, 4, file );
		fseek( file, 6, SEEK_CUR );
	}
	else
	{
		pSndFile->desiredSampleRate = 44100;
		fseek( file, 10, SEEK_CUR );
	}

	// TODO: 

	// Raw PCM wave data, what really matters
	// Seems to be signed 16 bit PCM for voice lines at least
	char *pSndData = malloc( pSndFile->size );
	for ( int i = 0; i < pSndFile->size; i++ )
	{
		fread( &pSndData[i], 1, 1, file );
	}

	pSndFile->data = pSndData;

	//fseek( file, 2, SEEK_CUR );

	return pSndFile;
}

void DestroySndFile( OJSndFile_t *pSndFile )
{
	free( pSndFile->data );
	free( pSndFile->filename );

	free( pSndFile );
}

bool WriteSndFileToWav( OJSndFile_t *pSndFile, const char *filename )
{
	FILE *file = fopen( filename, "wb" );

	if ( !file )
		return false;

	WaveHeader_t header = UTIL_MakeWaveHeader( pSndFile->desiredSampleRate, 1, 16 );
	header.chunkSize = 4 + 8 + 16 + 8 + pSndFile->size;
	header.subChunk2Size = pSndFile->size;

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
		ppSndFiles[sndBlock] = CreateSndFile( file, pos );

		sndBlock++;
	}

	for ( int i = 0; i < sndBlock; i++ )
	{
		printf( "Extracting %s...\n", ppSndFiles[i]->filename );

		char szWavFileName[520];
		snprintf( szWavFileName, sizeof( szWavFileName ), "%s\\%s", destination, ppSndFiles[i]->filename );

		bool success = WriteSndFileToWav( ppSndFiles[i], szWavFileName );
		if ( success )
			printf( "...done!\n" );
		else
			printf( "...FAILED!\n" );

		DestroySndFile( ppSndFiles[i] );
	}

	free( ppSndFiles );

	fclose( file );
	return true;
}

OJSndFile_t *ConvertWavToSndFile( const char *filename )
{
	FILE *file = fopen( filename, "r+b" );

	if ( !file )
		return NULL;

	OJSndFile_t *pSndFile = malloc( sizeof( OJSndFile_t ) );

	size_t fileSize = 0;
	fseek( file, 0, SEEK_END );
	fileSize = ftell( file );
	rewind( file );

	WaveHeader_t wavHeader;
	fread( &wavHeader, 1, sizeof( WaveHeader_t ), file );

	char *pszFileName = malloc( 520 );
	strncpy( pszFileName, UTIL_GetFileName( filename ), 520 );
	pSndFile->filename = pszFileName;

	// Still don't understand this properly
	if ( wavHeader.sampleRate < 44100 )
		pSndFile->desiredSampleRate = wavHeader.sampleRate;
	else
		pSndFile->desiredSampleRate = 44100;

	pSndFile->defaultSampleRate = 44100;

	pSndFile->size = (int)fileSize - ftell( file );

	pSndFile->data = malloc( pSndFile->size );
	for ( int i = 0; i < pSndFile->size; i++ )
	{
		fread( &pSndFile->data[i], 1, 1, file );
	}

	// lol figure these out
	pSndFile->unknown1 = 18;
	pSndFile->unknown2 = 88200;
	pSndFile->unknown3 = 1048578;

	fclose( file );
	return pSndFile;
}

bool PackSndPak( const char *directory, const char *filename )
{
	OJSndFile_t **ppSndFiles = calloc( 0, sizeof( OJSndFile_t* ) );

	char szPath[520];
	snprintf( szPath, sizeof( szPath ), "%s\\*.*", directory );

	// I'm too lazy to install any file system libraries so WinAPI works !FOR NOW!
	WIN32_FIND_DATAA file;
	HANDLE hFind = FindFirstFileExA( szPath, FindExInfoStandard, &file, FindExSearchLimitToDirectories, NULL, 0 );

	if ( hFind == INVALID_HANDLE_VALUE )
	{
		printf( "PackSndPak threw %d, bailing\n", GetLastError() );
		return false;
	}

	int sndBlock = 0;
	while ( FindNextFileA( hFind, &file ) )
	{
		if ( file.cFileName[0] != '.' )
		{
			// If it's not a file we want bail out
			const char *pszExtension = UTIL_GetFileExtension( file.cFileName );
			if ( stricmp( pszExtension, "wav" ) != 0 )
				continue;

			char szFullFileName[520];
			snprintf( szFullFileName, sizeof( szFullFileName ), "%s\\%s", directory, file.cFileName );

			ppSndFiles = realloc( ppSndFiles, ( ( sndBlock + 1 ) * sizeof( OJSndFile_t* ) ) );
			ppSndFiles[sndBlock] = ConvertWavToSndFile( szFullFileName );

			sndBlock++;
		}
	}

	FILE *pFile = fopen( filename, "wb" );

	if ( !pFile )
		return false;

	for ( int i = 0; i < sndBlock; i++ )
	{
		printf( "Packing %s...\n", ppSndFiles[i]->filename );

		fwrite( &ppSndFiles[i]->size, 1, sizeof( int ), pFile );

		for ( int j = 0; j < strlen( ppSndFiles[i]->filename ); j++ )
			fwrite( &ppSndFiles[i]->filename[j], 1, 1, pFile );

		char nul = '\0';
		fwrite( &nul, 1, 1, pFile );

		fwrite( &ppSndFiles[i]->unknown1, 1, sizeof( int ), pFile );

		if ( ppSndFiles[i]->desiredSampleRate < 44100 )
			fwrite( &ppSndFiles[i]->desiredSampleRate, 1, sizeof( int ), pFile );
		else
		{
			char dummy[4] = { 0x01, 0x00, 0x01, 0x00 };
			for ( int j = 0; j < 4; j++ )
				fwrite( &dummy[j], 1, 1, pFile );
		}

		fwrite( &ppSndFiles[i]->defaultSampleRate, 1, sizeof( int ), pFile );
		fwrite( &ppSndFiles[i]->unknown2, 1, sizeof( int ), pFile );
		fwrite( &ppSndFiles[i]->unknown3, 1, sizeof( int ), pFile );

		for ( int j = 0; j <= ppSndFiles[i]->size; j++ )
			fwrite( &ppSndFiles[i]->data[j], 1, 1, pFile );

		fwrite( &nul, 1, 1, pFile );
	}

	fclose( pFile );
	free( ppSndFiles );

	return true;
}

int main( int argc, char *argv[] )
{
	if ( argc < 2 )
	{
		printf( "Usage: ojpak [.pak file]\n" );
		system( "pause" );
		return 0;
	}

	const char *pszFileName = argv[1];
	const char *pszFileExtension = UTIL_GetFileExtension( pszFileName );

	// 0 = unpak, 1 = pak
	int mode;
	if ( pszFileExtension && strcmp( pszFileExtension, "pak" ) == 0 )
		mode = 0; // assume unpak
	else if ( !pszFileExtension )
		mode = 1; // assume pak
	else
	{
		printf( "Invalid input.\n" );
		system( "pause" );
		return 0;
	}

	char szStrippedFileName[520];
	UTIL_StripExtension( UTIL_GetFileName( pszFileName ), szStrippedFileName, sizeof( szStrippedFileName ) );

	if ( mode == 0 )
	{
		printf( "Selected file: %s\n", pszFileName );

		char szPath[520];
		strncpy( szPath, pszFileName, sizeof( szPath ) );
		UTIL_StripFilename( szPath );

		char szDestination[520];
		snprintf( szDestination, sizeof( szDestination ), "%s\\%s_unpacked", szPath, szStrippedFileName );

		_mkdir( szDestination );

		ExtractSndPak( pszFileName, szDestination );
	}
	else if ( mode == 1 )
	{
		printf( "Packing directory: %s\n", pszFileName );

		char szPakName[520];
		snprintf( szPakName, sizeof( szPakName ), "%s\\%s.pak", pszFileName, szStrippedFileName );

		PackSndPak( pszFileName, szPakName );
	}

	printf( "Finished.\n" );
	system( "pause" );
    return 0;
}
