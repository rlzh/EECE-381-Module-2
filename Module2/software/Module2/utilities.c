/*
 * utilities.c
 *
 *  Created on: Mar 17, 2013
 *
 */
#include "utilities.h"

void sendToAndroid(char* message) {
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

char* receiveFromAndroid() {
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
	unsigned char* message;

	printf("UART Initialization\n");
	alt_up_rs232_dev* uart = alt_up_rs232_open_dev(RS232_0_NAME);

	printf("Waiting for data to come from the Middleman\n");
	if (alt_up_rs232_get_used_space_in_read_FIFO(uart) == 0){
		return "nothing";
	}
	else {
		// First byte is the number of characters in our message
		alt_up_rs232_read_data(uart, &data, &parity);
		int num_to_receive = (int) data;
		message = malloc(num_to_receive * sizeof(unsigned char));

		printf("About to receive %d characters:\n", num_to_receive);
		for (i = 0; i < num_to_receive; i++) {
			while (alt_up_rs232_get_used_space_in_read_FIFO(uart) == 0);
			alt_up_rs232_read_data(uart, &data, &parity);
			message[i] = data;
			printf("%c", data);
		}
	}
	return message;
}

void loadSongInfo(char** song_info){
	short int file_size = 0;
	short int ret;
	int handle;
	alt_up_sd_card_dev* sd = NULL;
	sd = alt_up_sd_card_open_dev("/dev/Altera_UP_SD_Card_Avalon_Interface_0");

	if (sd != NULL && alt_up_sd_card_is_Present() && alt_up_sd_card_is_FAT16()) {
		printf("Card connected.\n");
		if ((handle = alt_up_sd_card_fopen("INFO.TXT", false))>=0){
			while ( (ret = alt_up_sd_card_read(handle)) >=0 ){
				file_size++;
			}
			alt_up_sd_card_fclose(handle);
		}
		//printf("file size %d\n", file_size);
		*song_info = malloc(file_size*sizeof(char));
		*song_info[0] = '\0';
		if (song_info == NULL)
			return;
		if ((handle = alt_up_sd_card_fopen("INFO.TXT", false))>=0){
			while ( (ret = alt_up_sd_card_read(handle)) >=0 ){
				//printf("%c", (char)ret);
				strcat(*song_info,&ret);
			}
			alt_up_sd_card_fclose(handle);
		}
		else {
			printf("error opening file\n");
		}
	}
	//printf("song info from func\n%s\n", *song_info);
	return;
}

void parseCommand(char* command, int* state, int* volume, char* file_name){
	char* temp='\0';
	int i=0;
	while(strcmp(&command[i],",")!= 0){
		strcat(temp,&command[i]);
		i++;
	}
	*volume = atoi(temp);
	printf("volume : %d\n", *volume);
	temp='\0';
	i++;
	while(strcmp(&command[i],",")!= 0){
		strcat(temp,&command[i]);
		i++;
	}
	i++;
	*state = atoi(temp);
	printf("state : %d\n", *state);
	temp='\0';
	while(strcmp(&command[i],"=")!= 0){
		strcat(temp,&command[i]);
		i++;
	}
	*file_name = atoi(temp);
	printf("fname : %d\n", *file_name);
	temp='\0';
	return;
}


