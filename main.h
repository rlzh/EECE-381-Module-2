/*
 * main.h
 *
 *  Created on: Mar 13, 2013
 *
 */

#ifndef MAIN_H_
#define MAIN_H_

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

#define MAX_SONGS_ALLOWED 20

struct pList {
	unsigned short int numOfSongs;
	unsigned short int list[MAX_SONGS_ALLOWED]; 	// limit to a maximum number of songs allowed for now...
	unsigned short int currentSong;		// index of the song currently playing in list
};
typedef struct pList Playlist;
typedef Playlist* PlaylistPtr;

struct music{
	unsigned short int songId; 	// unique numerical id to identify a song
	char* songName;
	int songState;				// state of the song (i.e. playing, paused, stopped)
	unsigned char* songBuffer;	// ***** not sure if we need this in the struct *****
	int songSize;				// ***** not sure if we need this in the struct *****

};
typedef struct music Song;


// FUNCTION DECLARATIONS

int copysongfromsd(char* filename);

void SoundEISR (void * test, unsigned int ID_irq);

void audioISR(void * context, unsigned int ID_IRQ);

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

void sendToAndroid(char* message);

char* receiveFromAndroid();



#endif /* MAIN_H_ */
