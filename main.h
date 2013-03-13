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
#include <string.h>
#include "system.h"

typedef int bool;
#define true 1
#define false 0

// CONSTANTS DECLARATION

#define MAX_SONGS_ALLOWED 20

struct pList {
	unsigned short int numOfSongs;
	unsigned short int list[MAX_SONGS_ALLOWED]; 	// limit to a maximum number of songs allowed for now...
	char* prevSongName;
	char* currentSongName;
	char* nextSongName;
};
typedef struct pList Playlist;

struct music{
	unsigned short int songId; // unique numerical id to identify a song
	char* songName;
	int songSize;
	unsigned char *songbuffer;
};
typedef struct music Song;


int copysongfromsd();
void SoundEISR (void * test, unsigned int ID_irq);
void audioISR(void * context, unsigned int ID_IRQ);


#endif /* MAIN_H_ */
