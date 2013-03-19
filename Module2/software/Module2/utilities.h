/*
 * utilities.h
 *
 *  Created on: Mar 17, 2013
 *      Author: Ryan Liu
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

void sendToAndroid( char* message);
char* receiveFromAndroid();
void loadSongInfo(char** song_info);
void parseCommand(char* command, int* state, int* volume, char* file_name);

#endif /* UTILITIES_H_ */
