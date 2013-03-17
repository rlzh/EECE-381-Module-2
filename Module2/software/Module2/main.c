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
int songsize=2000000;
const int bufferconst = 96;
alt_up_audio_dev* audio;

/*
 *  global variables declaration end
 */


void loadSongFromSd (char* filename, unsigned char *memorybuffer){
	int handle;
	alt_up_sd_card_dev *device_sd = NULL;
	short header[44];
	int connect = 0;
	long int size_of_file;


	device_sd = alt_up_sd_card_open_dev(
			"/dev/Altera_UP_SD_Card_Avalon_Interface_0");
	if (device_sd != NULL) {

		if ((connect == 0) && (alt_up_sd_card_is_Present())) {
			printf("Card connected.\n");
			if (alt_up_sd_card_is_FAT16()) {
				printf("FAT16 file system detected.\n");
			} else {
				printf("Unknown file system.\n");
				return 0;
			}

			if ((handle = alt_up_sd_card_fopen(filename, false))>= 0) {
				printf("%s successfully opened!\n", filename);
				printf("Loading %s.\n", filename);
			} else {
				printf("Error opening %s\n", filename);
			}

			int temp;
			for (temp = 0; temp < 44; temp++) {
				short ret = alt_up_sd_card_read(handle);
				assert(ret >= 0);
				header[temp] = ret;
			}

			size_of_file = (header[43]<<24|header[42]<<16|header[41]<<8|header[40]);
			printf("size of file: %ld\n", size_of_file);
			songsize = size_of_file;
			soundbuffer = (unsigned char*) malloc(songsize + 96
								* sizeof(unsigned char));
			if (soundbuffer == NULL) {
				printf("malloc failed");
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


void audio_configs_setup(void) {
	alt_up_av_config_dev * av_config = alt_up_av_config_open_dev(AUDIO_AND_VIDEO_CONFIG_0_NAME);
	alt_up_av_config_reset(av_config);
	while (!alt_up_av_config_read_ready(av_config)) {
	}
	audio = alt_up_audio_open_dev(AUDIO_0_NAME);
	alt_up_audio_reset_audio_core(audio);
}

void audioISR(void * context, unsigned int ID_IRQ) {

	int temp2 = 0;
	for (temp2 = 0; temp2 < bufferconst; temp2++) {

		sam[temp2] = ((soundbuffer[random + 1] << 8) | soundbuffer[random])	<< 8;
		random += 2;

	}
	if (random >= songsize)
		random = 0;

	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_LEFT);
	alt_up_audio_write_fifo(audio, sam, bufferconst, ALT_UP_AUDIO_RIGHT);
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
	//////////////////////// SETUP AUDIO //////////////////////////////////////////////
		audio_configs_setup();
		loadSongFromSd("ZELDA.WAV", soundbuffer);

		random = 0;
		alt_up_audio_enable_write_interrupt(audio);

		alt_irq_register(AUDIO_0_IRQ, 0, (void*) audioISR);
		alt_irq_enable(AUDIO_0_IRQ);
        return 0;
}


