/*
 * utilities.h
 *
 *  Created on: Mar 17, 2013
 *
 */

#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "altera_up_avalon_rs232.h"
#include "altera_up_sd_card_avalon_interface.h"
#include "altera_up_avalon_audio_and_video_config.h"
#include "altera_up_avalon_audio.h"
#include "sys/alt_irq.h"
#include <string.h>
#include "system.h"
#include "main.h"

// CONSTANTS DECLARATION
#define MAX_SONGS_ALLOWED		50
#define MAX_BYTES_PER_MESSAGE	130

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


void sendToAndroid(char* message);
/*
 * sends 'message' to Android
 */
char* receiveFromAndroid();
/*
 * receives message from Android and returns as 'char*'
 */
void loadSongInfo();
/*
 * loads song information from SD card and send it to Android
 */
unsigned int calcSongLength(Song song);
/*
 * calculates the length of 'song' and returns
 * the length in seconds
 */
void setFileId(char** file_names, int* file_count);
/*
 * stores all WAV files on SD card in 'file_names'
 * and the number of WAV files in 'file_count'
 */

char* getFileName(char** file_names, int id);
/*
 * returns the file name of song associated with 'file_id'
 * if song is not found, returns empty string
 */
void parseCommand(volatile char* command, volatile short int* volume,
			      volatile short int* state, volatile unsigned int* file_id);
/*
 * parse the commands received from Android into 'volume', 'state',
 * and 'file_id'
 */

void volumecontrol(unsigned int *buf, short int* volumenum, int buffersize);
/*
 * shifts bits in 'buf' accordingly to 'volumenum' to adjust volume of music
 */


#endif /* UTILITIES_H_ */
