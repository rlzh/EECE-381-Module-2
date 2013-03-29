/*
 * main.c
 *
 *  Created on: Mar 12, 2013
 *
 */

#include "main.h"

/*
 *  GLOBAL VARIABLES DECLARATION BEGIN
 */
char* file_names[MAX_SONGS_ALLOWED];

volatile char* command;
volatile short int state;
volatile short int state_old;
volatile short int volume;
volatile unsigned int file_id;
volatile unsigned int song_index;
volatile short int dj_flag;

short int song_sel; // use to select between 2 songs in DJ mode

unsigned char buf1[BUFFER_SIZE];
unsigned char buf2[BUFFER_SIZE];
volatile int buf_flag;
volatile int buf_index;
int buf2_count;
int buf1_count;

//DualChannel
unsigned char buf3[BUFFER_SIZE];
unsigned char buf4[BUFFER_SIZE];
volatile int buf_flag1;
volatile int buf_index1;
int buf2_count1;
int buf1_count1;

//Duelchannel or single channel flag
int singleordual;


//Testing for mic not done yet
alt_up_audio_dev* testmic;
unsigned char microphone;
unsigned int mic[96];

const int bufferconst = 96;
unsigned int sam[96];
unsigned int sam1[96];
int handle[2];
alt_up_audio_dev* audio;
alt_up_rs232_dev* uart;

/*
 *  GLOBAL VARIABLES DECLARATION END
 */

int loadBuffer() {
	unsigned char* buf;
	int temp;
	short ret;
	unsigned int i;
	if (buf_flag == 1){
		buf = buf1;
	}
	else{
		buf = buf2;
	}
	for (temp = 0;temp < BUFFER_SIZE; temp++){
		ret = alt_up_sd_card_read(handle[song_sel]);
		i++;
		if (ret < 0){
			if(buf_flag == 1)
				buf1_count = temp;
			else
				buf2_count = temp;
			return 1;
		}
		buf[temp] = ret;
		if (state == PAUSE){
			state_old = PAUSE;
			printf("\npause in loadSongBuffer()\n");
			alt_irq_disable(AUDIO_0_IRQ);
			while (state == PAUSE); // wait while song paused
			if (state == PLAY)
				alt_irq_enable(AUDIO_0_IRQ);
		}
		if (state == NEXT || state == PREV){
			printf("\nnext|prev in loadSongBuffer()\n");
			if (state_old != PAUSE)
				alt_irq_disable(AUDIO_0_IRQ);
			buf1_count = 0;
			buf2_count = 0;
			return 2;
		}
	}
	if(buf_flag == 1){
		buf1_count = BUFFER_SIZE;
	}
	else{
		buf2_count = BUFFER_SIZE;
	}
	return 0;
}

int playDualChannel (char *fname, char *fname1)
{
	unsigned int temp1;
	unsigned int temp2;

	//Calls twice
	temp1 = loadSong(fname, &handle[song_sel], song_index);
	temp1 = loadSong(fname, &handle[song_sel], song_index);
	temp2 = loadSong(fname1, &handle[song_sel], song_index);
	temp2 = loadSong(fname1, &handle[song_sel], song_index);
	temp1 = calcSongLength (temp1);
	temp2 = calcSongLength (temp2);
	/*
	 * Song length will be here, left out for testing for now
	 *
	 */
	int end_of_song = 0;
	int buf_flag_old;
	buf1_count = BUFFER_SIZE;
	buf2_count = BUFFER_SIZE;
	buf_index = 0;
	buf_flag = 1;
	buf1_count1 = BUFFER_SIZE;
	buf2_count1 = BUFFER_SIZE;
	buf_index1 = 0;
	buf_flag1 = 1;

	//Need  New Load buffer Method
}



int playSong(char* fname){
	unsigned int temp;
	temp = loadSong(fname, &handle[song_sel], song_index);
	temp = loadSong(fname, &handle[song_sel], song_index);// call twice to make sure no garbage in reading

	temp = calcSongLength(temp);
	char* song_length = malloc(20*sizeof(char));
	// use this to convert int to string since the libraries do not include 'itoa()'
	snprintf(song_length, sizeof(song_length), "%d", temp);
	printf("\nsong_length: %s seconds\n", song_length);//debug
	//sendToAndroid(song_length);
	free(song_length);

	int end_of_song = 0;
	int buf_flag_old;
	buf1_count = BUFFER_SIZE;
	buf2_count = BUFFER_SIZE;
	buf_index = 0;
	buf_flag = 1;

	loadBuffer();
	alt_irq_enable(AUDIO_0_IRQ);

	buf_flag = 2;
	buf_flag_old = buf_flag;
	unsigned int i=0;
	while(1){
		//printf("iteration : %d\n", i); // debugging
		//printf("volume : %d\n", volume);
		//printf("state : %d\n", state);
		//printf("fid : %d\n", file_id);
		alt_irq_enable(AUDIO_0_IRQ);	// <----- adding these 3 fixes glitches but why???
		alt_irq_enable(AUDIO_0_IRQ);	// <-----
		alt_irq_enable(AUDIO_0_IRQ);	// <-----

		if(state == IDLE && state_old!=IDLE){
			state = state_old;
		}
		if (state == PLAY){
			end_of_song = loadBuffer();
			while (buf_flag == buf_flag_old){
				if(state == NEXT || state == PREV){
					buf_flag = abs(3-buf_flag_old);
				}
			}
			buf_flag_old = buf_flag;
		}
		if (state == PAUSE){
			state_old = PAUSE;
			printf("\npause song play()\n "); //debug
			alt_irq_disable(AUDIO_0_IRQ);
			while (state == PAUSE); // wait while song paused
			if (state == PLAY){
				alt_irq_enable(AUDIO_0_IRQ);
			}
		}
		if(state == NEXT || state == PREV){
			printf("\nnext song play()\n");	//debug
			if (state_old != PAUSE)
				alt_irq_disable(AUDIO_0_IRQ);
			break;
		}
		if (end_of_song == 1){
			state_old = PAUSE;
			state = PAUSE;
			printf("\nend of song play()\n"); //debug
			while (buf_flag == buf_flag_old);
			alt_irq_disable(AUDIO_0_IRQ);
			break;
		}
		i++; // debug
	}
	return 0;
}

void songManager(void){
	state_old = IDLE;
	char* f_name;
	while(1){
		if (dj_flag == 1){
			break;
		}
		if(state != state_old && state != IDLE){
			printf("\nstate change\n");
			if (state == PLAY){ // play song
				printf("\nplay song songManager()\n");
				state_old = state;
				f_name = getFileName(file_names, file_id); // get file name
				playSong(f_name);
			}
			else if(state == NEXT){ // play next song
				printf("\nnext song songManager()\n");
				if (state_old == PLAY){
					state_old = state;
					state = PLAY;
				}
				else {
					state_old = PAUSE;
					state = PAUSE;
				}
			}
			else if(state == PREV){ // play prev song
				printf("\nprev song songManager()\n");
				if (state_old == PLAY){
					state_old = state;
					state = PLAY;
				}
				else {
					state_old = PAUSE;
					state = PAUSE;
				}
			}
		}
	}
	return;
}

void audio_configs_setup(void) {
	alt_up_av_config_dev * av_config = alt_up_av_config_open_dev(AUDIO_AND_VIDEO_CONFIG_0_NAME);
	alt_up_av_config_reset(av_config);
	while (!alt_up_av_config_read_ready(av_config)) {
	}
	audio = alt_up_audio_open_dev(AUDIO_0_NAME);
	alt_up_audio_reset_audio_core(audio);
}

void audioISR(void * context, unsigned int ID_IRQ) {
	int i;
	unsigned char* buf;
	if(buf_flag1 == 2){
		buf = buf1;
	}
	else {
		buf = buf2;
	}
	for (i = 0; i < bufferconst; i++){
		sam[i] = (unsigned int)((buf[buf_index + 1] << 8) | buf[buf_index]) << 8;
		buf_index += 2; // increasing this makes sound higher octave

		if((buf_flag == 2 && buf_index >= buf1_count)){ // buffer 1 is empty
			buf_index = 0;
			buf = buf2;
			buf_flag = 1;
		}
		else if((buf_flag == 1 && buf_index >= buf2_count)){ // buffer 2 is empty
			buf_index = 0;
			buf = buf1;
			buf_flag = 2;
		}
	}
	volumecontrol(sam,&volume,bufferconst);

	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_LEFT);
	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_RIGHT);
}

void dualchannelISR (void * context, unsigned int ID_IRQ)
{
	int i;
		unsigned char* buf;
		if(buf_flag == 2){
			buf = buf1;
		}
		else {
			buf = buf2;
		}
		for (i = 0; i < bufferconst; i++){
			sam[i] = (unsigned int)((buf[buf_index + 1] << 8) | buf[buf_index]) << 8;
			buf_index += 2; // increasing this makes sound higher octave

			if((buf_flag == 2 && buf_index >= buf1_count)){ // buffer 1 is empty
				buf_index = 0;
				buf = buf2;
				buf_flag = 1;
			}
			else if((buf_flag == 1 && buf_index >= buf2_count)){ // buffer 2 is empty
				buf_index = 0;
				buf = buf1;
				buf_flag = 2;
			}
		}


			unsigned char* bufD;
			if(buf_flag1 == 2){
				bufD = buf3;
			}
			else {
				bufD = buf4;
			}
			for (i = 0; i < bufferconst; i++){
				sam1[i] = (unsigned int)((buf[buf_index1 + 1] << 8) | buf[buf_index1]) << 8;
				buf_index1 += 2; // increasing this makes sound higher octave

				if((buf_flag1 == 2 && buf_index1 >= buf1_count)){ // buffer 1 is empty
					buf_index1 = 0;
					buf = buf2;
					buf_flag = 1;
				}
				else if((buf_flag == 1 && buf_index1 >= buf2_count)){ // buffer 2 is empty
					buf_index1 = 0;
					buf = buf1;
					buf_flag1 = 2;
				}
			}
			volumecontrol(sam,&volume,bufferconst);
			volumecontrol(sam1,&volume,bufferconst);
			alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_LEFT);
			alt_up_audio_write_fifo(audio, sam1, bufferconst, ALT_UP_AUDIO_RIGHT);

}

void uart_configs_setup(void){
	printf("UART Initialization\n");
	uart = alt_up_rs232_open_dev(RS232_0_NAME);
}

void androidListenerISR(void * context, unsigned int ID_IRQ){
	unsigned char data;
	unsigned char parity;

	command = receiveFromAndroid();
	while (alt_up_rs232_get_used_space_in_read_FIFO(uart)) {// clear read FIFO
		alt_up_rs232_read_data(uart, &data, &parity);
	}
	if (dj_flag == 0){
		parseCommand(command, &volume, &state, &file_id);
		//parseCommand(command, &volume, &state, &file_id, &song_index);
	}
	else if(dj_flag == 1){
		//TO-DO: implement parseCommandDJMode()
	}
}

void modifySingleorDualflag(int option)
{
	singleordual = option;
}

void Microphone (void)
{
	alt_up_audio_read_fifo(testmic,mic,96, ALT_UP_AUDIO_LEFT);
	alt_up_audio_read_fifo(testmic,mic,96, ALT_UP_AUDIO_RIGHT);

	alt_up_audio_write_fifo(audio, mic, bufferconst, ALT_UP_AUDIO_LEFT);
	alt_up_audio_write_fifo(audio, mic, bufferconst, ALT_UP_AUDIO_RIGHT);
}

int main() {
	int i;
	int file_count;
	///////////////////// INITIALIZE GLOBAL VARIABLES /////////////////////////////
	volume = 0;
	song_sel = 0;
	dj_flag = 0;
	singleordual = 0;
	state = IDLE;
	song_index = 44;
	for(i=0; i<MAX_SONGS_ALLOWED; i++){
			file_names[i]= malloc(MAX_FNAME_LENGTH*sizeof(char));
	}

	loadSongInfo();
	setFileId(file_names, &file_count);
	// debug
	/*for (i=0; i< file_count; i++){
		printf("file_name : %s\n", file_names[i]);
		printf("file id : %d\n", i);
	}*/

	//////////////////////// SETUP UART //////////////////////////////////////
	uart_configs_setup();
	alt_up_rs232_enable_read_interrupt(uart);
	alt_irq_register(RS232_0_IRQ, 0, (void*) androidListenerISR);
	alt_irq_enable(RS232_0_IRQ);

	while(state != PLAY);	// wait for android to play song

	//////////////////////// SETUP AUDIO ///////////////////////////////////////
	audio_configs_setup();
	alt_up_audio_enable_write_interrupt(audio);
	alt_irq_register(AUDIO_0_IRQ, 0, (void*) audioISR);


	while(1){
		songManager();	// infinite loop
		//TO-DO: implement djManager()
	}

	/////////////////////// TESTING ///////////////////////////////////////////
	int handle1;
	int handle2;
	dj_flag = 1;
	loadSong("MIA.WAV",&handle1, 44);

	return 0;
}

