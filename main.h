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



// FUNCTION DECLARATIONS

void audio_configs_setup(void);

void audioISR(void * context, unsigned int ID_IRQ);

void getSongFromSD();
/*
 * finds the file names of every WAV file in SD card and adds them
 * into the global playlist 'songLibrary'
 */

void getSongHeader(SongPtr s);
/*
 * reads the header file of song pointed to by 's' and stores it
 * as a member variable
 */

int copysongfromsd(char* filename);

void sendToAndroid(char* message);

char* receiveFromAndroid();



#endif /* MAIN_H_ */
