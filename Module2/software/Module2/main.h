/*
 * main.h
 *
 *  Created on: Mar 13, 2013
 *
 */

#ifndef MAIN_H_
#define MAIN_H_

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
#include "utilities.h"
#include "altera_avalon_timer.h"

// CONSTANTS DECLARATION
#define MAX_FNAME_LENGTH	8
#define BUFFER_SIZE		  	32004
#define SAMPLE_BUFFER_SIZE	96
#define IDLE				0
#define PLAY 				1
#define PAUSE				2
#define PREV				3
#define RWD					4
#define FWD					5
#define NEXT				6

struct music{
	volatile short int state;
	volatile short int state_old;
	volatile short int volume;
	volatile unsigned int file_id;

	char* file_name;
	unsigned int song_length;		// in seconds
	unsigned int song_size;			// in bytes
	unsigned int song_index;

	unsigned int buf1[BUFFER_SIZE];
	unsigned int buf2[BUFFER_SIZE];
	volatile int play_index;
	int load_index;
	int handle;
};
typedef struct music Song;

int load(int load_size, Song* song_ptr, bool init);
void play(Song* song_ptr);
void songManager(void);

void audio_configs_setup(void);
void audioISR(void * context, unsigned int ID_IRQ);

void dualchannelISR (void * context, unsigned int ID_IRQ);
void Microphone (void);
void modifySingleorDualflag(int option);

void uart_configs_setup(void);
void androidListenerISR(void * context, unsigned int ID_IRQ);


#endif /* MAIN_H_ */
