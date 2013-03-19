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
unsigned char buf1[65534];
unsigned char buf2[65534];
short int state;
int buf_flag;
int buf_index;
int song_size;
int buf2_count;
int buf1_count;
int handle;
int volume;
alt_up_sd_card_dev* sd;
const int bufferconst = 96;
unsigned int sam[96];
alt_up_audio_dev* audio;

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
		//printf("Card connected.\n");
		if ((handle = alt_up_sd_card_fopen(fname, false)) >= 0) {
			//printf("%s successfully opened!\n", fname);
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
	printf("size of file: %d\n", size_of_file);
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
	}
	if(buf_flag == 1){
		buf1_count = BUFFER_SIZE;
	}
	else{
		buf2_count = BUFFER_SIZE;
	}
	return 0;
}


int playSong(char* file_name){
	loadSongHeader(file_name);

	int end_of_song;
	int buf_flag_old;
	//alt_irq_disable(AUDIO_0_IRQ);

	buf1_count = BUFFER_SIZE;
	buf2_count = BUFFER_SIZE;
	buf_index = 0;
	buf_flag = 1;
	loadSongBuffer();
	buf_flag = 2;
	buf_flag_old = buf_flag;

	alt_irq_enable(AUDIO_0_IRQ);
	while(1){
		end_of_song = loadSongBuffer();
		while (buf_flag == buf_flag_old);
			buf_flag = abs(3-buf_flag_old);
		buf_flag_old = (buf_flag);
		if (end_of_song == 1){
			printf("end of song\n");
			alt_irq_disable(AUDIO_0_IRQ);
			//while (buf_flag == buf_flag_old);
			break;
		}
	}
	return 0;
}
/*void songManager(){
	short int i;
	unsigned char data;
	unsigned char parity;

	printf("UART Initialization\n");
	alt_up_rs232_dev* uart = alt_up_rs232_open_dev(RS232_0_NAME);
}*/

void audio_configs_setup(void) {
	alt_up_av_config_dev * av_config = alt_up_av_config_open_dev(
			AUDIO_AND_VIDEO_CONFIG_0_NAME);
	alt_up_av_config_reset(av_config);
	while (!alt_up_av_config_read_ready(av_config)) {
	}
	audio = alt_up_audio_open_dev(AUDIO_0_NAME);
	alt_up_audio_reset_audio_core(audio);
}

void audioISR(void * context, unsigned int ID_IRQ) {
	int temp;
	unsigned char* buf;
	if(buf_flag == 2){
		buf = buf1;
	}
	else {
		buf = buf2;
	}
	for (temp = 0; temp < bufferconst; temp++) {
		if (volume == 1){
			sam[temp] = (unsigned int)((buf[buf_index + 1] << 8) | buf[buf_index]) << 8;
		}
		else if(volume == 2){
			sam[temp] = (unsigned int)((buf[buf_index + 1] << 8) | buf[buf_index]) << 9;

		}
		else if(volume == 3){
			sam[temp] = (unsigned int)((buf[buf_index + 1] << 8) | buf[buf_index]) << 10;
		}
		else if(volume == 4){
			sam[temp] = (unsigned int)((buf[buf_index + 1] << 8) | buf[buf_index]) << 11;
		}
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

	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_LEFT);
	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_RIGHT);

}


int main() {
	volume = 2;
	state = IDLE;
	char** song_info;
	loadSongInfo(song_info);
	printf("song info from main\n%s\n", *song_info);

	sendToAndroid(*song_info);
	free(song_info);

	//////////////////////// SETUP AUDIO ///////////////////////////////////////
	audio_configs_setup();
	alt_up_audio_enable_write_interrupt(audio);
	alt_irq_register(AUDIO_0_IRQ, 0, (void*) audioISR);

	while(1){
		//char* msg = receiveFromAndroid();
		//printf("\nmsg: %s\n", msg);
		printf("play song\n");

		playSong("SNOW.WAV");
		playSong("8BIT.WAV");
		playSong("DYWC.WAV");
		playSong("SNOW.WAV");
		playSong("HUMAN.WAV");
		playSong("C2G.WAV");
		playSong("FIN2DARK.WAV");
		playSong("MIA.WAV");
		playSong("TTSKY.WAV");
	}

	return 0;
}

