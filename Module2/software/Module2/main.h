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

int copysongfromsd();
void SoundEISR (void * test, unsigned int ID_irq);
void audioISR(void * context, unsigned int ID_IRQ);


#endif /* MAIN_H_ */
