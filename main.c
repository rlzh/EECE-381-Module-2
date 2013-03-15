/*
 * main.c
 *
 *  Created on: Mar 12, 2013
 *
 */


#include "main.h"


/*
 *  global variables declaration begin
 */
unsigned char *soundbuffer;
unsigned int sam[96];
int random ;
int songsize = 784000;
const int bufferconst = 96;
alt_up_audio_dev* audio;
Playlist songLibrary;
unsigned short int idCount = 1;

/*
 *  global variables declaration end
 */

void audio_configs_setup(void) {
	alt_up_av_config_dev * av_config = alt_up_av_config_open_dev(AUDIO_AND_VIDEO_CONFIG_0_NAME);
	alt_up_av_config_reset(av_config);
	while (!alt_up_av_config_read_ready(av_config)) {
	}
	audio = alt_up_audio_open_dev(AUDIO_0_NAME);
	alt_up_audio_reset_audio_core(audio);
}

void audioISR(void * context, unsigned int ID_IRQ) {

	short int temp2 = 0;
	for (temp2 = 0; temp2 < bufferconst; temp2++) {

		sam[temp2] = ((soundbuffer[random + 1] << 8) | soundbuffer[random])	<< 8;
		random += 2;

	}
	if (random >= songsize)
		random = 0;

	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_LEFT);
	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_RIGHT);
}


void getSongFromSD(){
		alt_up_sd_card_dev* device_reference = NULL;
		short int connected = 0;
		short int first_file = 0;
		short int list_file = 1;
		short int read_file = 1;
		int file_handle;
		int file_data;
		char* file_name;
		char* temp;
		Song song;

		device_reference = alt_up_sd_card_open_dev(ALTERA_UP_SD_CARD_AVALON_INTERFACE_0_NAME);

		if (device_reference != NULL) {
			while(1) {
				// first detection of FAT 16 SD card
				if ((connected == 0) && (alt_up_sd_card_is_Present())) {
					printf("Card connected.\n");
					if (alt_up_sd_card_is_FAT16()) {
						printf("FAT 16 file system detected.\n");
					} else {
						printf("Unknown file system.\n");
						break;
					}
					connected = 1;
				}
				// SD card is still connected
				else if ((connected == 1) && (alt_up_sd_card_is_Present()) && (list_file == 1)){
					// finding the first file in directory
					if ((first_file == 0) && (alt_up_sd_card_find_first("", file_name)) == 0){
						printf("Listing WAV files found on disk\n");
						// identify current file is a WAV file
						if (strstr(file_name, ".WAV") != NULL){
							temp = strtok(file_name, ".");
							printf("%s\n", temp);
							song = createSong(temp, &idCount);
							addToPlaylist(song, &songLibrary);
						}
						first_file = 1;
					}
					// finding the next file in directory
					else if((first_file == 1) && (alt_up_sd_card_find_next(file_name) == 0)){
						// identify current file is a WAV file
						if (strstr(file_name, ".WAV") != NULL){
							temp = strtok(file_name, ".");
							printf("%s\n", temp);
							song = createSong(temp, &idCount);
							addToPlaylist(song, &songLibrary);
						}
					}
					else {
						printf("Finished listing WAV files\n");
						first_file = 0;
						list_file = 0;
					}
				}
				else if ((connected == 1) && (alt_up_sd_card_is_Present()) && (read_file == 1)){
					// commented out code below used for debug

					/*file_handle = alt_up_sd_card_fopen("INFO.txt", false);
					while ((file_data = alt_up_sd_card_read(file_handle)) >= 0){
						//printf("%c",(char) file_data);
						if (strcmp((char*)&file_data, ",") != 0){
							strcat(temp, (char*)&file_data);
						}
						else{
							printf("%s\n", temp);
							strcpy(temp, "");
						}
					}
					alt_up_sd_card_fclose(file_handle);
					read_file = 0; */
					break;
				}
			}
		}
}

void getSongHeader(SongPtr song){
	alt_up_sd_card_dev* device_reference = NULL;
	int i;
	char* file_name = (*song).songName;
	int file_data;
	int file_handle;
	int header = 44;

	device_reference = alt_up_sd_card_open_dev(ALTERA_UP_SD_CARD_AVALON_INTERFACE_0_NAME);
	strcat(file_name, ".WAV");

	if ( device_reference != NULL && (alt_up_sd_card_is_FAT16()) && (alt_up_sd_card_is_Present())){
		file_handle = alt_up_sd_card_fopen(file_name, false);
		for (i = 0; i < header; i++){
			file_data = alt_up_sd_card_read(file_handle);
			printf("%c", file_data);
		}
		alt_up_sd_card_fclose(file_handle);
	}
}

int copysongfromsd(char* filename) {
	int handle;
	alt_up_sd_card_dev* device_sd = NULL;
	int header = 44;
	int connect = 0;

	device_sd = alt_up_sd_card_open_dev(ALTERA_UP_SD_CARD_AVALON_INTERFACE_0_NAME);

	if (device_sd != NULL) {

		if ((connect == 0) && (alt_up_sd_card_is_Present())) {
			printf("Card connected.\n");
			if (alt_up_sd_card_is_FAT16()) {
				printf("FAT16 file system detected.\n");
			} else {
				printf("Unknown file system.\n");
				return 0;
			}

			// OPENS FILE
			handle = alt_up_sd_card_fopen(filename, false);
			if (handle >= 0) {
				printf("%s successfully opened!\n", filename);
				printf("Loading %s.", filename);
			} else {
				printf("Error opening %s\n", filename);
			}

			soundbuffer = (unsigned char*) malloc(songsize + 96* sizeof(unsigned char));
			if (soundbuffer == NULL) {
				printf("malloc failed");
			}
			int temp;
			for (temp = 0; temp < header; temp++) {
				alt_up_sd_card_read(handle);

			}
			for (temp = 0; temp < songsize; temp++) {
				short ret = alt_up_sd_card_read(handle);
				assert(ret >= 0);
				soundbuffer[temp] = ret;
			}

			if (alt_up_sd_card_fclose(handle) != -1) {
				printf("File %s closed.\n", filename);
			} else {
				printf("Error closing %s\n", filename);
				return -1;
			}
			connect = 1;

		}

		else if ((connect == 1) && (alt_up_sd_card_is_Present() == false)) {
			printf("Card disconnected.\n");
			return -1;
		}
	}
	return 1;
}

void sendToAndroid( char* message){
	short int i;
	unsigned char data;
	unsigned char parity;

    printf("UART Initialization\n");
    alt_up_rs232_dev* uart = alt_up_rs232_open_dev(RS232_0_NAME);

    printf("Clearing read buffer to start\n");
	while (alt_up_rs232_get_used_space_in_read_FIFO(uart)) {
		alt_up_rs232_read_data(uart, &data, &parity);
	}

    printf("Sending the message to the Middleman\n");

    // Start with the number of bytes in our message

	alt_up_rs232_write_data(uart, (char) strlen(message));

	// Now send the actual message to the Middleman

	for (i = 0; i < strlen(message); i++) {
		alt_up_rs232_write_data(uart, message[i]);
	}
}


char* receiveFromAndroid(){
		/*
		 * IMPLEMENTATION NOTE: Could instead always receive a predefined amount of bytes
		 * 						of data from Android side. (e.g. each message from Andriod
		 * 						will always be 8 bytes long)
		 * 						This way allows passing an array into the function by reference
		 * 						and function doesn't need to return anything.
		 */

		short int i;
		unsigned char data;
		unsigned char parity;

		printf("UART Initialization\n");
	    alt_up_rs232_dev* uart = alt_up_rs232_open_dev(RS232_0_NAME);


		printf("Clearing read buffer to start\n");
		while (alt_up_rs232_get_used_space_in_read_FIFO(uart)) {
			alt_up_rs232_read_data(uart, &data, &parity);
		}
		printf("Waiting for data to come from the Middleman\n");
		while (alt_up_rs232_get_used_space_in_read_FIFO(uart) == 0);

		// First byte is the number of characters in our message

		alt_up_rs232_read_data(uart, &data, &parity);
		int num_to_receive = (int)data;
		char* message = malloc(num_to_receive * sizeof(char));

		printf("About to receive %d characters:\n", num_to_receive);
		for (i = 0; i < num_to_receive; i++) {
			while (alt_up_rs232_get_used_space_in_read_FIFO(uart) == 0);
			alt_up_rs232_read_data(uart, &data, &parity);
			message[i] = data;
			printf("%c", data);
		}

	    return message;
}


int main()
{
	songLibrary = createPlaylist();
	getSongFromSD();
	getSongHeader(&((songLibrary).list[(songLibrary).currentSong]));
	//////////////////////// SETUP AUDIO //////////////////////////////////////////////
	audio_configs_setup();
	if (copysongfromsd("FFI.wav") != 1) {
			printf("error in loading song");
	}

	random = 0;
	alt_up_audio_enable_write_interrupt(audio);
	alt_irq_register(AUDIO_0_IRQ, 0, (void*) audioISR);
	alt_irq_enable(AUDIO_0_IRQ);

	printf("%d\n", songLibrary.numOfSongs);

	while(1);

	return 0;
}


