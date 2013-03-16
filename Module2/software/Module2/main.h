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

//typedef int bool;
#define true 1
#define false 0

// CONSTANTS DECLARATION

#define MAX_SONGS_ALLOWED 20


struct music{
	unsigned short int songId; // unique numerical id to identify a song
	char* songName;
	int songSize;
	unsigned char* songBuffer;
};
typedef struct music Song;


struct pList {
	unsigned short int numOfSongs;
	Song list[MAX_SONGS_ALLOWED]; 	// limit to a maximum number of songs allowed for now...
	unsigned short int order[MAX_SONGS_ALLOWED]
	unsigned short int currentSong;

};
typedef struct pList Playlist;

int copysongfromsd();
void SoundEISR (void * test, unsigned int ID_irq);
void audioISR(void * context, unsigned int ID_IRQ);
void sendToAndroid( char* message);
char* receiveFromAndroid();

#endif /* MAIN_H_ */
