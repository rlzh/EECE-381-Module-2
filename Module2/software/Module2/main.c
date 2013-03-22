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
int file_count;

volatile char* command;
volatile short int state;
volatile short int state_old;
volatile short int volume;
volatile unsigned int file_id;
//char* file_name;

unsigned char buf1[65536];
unsigned char buf2[65536];
volatile int buf_flag;
volatile int buf_index;
int song_size;
int buf2_count;
int buf1_count;
const int bufferconst = 96;
unsigned int sam[96];

int handle;
alt_up_audio_dev* audio;
alt_up_sd_card_dev* sd;
alt_up_rs232_dev* uart;

/*
 *  GLOBAL VARIABLES DECLARATION END
 */


void loadSongHeader(char* fname) {
	short header[44];
	int size_of_file;
	int sample_rate;
	int song_index;
	sd = NULL;
	sd = alt_up_sd_card_open_dev("/dev/Altera_UP_SD_Card_Avalon_Interface_0");
	if (sd != NULL && alt_up_sd_card_is_Present() && alt_up_sd_card_is_FAT16()) {
		if ((handle = alt_up_sd_card_fopen(fname, false)) >= 0) {
		} else {
			printf("Error opening %s\n", fname);
			return;
		}
	}
	for (song_index = 0; song_index < 44; song_index++) { // read header file
		short ret = alt_up_sd_card_read(handle);
		assert(ret >= 0);
		header[song_index] = ret;
	}
	sample_rate = (header[27] << 24 | header[26] << 16 | header[25] << 8 | header[24]);
	//printf("sample rate: %ld\n", sample_rate);

	// calculate size of file
	size_of_file = (header[43] << 24 | header[42] << 16 | header[41] << 8 | header[40]);
	//printf("size of file: %d\n", size_of_file);
	song_size = size_of_file;
	return;
}

int loadSongBuffer() {
	unsigned char* buf;
	int temp;
	short ret;

	if (buf_flag == 1){
		buf = buf1;
	}
	else{
		buf = buf2;
	}
	for (temp = 0;temp < BUFFER_SIZE; temp++){
		ret = alt_up_sd_card_read(handle);
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
			while (state == PAUSE);
			if (state == PLAY)
				alt_irq_enable(AUDIO_0_IRQ);
		}
		if (state == NEXT || state == PREV){
			printf("\nnext|prev in loadSongBuffer()\n");
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


int playSong(char* f_name){

	loadSongHeader(f_name);

	int end_of_song = 0;
	int buf_flag_old;
	buf1_count = BUFFER_SIZE;
	buf2_count = BUFFER_SIZE;
	buf_index = 0;
	buf_flag = 1;

	loadSongBuffer();
	alt_irq_enable(AUDIO_0_IRQ);

	buf_flag = 2;
	buf_flag_old = buf_flag;
	unsigned int i=0;
	while(1){
		//printfs for debug
		//printf("iteration : %d\n", i);
		//printf("volume : %d\n", volume);
		//printf("state : %d\n", state);
		//printf("fid : %d\n", file_id);
		alt_irq_enable(AUDIO_0_IRQ);	// <----- adding these 3 fixes glitches but why?????
		alt_irq_enable(AUDIO_0_IRQ);	// <-----
		alt_irq_enable(AUDIO_0_IRQ);	// <-----
		if(state == IDLE && state_old!=IDLE){
			state = state_old;
		}
		if (state == PLAY){
			end_of_song = loadSongBuffer();
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
			while (state == PAUSE);
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
		i++;
	}
	return 0;
}

void songManager(void){
	state_old = IDLE;
	char* f_name;
	while(1){
		if(state != state_old && state != IDLE){
			printf("\nstate change\n");
			if (state == PLAY){ // play song
				printf("\nplay song songManager()\n");
				state_old = state;
				// get file name
				f_name = getFileName(file_names, file_id);
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
	int temp;
	int i;
	unsigned char* buf;
	if(buf_flag == 2){
		buf = buf1;
	}
	else {
		buf = buf2;
	}
	for (temp = 0; temp < bufferconst; temp++){
		sam[temp] = (unsigned int)((buf[buf_index + 1] << 8) | buf[buf_index]) << 8;
		buf_index += 2;

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
	//volumecontrol(sam,&volume,96);
	/*
	 * NOTE: moved volumecontrol here because I declared 'sam' to be volatile
	 * 		 something to do with letting things other than this code(i.e. the interrupts)
	 * 		 to modify variables in the code. I'm not too sure about this though.
	 * 		 Will ask Jeff about this on Tuesday.
	 */
	for (i = 0; i < bufferconst; i++) {
		if (volume == 2){
			if (sam[i] == 0x007FFFFF)
				sam [i] = 0x007FFFFF;
			else
				sam[i] = sam[i] << 2;
		}
		if (volume == 1){
			if (sam[i] == 0x007FFFFF)
				sam [i] = 0x007FFFFF;
			else
				sam[i] = sam[i] << 1;
		}
		if (volume == 0)
			sam[i] = sam [i];
		if (volume == -1) {
			if (sam[i] >= 0x00800000) {
				sam[i] = sam[i] >> 1;
				sam[i] = (sam[i] | 0x00800000);
			} else {
				sam[i] = sam[i] >> 1;
			}
		}
		if (volume == -2) {
			if (sam[i] >= 0x00800000) {
				sam[i] = sam[i] >> 2;
				sam[i] = (sam[i] | 0x00C00000);// & 0x00DFFFFF;
			} else {
				sam[i] = sam[i] >> 2;
			}
		}
		if (volume == -3){
			if (sam[i] >= 0x00800000) {
				sam[i] = sam[i] >> 3;
				sam[i] = (sam[i] | 0x00E00000);
			} else {
				sam[i] = sam[i] >> 3;
			}
		}
		if (volume == -4){
			if (sam[i] >= 0x00800000) {
				sam[i] = sam[i] >> 4;
				sam[i] = (sam[i] | 0x00F00000);
			} else {
				sam[i] = sam[i] >> 4;
			}
		}
	}

	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_LEFT);
	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_RIGHT);
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
	parseCommand(command, &volume, &state, &file_id);
}


int main() {
	int i;
	// initialize global variables
	volume = 0;
	state = IDLE;
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

	songManager();	// infinite loop

	//hardcoded function
	/*while(1){
		printf("\nplay Song\n");
		playSong("HUMAN.WAV");
		//playSong("8BIT.WAV");
		//playSong("ANIMALB.WAV");
		//playSong("BREAKD.WAV");
		playSong("HEYYA.WAV");
		playSong("C2G.WAV");
		//playSong("DYWC.WAV");
		//playSong("SNOW.WAV");
		playSong("FIN2DARK.WAV");
		//playSong("MIA.WAV");
		//playSong("TTSKY.WAV");
	}*/

	return 0;
}

