/*
 * utilities.h
 *
 *  Created on: Mar 17, 2013
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
#include "main.h"

// CONSTANTS DECLARATION
#define MAX_SONGS_ALLOWED		50
#define MAX_BYTES_PER_MESSAGE	130
#define BYTES_PER_SECOND		64000





void sendToAndroid(char* message);
/*
 * sends 'message' to Android
 */
char* receiveFromAndroid();
/*
 * receives message from Android and returns as 'char*'
 */
void loadSongInfo(char* dir);
/*
 * loads song information from SD card and send it to Android
 */

void setFileId(char** file_names, int* file_count, char* dir);
/*
 * stores all WAV files on SD card in 'file_names'
 * and the number of WAV files in 'file_count'
 */

char* getFileName(char** file_names, int id);
/*
 * returns the file name of song associated with 'file_id'
 * if song is not found, returns empty string
 */

void parseCommand(volatile char* command, volatile short int* dj_or_playlist, volatile short int* volume, volatile short int* state,
				volatile unsigned int* file_id,volatile unsigned int* fid1, volatile unsigned int* fid2,
				volatile int* ch1_balance, volatile int* ch2_balance, volatile int* song1_speed,
				volatile int* song2_speed,volatile short int* state1, volatile short int* state2,
				volatile unsigned int* play_time1, volatile unsigned int* play_time2);
/*
 * master parse function : reads the first bit to decide calling between parseCommandPlaylist or parseCommandDJ
 */

void parseCommandPlaylist(volatile char* command, volatile short int* volume,
			      volatile short int* state, volatile unsigned int* file_id/*,
			      volatile unsigned int* song_index*/);
/*
 * parse the commands received from Android into 'volume', 'state',
 * and 'file_id'
 */

void parseCommandDJ(volatile char* command, volatile unsigned int* fid1, volatile unsigned int* fid2,
		volatile int* ch1_balance, volatile int* ch2_balance, volatile int* song1_speed,
		volatile int* song2_speed, volatile short int* state1, volatile short int* state2,
		volatile unsigned int* play_time1, volatile unsigned int* play_time2);
/*
 * parse the commands received from android running in DJ mode
 */

void volumecontrol(unsigned int *buf, volatile short int volumenum, int buffersize);  // <----- deleted use function below
/*
 * shifts bits in 'buf' accordingly to 'volumenum' to adjust volume of music
 */

void volumeAdjust (unsigned int* buffer, int volume, int buffer_size);
/*
 * shifts bits in 'buffer' accordingly to 'volume' to adjust volume of music
 */

void balanceAdjust(unsigned int* input1, unsigned int* input2, int balance);
/*
 * shifts the bits in input1 & input2 in accordance to balance
 */

int loadSong(alt_up_sd_card_dev* sd, char* fname, int* handle, int index);
/*
 * loads song with file name 'fname' to specified index in the song
 */

int calcSongLength(unsigned int size_of_file);
/*
 * calculates and returns the length of a song given 'size_of_file'
 */

int calcPlayIndex(volatile unsigned int* song_time, volatile int* play_index);



#endif /* UTILITIES_H_ */
