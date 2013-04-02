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
char* file_names[MAX_SONGS_ALLOWED]; // array of all files names in sd card
volatile char* command;				 // used to receive commands android

Song song;
DJ_Song song1;
DJ_Song song2;
DJ_Song song3;
DJ_Song song4;

unsigned int l_buf[96];
//DualChannel
unsigned char buf3[BUFFER_SIZE];
unsigned char buf4[BUFFER_SIZE];
volatile int buf_flag1;
volatile int buf_index1;
int buf2_count1;
int buf1_count1;

//Duelchannel or single channel flag
int singleordual;
int fifospace;


//Testing for mic not done yet
alt_up_audio_dev* testmic;
unsigned char microphone;
unsigned int mic[96];

const int bufferconst = 96;
unsigned int sam[96];

alt_up_audio_dev* audio;
alt_up_sd_card_dev* sd;
alt_up_rs232_dev* uart;
/*
 *  GLOBAL VARIABLES DECLARATION END
 */



void Microphone (void)
{

	//unsigned int l_buf[96];
	//unsigned int r_buf[96];
	int i;
	// open the Audio port
	//testmic = alt_up_audio_open_dev ("/dev/Audio");
	//printf ("testing \n");
	//if ( audio == NULL)
	//{
	//printf ("Error: could not open audio device \n");
	//}
	///else

	int fifospace = alt_up_audio_read_fifo_avail(audio,ALT_UP_AUDIO_LEFT);

	if (fifospace > 0 )
	{

		alt_up_audio_read_fifo(audio,(l_buf),96, ALT_UP_AUDIO_LEFT);
		//alt_up_audio_read_fifo(audio,(r_buf),96, ALT_UP_AUDIO_RIGHT);


		//alt_up_audio_write_fifo(audio, (r_buf), 96, ALT_UP_AUDIO_LEFT);
		//alt_up_audio_write_fifo(audio, (r_buf), 96, ALT_UP_AUDIO_RIGHT);
	}
	for (i = 0; i < bufferconst; i++){
		if ((l_buf[i] & 0x8000) > 0){
			l_buf[i] = l_buf[i] | 0xFFFF0000;
		}
	}
	volumecontrol(l_buf, -2, 96);

}


/*
void modifySingleorDualflag(int option)
{
	singleordual = option;
}

 */


void audio_configs_setup(void) {
	alt_up_av_config_dev * av_config = alt_up_av_config_open_dev(AUDIO_AND_VIDEO_CONFIG_0_NAME);
	alt_up_av_config_reset(av_config);
	while (!alt_up_av_config_read_ready(av_config)) {
	}
	audio = alt_up_audio_open_dev(AUDIO_0_NAME);
	alt_up_audio_reset_audio_core(audio);
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
	parseCommand(command, &(song.volume), &(song.state), &(song.file_id));
	//parseCommand(command, &volume, &state, &file_id, &song_index);

}
void sd_card_configs_setup(void){
	sd = NULL;
	sd = alt_up_sd_card_open_dev("/dev/Altera_UP_SD_Card_Avalon_Interface_0");
}

void audioISR(void * context, unsigned int ID_IRQ) {
	int i;
	for (i = 0; i < bufferconst; i++){
		sam[i] = (unsigned int) song.buf1[song.play_index];
		song.play_index++;
		if (song.play_index == BUFFER_SIZE ){
			song.play_index = 0;
		}
	}
	volumecontrol(sam,(song.volume),bufferconst);

	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_LEFT);
	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_RIGHT);
}

int state_checker( short int* state,  short int* state_old){
	if (*state == PAUSE){
		*state_old = PAUSE;
		alt_irq_disable(AUDIO_0_IRQ);
		while (*state == PAUSE);
		if (*state == PLAY)
			alt_irq_enable(AUDIO_0_IRQ);
	}
	if (*state == NEXT || *state == PREV){
		if (*state_old != PAUSE)
			alt_irq_disable(AUDIO_0_IRQ);
		return 1;
	}
	return 0;
}

int load(int load_size, Song* song_ptr, bool init){
	int i;
	int ret1 = 0;
	int ret2 = 0;
	int half_buffer_size = BUFFER_SIZE/2;
	//int quarter_buffer_size = BUFFER_SIZE/4;  // could use this variable instead of 'third_buffer_size'
	int third_buffer_size = BUFFER_SIZE/3;
	for (i = 0; i < load_size; i++){
		if((*song_ptr).state == IDLE && (*song_ptr).state_old != IDLE)
			(*song_ptr).state = (*song_ptr).state_old;
		if (!init){
			if (abs((*song_ptr).load_index - ((*song_ptr).play_index)) > (half_buffer_size)){
				while (abs((*song_ptr).load_index - ((*song_ptr).play_index)) > (third_buffer_size)
						&& (*song_ptr).state == PLAY){
					//printf("play index = %d\n ",(*song_ptr).play_index);
					// do nothing & wait;
				}
			}
		}
		//if (state_checker(&((*song_ptr).state),&((*song_ptr).state_old)) == 1) <-- Function state_checker seems to be slowing code down
		//return -1;
		if ((*song_ptr).state == PAUSE){
			(*song_ptr).state_old = PAUSE;
			alt_irq_disable(AUDIO_0_IRQ);
			while ((*song_ptr).state == PAUSE);
			if ((*song_ptr).state == PLAY)
				alt_irq_enable(AUDIO_0_IRQ);
		}
		if ((*song_ptr).state == NEXT || (*song_ptr).state == PREV){
			if ((*song_ptr).state_old != PAUSE)
				alt_irq_disable(AUDIO_0_IRQ);
			return 1;
		}

		if ((*song_ptr).load_index == BUFFER_SIZE)
			(*song_ptr).load_index = 0;

		if (i % 2 == 0){
			ret1 = alt_up_sd_card_read((*song_ptr).handle);
		} else {
			ret2 = alt_up_sd_card_read((*song_ptr).handle);
			if (ret1 < 0 || ret2 < 0) { return 1; }
			(*song_ptr).buf1[(*song_ptr).load_index] = (ret2 << 8 | ret1) << 8;
			(*song_ptr).load_index++;
		}
	}
	return 0;
}
void play(Song* song_ptr){
	int init_load_size = BUFFER_SIZE;
	int ret;

	(*song_ptr).song_size = (unsigned int) loadSong(sd, (*song_ptr).file_name, &((*song_ptr).handle), 44);
	alt_up_sd_card_fclose((*song_ptr).handle);
	(*song_ptr).song_size = (unsigned int) loadSong(sd, (*song_ptr).file_name, &((*song_ptr).handle), 44);

	(*song_ptr).song_length = (unsigned int) calcSongLength((*song_ptr).song_size);
	char* song_length = malloc(20*sizeof(char));
	snprintf(song_length, sizeof(song_length), "%d", (*song_ptr).song_length);
	printf("song length: %s seconds\n", song_length); 	//debug
	//sendToAndroid(song_length);

	(*song_ptr).song_index = 0;
	(*song_ptr).load_index = 0;
	(*song_ptr).play_index = 0;

	printf("load buffer initial\n");  //debug
	load(init_load_size, (song_ptr), true);

	printf("register & enable IRQ\n");   // debug
	alt_irq_register(AUDIO_0_IRQ, 0, (void*) audioISR);
	alt_irq_enable(AUDIO_0_IRQ);

	ret = load(((*song_ptr).song_size-init_load_size), (song_ptr), false);

	alt_irq_disable(AUDIO_0_IRQ);
	alt_up_sd_card_fclose((*song_ptr).handle);

	if (ret == 0){
		(*song_ptr).state_old = PAUSE;
		(*song_ptr).state = PAUSE;
	}
	return;
}

void songManager(void){
	song.state_old = IDLE;
	while(1){
		if(song.state != song.state_old && song.state != IDLE){
			printf("\nstate change\n");
			if (song.state == PLAY){ // play song
				printf("\nplay song songManager()\n");
				song.state_old = song.state;
				song.file_name = getFileName(file_names, song.file_id); // get file name
				//playSong(&song1);
				play(&song);
			}
			else if(song.state == NEXT){ // play next song
				printf("\nnext song songManager()\n");
				if (song.state_old == PLAY){
					song.state_old = song.state;
					song.state = PLAY;
				}
				else {
					song.state_old = PAUSE;
					song.state = PAUSE;
				}
			}
			else if(song.state == PREV){ // play prev song
				printf("\nprev song songManager()\n");
				if (song.state_old == PLAY){
					song.state_old = song.state;
					song.state = PLAY;
				}
				else {
					song.state_old = PAUSE;
					song.state = PAUSE;
				}
			}
		}
	}
}

void djISR(void * context, unsigned int ID_IRQ) {
	int i;
	bool both_positive = false;
	bool both_negative = false;
	for (i = 0; i < bufferconst; i++){
		if (song1.buffer[song1.play_index] < 0x00800000 && song2.buffer[song2.play_index] < 0x00800000 ){
			both_positive = true;
		}
		else if (song1.buffer[song1.play_index] > 0x00800000 && song2.buffer[song2.play_index] > 0x00800000){
			both_negative = true;
		}
		if (song1.play_index < song1.song_size && song2.play_index < song2.song_size){
			sam[i] = (unsigned int)( song1.buffer[song1.play_index] + song2.buffer[song2.play_index]) ;
			song1.play_index++;
			song2.play_index++;
		}
		else if (song1.play_index >= song1.song_size && song2.play_index < song2.song_size){
			sam[i] = (unsigned int)(song2.buffer[song2.play_index])  ;
			song2.play_index++;
		}
		else if (song1.play_index < song1.song_size && song2.play_index >= song2.song_size){
			sam[i] = (unsigned int)(song1.buffer[song1.play_index]) ;
			song1.play_index++;
		}
		else {
			sam[i] = 0;
		}
		/*if(sam[i]>= 0x00800000 && both_positive == true ){
			sam[i] = 0x007FFFFF;
		}
		else if (sam[i] > 0x00FFFFFF && both_negative == true){
			sam[i] = 0x00FFFFFF;
		}
		else if (sam[i] < 0x00800000 && both_negative == true){
			sam[i] = sam[i] | 0x00800000;

		}
		sam[i] = sam[i] & 0x00FFFFFF;*/
	}
	//volumecontrol (sam, -2, bufferconst);
	Microphone();
	for (i = 0; i<96;i++)
	{
		sam[i] = sam[i]+l_buf[i];
	}
	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_LEFT);
	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_RIGHT);
}

void loadDJ(int load_size, DJ_Song* dj_song_ptr, bool init){
	short ret1 = 0;
	short ret2 = 0;
	int val = 0;
	int i;
	if (init)
		(*dj_song_ptr).load_index = 0;

	for (i = 0; i < load_size; i++){
		if (i % 2 == 0){
			ret1 = alt_up_sd_card_read((*dj_song_ptr).handle);
		}
		else {
			ret2 = alt_up_sd_card_read((*dj_song_ptr).handle);
			if (ret1 < 0 || ret2 < 0)
				return;
			val = (ret2 << 8 | ret1);
			memcpy(&((*dj_song_ptr).buffer[(*dj_song_ptr).load_index]), &val, sizeof(val));
			if ((((*dj_song_ptr).buffer[(*dj_song_ptr).load_index]) & 0x8000) > 0){
				((*dj_song_ptr).buffer[(*dj_song_ptr).load_index]) = ((*dj_song_ptr).buffer[(*dj_song_ptr).load_index]) | 0xFFFF0000;
			}
			//(*dj_song_ptr).buffer[(*dj_song_ptr).load_index] = (ret2 << 8 | ret1);
			(*dj_song_ptr).load_index++;
		}
	}
}
/*
 // loads 2 DJ_songs at the same time
 // PROBLEM: loading 2 songs together is too slow

void load2(int load_size, DJ_Song* dj_song_ptr1, DJ_Song* dj_song_ptr2, bool init ){
	short ret1 = 0;
	short ret2 = 0;
	short ret3 = 0;
	short ret4 = 0;
	int val1 = 0;
	int val2 = 0;
	int i;
	if (init){
		(*dj_song_ptr1).load_index = 0;
		(*dj_song_ptr2).load_index = 0;
	}
	for (i = 0; i < load_size; i++){
		if (i % 2 == 0){  // load song1
			ret1 = alt_up_sd_card_read((*dj_song_ptr1).handle);
		}
		else {
			ret2 = alt_up_sd_card_read((*dj_song_ptr1).handle);
			if (ret1 < 0 || ret2 < 0){
				(*dj_song_ptr1).load_finished = true;
			}
			else {
				//val1 = (ret2 << 8 | ret1);
				//memcpy(&((*dj_song_ptr1).buffer[(*dj_song_ptr1).load_index]), &val1, sizeof(val1));
				(*dj_song_ptr1).buffer[(*dj_song_ptr1).load_index] = (ret2 << 8 | ret1);
				(*dj_song_ptr1).load_index++;
			}
		}

		if (i % 2 == 0){  // load song2
			ret3 = alt_up_sd_card_read((*dj_song_ptr2).handle);
		}
		else {
			ret4 = alt_up_sd_card_read((*dj_song_ptr2).handle);
			if (ret3 < 0 || ret4 < 0){
				(*dj_song_ptr2).load_finished = true;
			}
			else {
				//val2 = (ret4 << 8 | ret3);
				//memcpy(&((*dj_song_ptr2).buffer[(*dj_song_ptr2).load_index]), &val2, sizeof(val2));
				(*dj_song_ptr2).buffer[(*dj_song_ptr2).load_index] = (ret4 << 8 | ret3);
				(*dj_song_ptr2).load_index++;
			}
		}

		if ((*dj_song_ptr1).load_finished && (*dj_song_ptr2).load_finished){ // both songs are finished
			return;
		}
	}
}*/


void initDJ_Song(DJ_Song* dj_song_ptr){
	(*dj_song_ptr).load_finished = false;
	(*dj_song_ptr).song_size = loadSong(sd, ((*dj_song_ptr).file_name), &((*dj_song_ptr).handle), 44);
	(*dj_song_ptr).song_length = (unsigned int) calcSongLength((*dj_song_ptr).song_size);
	char* song_length = malloc(20*sizeof(char));
	snprintf(song_length, sizeof(song_length), "%d", (*dj_song_ptr).song_length);

	printf("song length: %s seconds\n", song_length); 	//debug

	(*dj_song_ptr).song_size = ((*dj_song_ptr).song_size);
	(*dj_song_ptr).buffer = malloc((((*dj_song_ptr).song_size)/2)*sizeof(int));

}

void playDJ(DJ_Song* dj_song_ptr1, DJ_Song* dj_song_ptr2){
	// initialize songs
	printf("song1\n"); // debug
	initDJ_Song(dj_song_ptr1);
	printf("song2\n");	// debug
	initDJ_Song(dj_song_ptr2);

	//load songs
	//loadDJ(DEBUG_SIZE, dj_song_ptr1, true);// <--- uncomment to stream
	//loadDJ(DEBUG_SIZE, dj_song_ptr2, true);// <--- uncomment to stream
	loadDJ((*dj_song_ptr1).song_size, dj_song_ptr1, true);// <--- uncomment to load
	loadDJ((*dj_song_ptr2).song_size, dj_song_ptr2, true);// <--- uncomment to load
	/*int i;
	for (i = 0; i < DEBUG_SIZE; i++){
		printf("buffer %d\n", (*dj_song_ptr1).buffer[i]);
	}*/
	(*dj_song_ptr1).song_size = ((*dj_song_ptr1).song_size)/2;
	(*dj_song_ptr2).song_size = ((*dj_song_ptr2).song_size)/2;

	// play
	(*dj_song_ptr1).play_index = 0;
	(*dj_song_ptr1).play_finished = false;
	(*dj_song_ptr2).play_index = 0;
	(*dj_song_ptr2).play_finished = false;

	printf("setup irq\n");
	alt_irq_register(AUDIO_0_IRQ, 0, (void*) djISR);
	printf("enable irq\n");
	alt_irq_enable(AUDIO_0_IRQ);
	while ((*dj_song_ptr1).play_index < (*dj_song_ptr1).song_size ||
			(*dj_song_ptr2).play_index < (*dj_song_ptr2).song_size){
		//loadDJ(DEBUG_SIZE, dj_song_ptr1, false); // <--- uncomment to stream
		//loadDJ(DEBUG_SIZE, dj_song_ptr2, false); // <--- uncomment to stream
	}

	if ((*dj_song_ptr1).play_index >= (*dj_song_ptr1).song_size){
		(*dj_song_ptr1).play_finished = true;
	}
	if ((*dj_song_ptr2).play_index >= (*dj_song_ptr2).song_size){
		(*dj_song_ptr1).play_finished = true;
	}

	printf("play index 1: %d\n",((*dj_song_ptr1).play_index)); // debug
	printf("play index 2: %d\n",((*dj_song_ptr2).play_index)); // debug
	alt_irq_disable(AUDIO_0_IRQ);
	alt_up_sd_card_fclose((*dj_song_ptr1).handle);
	alt_up_sd_card_fclose((*dj_song_ptr2).handle);
	free((*dj_song_ptr1).buffer);
	free((*dj_song_ptr2).buffer);
}

void djManager(void){
	//TO-DO

}

int main() {
	int i;
	int file_count;
	/////////////// INITIALIZE GLOBAL VARIABLES ///////////////////////////////
	song.volume = 0;
	song.state = IDLE;
	for(i=0; i<MAX_SONGS_ALLOWED; i++){
		file_names[i]= malloc(MAX_FNAME_LENGTH*sizeof(char));
	}

	loadSongInfo();	// send song info to android

	setFileId(file_names, &file_count);

	// debug
	for (i=0; i< file_count; i++){
		printf("file_name : %s\n", file_names[i]);
		printf("file id : %d\n", i);
	}
	//////////////////////// SETUP SD CARD //////////////////////////////////
	sd_card_configs_setup();

	//////////////////////// SETUP UART ////////////////////////////////////
	uart_configs_setup();
	alt_up_rs232_enable_read_interrupt(uart);
	alt_irq_register(RS232_0_IRQ, 0, (void*) androidListenerISR);
	alt_irq_enable(RS232_0_IRQ);

	//while(song1.state != PLAY);	// wait for android to play song

	//////////////////////// SETUP AUDIO ///////////////////////////////////
	audio_configs_setup();
	alt_up_audio_enable_write_interrupt(audio);

	//songManager();	// infinite loop

	//////////////////////// TESGING /////////////////////////////////////

	song1.file_name = "BARBRAS.WAV";
	song2.file_name = "STEREOL.WAV";
	song3.file_name = "STOCKH.WAV";
	song4.file_name = "HELLO.WAV";
	song1.volume = -4;
	playDJ(&song1, &song2);

/*
	song.state = PLAY;
	song.volume = -3;
	song.file_name = "DYWC.WAV";
	play(&song);
	int load_size = BUFFER_SIZE;
	int ret;

	song.song_size = loadSong(sd, song.file_name, &(song.handle), 44);

	song.song_index = 0;
	song.load_index = 0;
	song.play_index = 0;

	printf("load buffer initial\n");
	load(load_size, &song, true);

	printf("enable IRQ\n");
	alt_irq_register(AUDIO_0_IRQ, 0, (void*) audioISR);
	alt_irq_enable(AUDIO_0_IRQ);

	printf("load buffer\n");
	ret = load((song.song_size-load_size), &song, false);
	printf("done\n");
	alt_irq_disable(AUDIO_0_IRQ);
	*/

	/*printf("song1\n"); // debug

	song1.file_name = "BANDG.WAV";
	song1.song_size = loadSong(sd, (song1.file_name), &(song1.handle), 44);
	//alt_up_sd_card_fclose(song1.handle);
	//song1.song_size = loadSong(sd, (song1.file_name), &(song1.handle), 44);

	song1.song_length = (unsigned int) calcSongLength(song1.song_size);
	char* song_length = malloc(20*sizeof(char));
	snprintf(song_length, sizeof(song_length), "%d", song1.song_length);
	printf("song length: %s seconds\n", song_length); 	//debug

	song1.song_size = (song1.song_size);
	song1.buffer = malloc(((song1.song_size)/2)*sizeof(int));

	printf("load song1\n");  // debug
	loadDJ(&song1);

	printf("song2\n");	// debug

	song2.file_name = "HELLO.WAV";
	song2.song_size = loadSong(sd, (song2.file_name), &(song2.handle), 44);
	//alt_up_sd_card_fclose(song2.handle);
	//song2.song_size = loadSong(sd, (song2.file_name), &(song2.handle), 44);

	song2.song_length = (unsigned int) calcSongLength(song2.song_size);
	char* song_length2 = malloc(20*sizeof(char));
	snprintf(song_length2, sizeof(song_length2), "%d", song2.song_length);
	printf("song length: %s seconds\n", song_length2); 	//debug

	song2.song_size = (song2.song_size);
	song2.buffer = malloc(((song2.song_size)/2)*sizeof(int));

	printf("load song2\n");  // debug
	loadDJ(&song2);

	song1.song_size = (song1.song_size)/ 2;

	song2.song_size = (song2.song_size)/ 2;

	//printf("both songs loaded\n");
	while (1){
		playDJ(&song1, &song2);
	}
	free(song1.buffer);
	free(song2.buffer);
	printf("freed buffers");
	 */
	return 0;
}

