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

Song song1;
Song song2;

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

alt_up_audio_dev* audio;
alt_up_sd_card_dev* sd;
alt_up_rs232_dev* uart;
/*
 *  GLOBAL VARIABLES DECLARATION END
 */

/*
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

void Microphone (void)
{
	alt_up_audio_read_fifo(testmic,mic,96, ALT_UP_AUDIO_LEFT);
	alt_up_audio_read_fifo(testmic,mic,96, ALT_UP_AUDIO_RIGHT);

	alt_up_audio_write_fifo(audio, mic, bufferconst, ALT_UP_AUDIO_LEFT);
	alt_up_audio_write_fifo(audio, mic, bufferconst, ALT_UP_AUDIO_RIGHT);
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

	//  Song length will be here, left out for testing for now


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
	parseCommand(command, &(song1.volume), &(song1.state), &(song1.file_id));
	//parseCommand(command, &volume, &state, &file_id, &song_index);

}
void sd_card_configs_setup(void){
	sd = NULL;
	sd = alt_up_sd_card_open_dev("/dev/Altera_UP_SD_Card_Avalon_Interface_0");
}

void audioISR(void * context, unsigned int ID_IRQ) {
	int i;
	for (i = 0; i < bufferconst; i++){
		sam[i] = (unsigned int) song1.buf1[song1.play_index];
		song1.play_index++;
		if (song1.play_index == BUFFER_SIZE ){
				song1.play_index = 0;
		}
	}
	volumecontrol(sam,&(song1.volume),bufferconst);

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
		if((*song_ptr).state == IDLE && (*song_ptr).state_old!=IDLE)
			(*song_ptr).state = (*song_ptr).state_old;
		if (!init){
			if (abs((*song_ptr).load_index - ((*song_ptr).play_index)) > (half_buffer_size)){
				while (abs((*song_ptr).load_index - ((*song_ptr).play_index)) > (third_buffer_size)
						&& (*song_ptr).state == PLAY){
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
	load(init_load_size, (song_ptr), false);
	printf("enable IRQ\n");   // debug
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
	song1.state_old = IDLE;
	while(1){
		if(song1.state != song1.state_old && song1.state != IDLE){
			printf("\nstate change\n");
			if (song1.state == PLAY){ // play song
				printf("\nplay song songManager()\n");
				song1.state_old = song1.state;
				song1.file_name = getFileName(file_names, song1.file_id); // get file name
				//playSong(&song1);
				play(&song1);
			}
			else if(song1.state == NEXT){ // play next song
				printf("\nnext song songManager()\n");
				if (song1.state_old == PLAY){
					song1.state_old = song1.state;
					song1.state = PLAY;
				}
				else {
					song1.state_old = PAUSE;
					song1.state = PAUSE;
				}
			}
			else if(song1.state == PREV){ // play prev song
				printf("\nprev song songManager()\n");
				if (song1.state_old == PLAY){
					song1.state_old = song1.state;
					song1.state = PLAY;
				}
				else {
					song1.state_old = PAUSE;
					song1.state = PAUSE;
				}
			}
		}
	}
}


int main() {
	int i;
	int file_count;
	/////////////// INITIALIZE GLOBAL VARIABLES ///////////////////////////////
	song1.volume = 0;
	song1.state = IDLE;
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


	while(song1.state != PLAY);	// wait for android to play song


	//////////////////////// SETUP AUDIO ///////////////////////////////////
	audio_configs_setup();
	alt_up_audio_enable_write_interrupt(audio);
	alt_irq_register(AUDIO_0_IRQ, 0, (void*) audioISR);

	songManager();	// infinite loop

	//////////////////////// TESGING /////////////////////////////////////
	/*

	song2.state = PLAY;
	song2.file_name = "DYWC.WAV";
	play(&song2);
	int load_size = BUFFER_SIZE;
	int ret;

	song2.song_size = loadSong(sd, song2.file_name, &(song2.handle), 44);

	song2.song_index = 0;
	song2.load_index = 0;
	song2.play_index = 0;

	printf("load buffer initial\n");
	load(load_size, &song2, false);

	printf("enable IRQ\n");
	alt_irq_enable(AUDIO_0_IRQ);

	ret = load((song2.song_size-load_size), &song2, false);
	alt_irq_disable(AUDIO_0_IRQ);
	*/
	return 0;
}

