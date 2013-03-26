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
#include "altera_avalon_timer.h"


//typedef int bool;
#define true 1
#define false 0

// CONSTANTS DECLARATION
#define MAX_FNAME_LENGTH	8
#define BUFFER_SIZE		  	64000
#define SAMPLE_BUFFER_SIZE	96
#define IDLE				0
#define PLAY 				1
#define PAUSE				2
#define PREV				3
#define RWD					4
#define FWD					5
#define NEXT				6


void loadSong(char* fname, int index);
int loadBuffer();
int playSong(char* file_name);
void songManager(void);

void audio_configs_setup(void);
void audioISR(void * context, unsigned int ID_IRQ);

void uart_configs_setup(void);
void androidListenerISR(void * context, unsigned int ID_IRQ);


#endif /* MAIN_H_ */
