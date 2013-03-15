/*
 * utilities.h
 *
 *  Created on: Mar 14, 2013
 *
 */

#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <stdio.h>
#include <assert.h>
#include "altera_up_avalon_rs232.h"
#include "altera_up_sd_card_avalon_interface.h"
#include "altera_up_avalon_audio_and_video_config.h"
#include "altera_up_avalon_audio.h"
#include "sys/alt_irq.h"
#include <string.h>
#include "system.h"


//typedef int bool;
#define true 1
#define false 0

// CONSTANTS DECLARATION

#define MAX_SONGS_ALLOWED 100

// DATA STRUCTURE

struct pList {
	unsigned short int numOfSongs;
	unsigned short int list[MAX_SONGS_ALLOWED]; 	// limit to a maximum number of songs allowed for now...
	unsigned short int order[MAX_SONGS_ALLOWED];	// order in which songs in the playlist should be played
	unsigned short int currentSong;		// index of the song currently playing in list
};
typedef struct pList Playlist;
typedef Playlist* PlaylistPtr;

struct header
{
    char chunk_id[4];
    int chunk_size;
    char format[4];
    char subchunk1_id[4];
    int subchunk1_size;
    short int audio_format;
    short int num_channels;
    int sample_rate;
    int byte_rate;
    short int block_align;
    short int bits_per_sample;
    short int extra_param_size;
    char subchunk2_id[4];
    int subchunk2_size;
};
typedef struct header SongHeader;
typedef struct SongHeader* SongHeaderPtr;

struct music{
	unsigned short int songId; 	// unique numerical id to identify a song
	char* songName;
	int songState;				// state of the song (i.e. 2 == playing, 1 == paused, 0 == stopped,)
	unsigned char* songBuffer;	// ***** not sure if we need this in the struct *****
	int songSize;				// ***** not sure if we need this in the struct *****
	SongHeader header;
};
typedef struct music Song;
typedef struct Song* SongPtr;


// FUNCTION DECLARATIONS

Song createSong(char* name, unsigned short int* id);
/*
 * creates a new song with unique id and a song name
 */

void playSong();
/*
 * play a chosen song
 */

void pauseSong();
/*
 * pause a chosen song
 */

void skipSong();
/*
 * skip to the beginning or end of a chosen song
 */

void seekSong();
/*
 * seeks forwards and backward through a chosen song
 */

void volumeAdjust();
/*
 * adjust the volume of the music
 */

Playlist createPlaylist();
/*
 * creates and returns an empty playlist
 */

int addToPlaylist(Song s, PlaylistPtr ptr);
/*
 * adds chosen song 's' to the chosen playlist pointed to by 'ptr'
 * returns 0 if song successfully added
 * returns 1 if song already exists in playlist
 * returns 2 if playlist is full
 */

int removeFromPlaylist(Song s, PlaylistPtr ptr);
/*
 * removes song 's' from playlist pointed to by 'ptr'
 * returns 0 if song successfully removed
 * returns 1 if song is not found in playlist
 */
#endif /* UTILITIES_H_ */
