/*
 * main.h
 *
 *  Created on: Mar 13, 2013
 *
 */

#ifndef MAIN_H_
#define MAIN_H_

int copysongfromsd();
void SoundEISR (void * test, unsigned int ID_irq);
void audioISR(void * context, unsigned int ID_IRQ);


#endif /* MAIN_H_ */
