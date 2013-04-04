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

	printf("\nsendToAndroid() UART Initialization\n");
	alt_up_rs232_dev* uart = alt_up_rs232_open_dev(RS232_0_NAME);

	printf("Clearing read buffer to start\n");
	while (alt_up_rs232_get_used_space_in_read_FIFO(uart)) {
		alt_up_rs232_read_data(uart, &data, &parity);
	}

	printf("Sending the message to the Middleman\n");

	// Start with the number of bytes in our message

	alt_up_rs232_write_data(uart, (char) strlen(message));

	// Now send the actual message to the Middleman
	assert(strlen(message) <= MAX_BYTES_PER_MESSAGE);
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

	printf("\nreceiveFromAndroid() UART Initialization\n");
	alt_up_rs232_dev* uart = alt_up_rs232_open_dev(RS232_0_NAME);

	printf("Check for data to come from the Middleman\n");
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
	return (char*) message;
}

void loadSongInfo(char* dir){
	short int file_size = 0; // # of bytes in INFO.txt
	short int msg_size = 0;	// # of bytes being sent in msg
	unsigned int file_count = 0; // # of WAV files in INFO.txt

	unsigned int* break_points; // points in INFO.txt we should break into a new msg
	unsigned int break_point = 0;
	unsigned short break_point_count = 0; // # of breakpoints in 'break_point'

	unsigned int file_index = 0;
	unsigned int file_index_old = 0;
	unsigned int temp = 0;
	char* song_info;
	short int ret;
	int handle;
	char* file_name = malloc(8*sizeof(char));
	strcpy(file_name, "INFO.TXT");
	strcat(dir, file_name);
	//printf("dir from loadsongINFO= %s\n", dir);
	alt_up_sd_card_dev* sd = NULL;
	sd = alt_up_sd_card_open_dev("/dev/Altera_UP_SD_Card_Avalon_Interface_0");
	/*
	 * ITERATION 1: count the # of files
	 * 				count the max # of breakpoints
	 * 				count the # of bytes
	 */
	if (sd != NULL && alt_up_sd_card_is_Present() && alt_up_sd_card_is_FAT16()) {
		if ((handle = alt_up_sd_card_fopen(dir, false))>=0){
			while ((ret = alt_up_sd_card_read(handle)) >=0 ){
				file_size++; // count # of bytes in INFO.txt
				if(strcmp((char*)(&ret),";")==0){
					break_point_count++; // count the max # of possible break points
					file_count++;	// count # of files in INFO.txt
				}
			}
			alt_up_sd_card_fclose(handle);
		}
	}
	/*
	 * ITERATION 2 : determine & store proper breakpoints in 'break_points'
	 */
	break_points = malloc(break_point_count*sizeof(unsigned int)); // allocate enough memory for all break points
	break_point_count = 0;

	if (sd != NULL && alt_up_sd_card_is_Present() && alt_up_sd_card_is_FAT16()) {
		if ((handle = alt_up_sd_card_fopen(dir, false))>=0){
			while ((ret = alt_up_sd_card_read(handle)) >=0 ){
				temp++;
				msg_size++;
				if(strcmp((char*)(&ret),";")==0 && msg_size <= MAX_BYTES_PER_MESSAGE){
					break_point = temp;
					if (temp == file_size-1){ // reached end of file
						break_points[break_point_count] = break_point;
						break_point_count++;
					}
				}
				else if (strcmp((char*)(&ret),";")==0 && msg_size > MAX_BYTES_PER_MESSAGE){
					break_points[break_point_count] = break_point;
					break_point_count++;
					msg_size = temp - break_point;
				}

			}
			alt_up_sd_card_fclose(handle);
		}
	}
	//debug
	/*printf("break point count: %d\n", break_point_count);
	for (temp = 0; temp < break_point_count; temp++){
		printf("break point %d: %d\n", temp,break_points[temp]);
	}*/
	temp = 0;
	/*
	 * ITERATION 3 : send msg to Android based on 'break_points'
	 */
	while(temp < break_point_count){
		file_index = 0;
		song_info = malloc(MAX_BYTES_PER_MESSAGE*sizeof(char));
		song_info[0] = '\0';
		assert(song_info != NULL);
		if (sd != NULL && alt_up_sd_card_is_Present() && alt_up_sd_card_is_FAT16()) {
			if ((handle = alt_up_sd_card_fopen(dir, false))>=0){
				while ( file_index < file_index_old ){ // skip to previous index
					alt_up_sd_card_read(handle);
					file_index++;
				}
				while(file_index <= break_points[temp]){ // while not reached break point
					ret = alt_up_sd_card_read(handle);
					assert(ret >= 0);
					file_index++;
					strcat(song_info,(char*)&ret);
				}
				alt_up_sd_card_fclose(handle);
			}
			else {
				printf("error opening file\n");
			}
		}
		//debug
		printf("song info from func\n%s\n", song_info);
		sendToAndroid(song_info);
		free(song_info);
		file_index_old = file_index; // save previous index
		temp++;
	}
	free(break_points);
	return;
}

void setFileId(char** file_names, int* file_count, char* dir){
	short int first_file = 0;
	char* file_name = '\0';
	printf("dir from setFileID = %s\n",dir);
	*file_count = 0;
	alt_up_sd_card_dev* sd = NULL;
	sd = alt_up_sd_card_open_dev("/dev/Altera_UP_SD_Card_Avalon_Interface_0");

	if (sd != NULL && alt_up_sd_card_is_Present() && alt_up_sd_card_is_FAT16()) {
		printf("Card connected.\n");
		while(1) {
			// finding the first file in directory
			if ((first_file == 0) && (alt_up_sd_card_find_first(dir, file_name)) == 0){
				printf("Listing WAV files found on disk\n");
				// identify current file is a WAV file
				if (strstr(file_name, ".WAV") != NULL){
					strcpy(file_names[*file_count] , file_name);
					(*file_count)++;
				}
				first_file = 1;
			}
			// finding the next file in directory
			else if((first_file == 1) && (alt_up_sd_card_find_next(file_name) == 0)){
				// identify current file is a WAV file
				if (strstr(file_name, ".WAV") != NULL){
					strcpy(file_names[*file_count] , file_name);
					(*file_count)++;
				}
			}
			else {
				printf("Finished listing WAV files\n");
				break;
			}
		}
	}
	return;
}


char* getFileName(char** file_names, int id){
	return file_names[id];
}

void parseCommand(volatile char* command, volatile short int* dj_or_playlist, volatile short int* volume, volatile short int* state,
		volatile unsigned int* file_id,volatile unsigned int* fid1, volatile unsigned int* fid2,
		volatile int* ch1_balance, volatile int* ch2_balance, volatile int* song1_speed,
		volatile int* song2_speed, volatile short int* state1, volatile short int* state2)
{
	*dj_or_playlist = (unsigned int) atoi((char*) &command[0]);
	if (*dj_or_playlist == 0){
		parseCommandPlaylist(command, volume, state, file_id);
	}
	else {
		parseCommandDJ(command, fid1, fid2, ch1_balance, ch2_balance,
					 song1_speed, song2_speed, state1, state2);
	}
}

void parseCommandDJ(volatile char* command, volatile unsigned int* fid1, volatile unsigned int* fid2,
		volatile int* ch1_balance, volatile int* ch2_balance, volatile int* song1_speed,
		volatile int* song2_speed, volatile short int* state1, volatile short int* state2)
{
	*fid1 = (unsigned int) atoi((char*) &command[2]);
	*fid2 = (unsigned int) atoi((char*) &command[5]);
	int bal = (unsigned int) atoi((char*) &command[8]);
	if(bal >= 0 && bal < 11 )
		*ch1_balance = -4;
	else if (bal >= 11 && bal < 22 )
		*ch1_balance = -3;
	else if (bal >= 22 && bal < 33 )
		*ch1_balance = -2;
	else if (bal >= 33 && bal < 44 )
		*ch1_balance = -1;
	else if (bal >= 44 && bal < 55 )
		*ch1_balance = 0;
	else if (bal >= 55 && bal < 66 )
		*ch1_balance = 1;
	else if (bal >= 66 && bal < 77)
		*ch1_balance = 2;
	else if (bal >= 77 && bal < 88)
		*ch1_balance = 3;
	else
		*ch1_balance = 4;

	bal = (unsigned int) atoi((char*) &command[11]);

	if(bal >= 0 && bal < 11 )
		*ch2_balance = -4;
	else if (bal >= 11 && bal < 22 )
		*ch2_balance = -3;
	else if (bal >= 22 && bal < 33 )
		*ch2_balance = -2;
	else if (bal >= 33 && bal < 44 )
		*ch2_balance = -1;
	else if (bal >= 44 && bal < 55 )
		*ch2_balance = 0;
	else if (bal >= 55 && bal < 66 )
		*ch2_balance = 1;
	else if (bal >= 66 && bal < 77)
		*ch2_balance = 2;
	else if (bal >= 77 && bal < 88)
		*ch2_balance = 3;
	else
		*ch2_balance = 4;

	*song1_speed = (unsigned int) atoi((char*) &command[14]);
	*song2_speed = (unsigned int) atoi((char*) &command[16]);
	*state1 = (unsigned int) atoi((char*) &command[18]);
	*state2 = (unsigned int) atoi((char*) &command[20]);
}

void parseCommandPlaylist(volatile char* command, volatile short int* volume,
		volatile short int* state,volatile unsigned int* file_id)
{
	int vol = atoi((char*)&command[2]);
	if (vol == 0)
		*volume = -1;
	else if(vol >= 0 && vol < 11 )
		*volume = 0;
	else if (vol >= 11 && vol < 22 )
		*volume = 1;
	else if (vol >= 22 && vol < 33 )
		*volume = 2;
	else if (vol >= 33 && vol < 44 )
		*volume = 3;
	else if (vol >= 44 && vol < 55 )
		*volume = 4;
	else if (vol >= 55 && vol < 66 )
		*volume = 5;
	else if (vol >= 66 && vol < 77)
		*volume = 6;
	else if (vol >= 77 && vol < 88)
		*volume = 7;
	else
		*volume = 8;
	*state = atoi((char*)&command[5]);
	*file_id = (unsigned int) atoi((char*)&command[7]);
	return;
}

void volumeAdjust (unsigned int* buffer, int volume, int buffer_size){
	int i;
	if (volume < 0){
		for (i = 0; i < buffer_size; i++)
			buffer[i] = 0;
	}
	else{
		for (i = 0; i < buffer_size; i++)
			buffer[i] = buffer[i] << volume;
	}
	return;
}

void balanceAdjust(unsigned int* input1, unsigned int* input2, int balance){
	if (balance > 0){
		if (balance == 4){
			*input2 = *input2 & 0x00000000;
		}
		else {
			*input2 = *input2 >> balance;
		}
	}
	else if (balance < 0){
		if (balance == -4){
			*input1 = *input1 & 0x00000000;
		}
		else {
			*input1 = *input1 >> abs(balance);
		}
	}
	else {
		*input1 = *input1;
		*input2 = *input2;
	}
}

int loadSong(alt_up_sd_card_dev* sd, char* fname, int* handle, int index) {
	short header[44];
	int size_of_file;
	//int sample_rate;
	//int song_index;
	//printf("fname loadsong = %s\n", fname); // debug
	int i;
	if (sd != NULL && alt_up_sd_card_is_Present() && alt_up_sd_card_is_FAT16()) {
		if ((*handle = alt_up_sd_card_fopen(fname, false))>=0)
			printf("File opened\n");
		else
			printf("File open error\n");
	}
	if ( alt_up_sd_card_is_Present() && alt_up_sd_card_is_FAT16()) {
		for (i = 0; i < index; i++) { // read header file
			short ret = alt_up_sd_card_read(*handle);
			//printf("handle %d\n", *handle); // debug
			assert(ret >= 0);
			header[i] = ret;
		}
	}
	//sample_rate = (header[27] << 24 | header[26] << 16 | header[25] << 8 | header[24]);
	//printf("sample rate: %ld\n", sample_rate);

	// calculate size of file
	size_of_file = (header[43] << 24 | header[42] << 16 | header[41] << 8 | header[40]);
	printf("size of file: %d bytes\n", size_of_file); //debug
	return size_of_file;
}


int calcSongLength(unsigned int size_of_file){
	return (int)(size_of_file / BYTES_PER_SECOND);
}



