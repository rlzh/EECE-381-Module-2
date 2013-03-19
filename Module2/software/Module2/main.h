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
#include "utilities.h"


//typedef int bool;
#define true 1
#define false 0

// CONSTANTS DECLARATION

#define MAX_SONGS_ALLOWED 	50
#define BUFFER_SIZE		  	65534
#define SAMPLE_BUFFER_SIZE	96
#define IDLE				0
#define PLAY 				1
#define PAUSE				2
#define PREV				3
#define RWD					4
#define FWD					5
#define NEXT				6


struct music{
	unsigned short int songId; // unique numerical id to identify a song
	char* songName;
	unsigned long int songSize;
	unsigned char* songBuffer;
};
typedef struct music Song;


struct pList {
	unsigned short int numOfSongs;
	Song list[MAX_SONGS_ALLOWED]; 	// limit to a maximum number of songs allowed for now...
	unsigned short int order[MAX_SONGS_ALLOWED];
	unsigned short int currentSong;

};
typedef struct pList Playlist;


void loadSongHeader(char* fname);
int loadSongBuffer();
int playSong(char* file_name);

void songManager(char* command);
void adjustVolume(int new_volume);
void audio_configs_setup(void);
void audioISR(void * context, unsigned int ID_IRQ);

#endif /* MAIN_H_ */
