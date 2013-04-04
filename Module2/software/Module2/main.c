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
char* playlist_file_names[MAX_SONGS_ALLOWED]; // array of all playlist files names in sd card
char* dj_file_names[MAX_SONGS_ALLOWED];		  // array fo all dj files names in sd card
volatile char* command;				 // used to receive commands android
volatile short int dj_or_playlist;	 // flag indicating which mode to play
volatile int channel1_balance;		 // value from -4 to 4 ----> -4 = only right song & 4 = only left song
volatile int channel2_balance;		 // value from -4 to 4 ----> -4 = only right song & 4 = only left song
volatile int speed1;
volatile int speed2;

Song song;
DJ_Song curr_song1;
DJ_Song curr_song2;

//DualChannel
unsigned int sam1[96];
unsigned int sam2[96];

const int bufferconst = 96;
unsigned int sam[96];
unsigned int mic_buffer[96];
int fifospace;

alt_up_audio_dev* audio;
alt_up_sd_card_dev* sd;
alt_up_rs232_dev* uart;
/*
 *  GLOBAL VARIABLES DECLARATION END
 */

void Microphone (void)
{
	int i;

	int fifospace = alt_up_audio_read_fifo_avail(audio,ALT_UP_AUDIO_LEFT);

	if (fifospace > 0 )
	{
		alt_up_audio_read_fifo(audio,(mic_buffer),96, ALT_UP_AUDIO_LEFT);
		//alt_up_audio_read_fifo(audio,(r_buf),96, ALT_UP_AUDIO_RIGHT);
		//alt_up_audio_write_fifo(audio, (r_buf), 96, ALT_UP_AUDIO_LEFT);
		//alt_up_audio_write_fifo(audio, (r_buf), 96, ALT_UP_AUDIO_RIGHT);
	}
	for (i = 0; i < bufferconst; i++){
		if ((mic_buffer[i] & 0x8000) > 0){
			mic_buffer[i] = mic_buffer[i] | 0xFFFF0000;
		}
	}
	//volumecontrol(mic_buffer, -2, 96);
}

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

void sd_card_configs_setup(void){
	sd = NULL;
	sd = alt_up_sd_card_open_dev("/dev/Altera_UP_SD_Card_Avalon_Interface_0");
}

void androidListenerISR(void * context, unsigned int ID_IRQ){
	unsigned char data;
	unsigned char parity;

	command = receiveFromAndroid();
	while (alt_up_rs232_get_used_space_in_read_FIFO(uart)) {// clear read FIFO
		alt_up_rs232_read_data(uart, &data, &parity);
	}
	parseCommand(command, &dj_or_playlist, &((song).volume),&(song.state),&(song.file_id),&(curr_song2.file_id),
			&(curr_song1.file_id),&channel2_balance, &channel1_balance, &speed2, &speed1, &(curr_song2.state),
			&(curr_song1.state));
}

void audioISR(void * context, unsigned int ID_IRQ) {
	int i;
	for (i = 0; i < bufferconst; i++){
		sam[i] = (unsigned int) (song.buf1[song.play_index]) >> 1;
		song.play_index++;
		if (song.play_index == BUFFER_SIZE ){
			song.play_index = 0;
		}
	}
	volumeAdjust(sam, song.volume, bufferconst);
	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_LEFT);
	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_RIGHT);
}

int load(int load_size, Song* song_ptr, bool init){
	int i;
	int ret1 = 0;
	int ret2 = 0;
	int half_buffer_size = BUFFER_SIZE/2;
	int two_third_buffer_size = (2*BUFFER_SIZE)/3;

	for (i = 0; i < load_size; i++){
		if((*song_ptr).state == IDLE && (*song_ptr).state_old != IDLE)
			(*song_ptr).state = (*song_ptr).state_old;
		if (!init){
			if (abs((*song_ptr).load_index - ((*song_ptr).play_index)) > (two_third_buffer_size)){
				while (abs((*song_ptr).load_index - ((*song_ptr).play_index)) > (half_buffer_size)
						&& (*song_ptr).state == PLAY){
					// do nothing & wait;
					if (dj_or_playlist == 1){
						alt_irq_disable(AUDIO_0_IRQ);
						alt_up_sd_card_fclose(song.handle);
						printf("\nbreaking from wait\n");
						return 0;
					}
				}
			}
		}
		if ((*song_ptr).state == PAUSE){
			(*song_ptr).state_old = PAUSE;
			alt_irq_disable(AUDIO_0_IRQ);
			while ((*song_ptr).state == PAUSE){
				if (dj_or_playlist == 1){
					alt_up_sd_card_fclose(song.handle);
					printf("\nbreak from pause\n");
					return 0;
				}
			}
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
			if (ret1 < 0 || ret2 < 0){
				return 1;
			}
			(*song_ptr).buf1[(*song_ptr).load_index] = (ret2 << 8 | ret1);
			if ((((*song_ptr).buf1[(*song_ptr).load_index]) & 0x8000) > 0){
				((*song_ptr).buf1[(*song_ptr).load_index]) = ((*song_ptr).buf1[(*song_ptr).load_index]) | 0xFFFF0000;
			}
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
	printf("\nenter song manager\n");
	song.state_old = IDLE;
	while(1){
		if (dj_or_playlist == 1){
			alt_irq_disable(AUDIO_0_IRQ);
			alt_up_sd_card_fclose(song.handle);
			printf("\nleaving song manager\n");
			break;
		}
		if(song.state != song.state_old && song.state != IDLE){
			printf("\nstate change\n");
			if (song.state == PLAY){ // play song
				printf("\nplay song songManager()\n");
				song.state_old = song.state;
				song.file_name = getFileName(playlist_file_names, song.file_id); // get file name
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
	unsigned int sam1_song1_input = 0;
	unsigned int sam1_song2_input = 0;
	unsigned int sam2_song1_input = 0;
	unsigned int sam2_song2_input = 0;

	for (i = 0; i < bufferconst; i++){
		sam1_song1_input = (unsigned int) (curr_song1.buffer[curr_song1.play_index]) << 3;
		sam1_song2_input = (unsigned int) (curr_song2.buffer[curr_song2.play_index]) << 3;
		sam2_song1_input = (unsigned int) (curr_song1.buffer[curr_song1.play_index]) << 3;
		sam2_song2_input = (unsigned int) (curr_song2.buffer[curr_song2.play_index]) << 3;

		balanceAdjust(&sam1_song1_input, &sam1_song2_input, channel1_balance);
		balanceAdjust(&sam2_song1_input, &sam2_song2_input, channel2_balance);

		if (curr_song1.play_index >= (curr_song1.buffer_size-1))
			curr_song1.play_index = 0;
		if (curr_song2.play_index >= (curr_song2.buffer_size-1))
			curr_song2.play_index = 0;

		if ((curr_song1.state == PLAY && curr_song2.state == PLAY)){
			sam1[i] = (sam1_song1_input + sam1_song2_input);
			sam2[i] = (sam2_song1_input + sam2_song2_input);
			curr_song1.play_index+=1;
			curr_song2.play_index+=1;
		}
		else if (((curr_song1.state == PAUSE || curr_song1.state == IDLE)&& curr_song2.state == PLAY)){
			sam1[i] = (sam1_song2_input);
			sam2[i] = (sam2_song2_input);
			curr_song2.play_index++;
		}
		else if ((curr_song1.state == PLAY && (curr_song2.state == PAUSE || curr_song2.state == IDLE ))){
			sam1[i] = (sam1_song1_input);
			sam2[i] = (sam2_song1_input);
			curr_song1.play_index++;
		}

		else {
			sam1[i] = 0;
			sam2[i] = 0;
		}
	}
	Microphone(); // get microphone data
	/*for (i = 0; i < 96; i++){ // merge microphone data with song data
		sam1[i] = sam1[i] + mic_buffer[i];
		sam2[i] = sam2[i] + mic_buffer[i];
	}*/
	alt_up_audio_write_fifo(audio, sam1, bufferconst, ALT_UP_AUDIO_LEFT);
	alt_up_audio_write_fifo(audio, sam2, bufferconst, ALT_UP_AUDIO_RIGHT);
}

void initDJ_Song(DJ_Song* dj_song_ptr){
	(*dj_song_ptr).load_finished = false;
	int size = strlen(((*dj_song_ptr).file_name));
	char* fname  = malloc((size+3)*sizeof(char));
	strcpy(fname,"DJ/");
	strcat(fname,((*dj_song_ptr).file_name));
	(*dj_song_ptr).song_size = loadSong(sd, fname, &((*dj_song_ptr).handle), 44);
	(*dj_song_ptr).song_length = (unsigned int) calcSongLength((*dj_song_ptr).song_size);
	char* song_length = malloc(20*sizeof(char));
	snprintf(song_length, sizeof(song_length), "%d", (*dj_song_ptr).song_length);

	printf("file name: %s\n", (*dj_song_ptr).file_name);	// debug
	printf("song length: %s seconds\n", song_length); 		// debug

	(*dj_song_ptr).buffer_size = ((*dj_song_ptr).song_size)/2;
	(*dj_song_ptr).buffer = malloc(((*dj_song_ptr).buffer_size)*sizeof(int));

	(*dj_song_ptr).load_index = 0;
	(*dj_song_ptr).play_index = 0;
	(*dj_song_ptr).play_finished = false;

}

void loadDJ(int load_size, DJ_Song* dj_song_ptr, bool load_entire){
	short ret1 = 0;
	short ret2 = 0;
	int val = 0;
	int i;

	for (i = 0; i < load_size; i++){
		if (i % 2 == 0){
			ret1 = alt_up_sd_card_read((*dj_song_ptr).handle);
		}
		else {
			ret2 = alt_up_sd_card_read((*dj_song_ptr).handle);
			if (ret1 < 0 || ret2 < 0){
				(*dj_song_ptr).load_finished = true;
				if (load_entire)
					alt_up_sd_card_fclose((*dj_song_ptr).handle);
				return;
			}
			val = (ret2 << 8 | ret1);
			memcpy(&((*dj_song_ptr).buffer[(*dj_song_ptr).load_index]), &val, sizeof(val));
			if ((((*dj_song_ptr).buffer[(*dj_song_ptr).load_index]) & 0x8000) > 0){
				((*dj_song_ptr).buffer[(*dj_song_ptr).load_index]) = ((*dj_song_ptr).buffer[(*dj_song_ptr).load_index]) | 0xFFFF0000;
			}
			(*dj_song_ptr).load_index++;
		}
	}
}

void playDJ(DJ_Song* dj_song_ptr1, DJ_Song* dj_song_ptr2){
	// play
	alt_irq_register(AUDIO_0_IRQ, 0, (void*) djISR);
	alt_irq_enable(AUDIO_0_IRQ);

	alt_avalon_timer_sc_init(0,AUDIO_0_IRQ_INTERRUPT_CONTROLLER_ID,AUDIO_0_IRQ,100);

	/*(*dj_song_ptr1).play_finished = false;
	(*dj_song_ptr2).play_finished = false;
	*/
	while ((*dj_song_ptr1).play_index < (*dj_song_ptr1).buffer_size &&
			(*dj_song_ptr2).play_index < (*dj_song_ptr2).buffer_size){
		if ((*dj_song_ptr1).state == PAUSE && (*dj_song_ptr2).state == PAUSE){
			printf("\nboth songs paused\n");	//debug
			alt_irq_disable(AUDIO_0_IRQ);
			while((*dj_song_ptr1).state == PAUSE && (*dj_song_ptr2).state == PAUSE){
				if (dj_or_playlist == 0){
					printf("\nleave pause DJ\n");	//debug
					free(curr_song1.buffer);
					free(curr_song2.buffer);
					return;
				}
			}
			alt_irq_enable(AUDIO_0_IRQ);
		}

	}
	/*if ((*dj_song_ptr1).play_index >= (*dj_song_ptr1).buffer_size){
		(*dj_song_ptr1).play_finished = true;
		(*dj_song_ptr1).play_index = 0;
	}
	if ((*dj_song_ptr2).play_index >= (*dj_song_ptr2).buffer_size){
		(*dj_song_ptr2).play_finished = true;
		(*dj_song_ptr2).play_index = 0;
	}*/
	printf("disable irq\n");			// debug
	alt_irq_disable(AUDIO_0_IRQ);
}

void djManager(void){
	printf("enter dj manager\n");
	bool first_iteration = true;
	while(1){
		if (dj_or_playlist == 0){
			printf("\nleave djmanager\n");
			free(curr_song1.buffer);
			free(curr_song2.buffer);
			break;
		}
		if (first_iteration){ // only initialize & load songs in first iteration
			printf("\nfirst iteration\n");
			// get file names
			printf("\nget file names\n");
			curr_song1.file_name = getFileName(dj_file_names, curr_song1.file_id);
			curr_song2.file_name = getFileName(dj_file_names, curr_song2.file_id);
			// init songs
			printf("\ninit songs\n");
			initDJ_Song(&curr_song1);
			initDJ_Song(&curr_song2);
			// load songs
			printf("\nload song1\n");
			loadDJ(curr_song1.song_size, &curr_song1, true);
			printf("\nload song2\n");
			loadDJ(curr_song2.song_size, &curr_song2, true);
			first_iteration = false;
		}
		// play
		printf("\nplay songs\n");
		playDJ(&curr_song1, &curr_song2);
	}
	return;
}

int main() {
	int i;
	int play_list_file_count;
	int dj_file_count;
	////////////////// INITIALIZE GLOBAL VARIABLES ///////////////////////////////
	song.state = IDLE;
	curr_song1.state = IDLE;
	curr_song2.state = IDLE;
	dj_or_playlist = 2;

	for(i = 0; i < bufferconst; i++){
		mic_buffer[i] = 0;
	}
	for(i = 0; i < MAX_SONGS_ALLOWED; i++){
		playlist_file_names[i]= malloc(MAX_FNAME_LENGTH*sizeof(char));
		dj_file_names[i] = malloc(MAX_FNAME_LENGTH*sizeof(char));
	}

	loadSongInfo("");	// sends song info to android
	loadSongInfo(DJ_DIR);
	char* dir = malloc(5*sizeof(char));
	strcpy(dir,"");
	setFileId(playlist_file_names, &play_list_file_count, dir);
	strcpy(dir,"DJ/");
	setFileId(dj_file_names, &dj_file_count, dir);

	// debug
	printf("playlist songs\n");
	for (i=0; i< play_list_file_count; i++){
		printf("file_name : %s\n", playlist_file_names[i]);
		printf("file id : %d\n", i);
	}
	printf("dj songs\n");
	for (i=0; i< dj_file_count; i++){
		printf("file_name : %s\n", dj_file_names[i]);
		printf("file id : %d\n", i);
	}
	//////////////////////// SETUP SD CARD //////////////////////////////////
	sd_card_configs_setup();

	//////////////////////// SETUP UART ////////////////////////////////////
	uart_configs_setup();
	alt_up_rs232_enable_read_interrupt(uart);
	alt_irq_register(RS232_0_IRQ, 0, (void*) androidListenerISR);
	alt_irq_enable(RS232_0_IRQ);

	//while(song.state != PLAY && (curr_song1.state != PLAY) && (curr_song2.state != PLAY));	// wait for android to play song

	//////////////////////// SETUP AUDIO ///////////////////////////////////
	audio_configs_setup();
	alt_up_audio_enable_write_interrupt(audio);

	/*while(1){
		songManager();
		djManager();
	}*/
	//////////////////////// TESGING /////////////////////////////////////
	curr_song1.file_id = 0;
	curr_song2.file_id = 1;
	curr_song1.state = PLAY;
	curr_song2.state = PLAY;
	channel1_balance = -4;
	channel2_balance = 4;
	djManager();

	/*
	song.state = PLAY;
	song.file_name = "YOUNGB.WAV";
	song.volume = 8;
	play(&song);
	*/
	/*while(1){
		i++;
		if (i%102345 == 0){
			printf("\ncurr_song1 fid = %d\n", curr_song1.file_id);
			printf("curr_song2 fid = %d\n", curr_song2.file_id);
			printf("ch1 balance  = %d\n", channel1_balance);
			printf("ch2 balance  = %d\n", channel2_balance);
			printf("speed1 = %d\n", speed1);
			printf("speed2 = %d\n", speed2);
			printf("state1 = %d\n", curr_song1.state);
			printf("state2 = %d\n", curr_song2.state);
			printf("normal song fid = %d\n", song.file_id);
		}
	}*/
	return 0;
}

