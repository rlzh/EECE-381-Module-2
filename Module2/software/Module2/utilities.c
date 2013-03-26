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

void loadSongInfo(){
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
	alt_up_sd_card_dev* sd = NULL;
	sd = alt_up_sd_card_open_dev("/dev/Altera_UP_SD_Card_Avalon_Interface_0");
	/*
	 * ITERATION 1: count the # of files
	 * 				count the max # of breakpoints
	 * 				count the # of bytes
	 */
	if (sd != NULL && alt_up_sd_card_is_Present() && alt_up_sd_card_is_FAT16()) {
		if ((handle = alt_up_sd_card_fopen("INFO.TXT", false))>=0){
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
		if ((handle = alt_up_sd_card_fopen("INFO.TXT", false))>=0){
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
			if ((handle = alt_up_sd_card_fopen("INFO.TXT", false))>=0){
				while ( file_index < file_index_old ){ // skip to previous index
					alt_up_sd_card_read(handle);
					file_index++;
				}
				while(file_index <= break_points[temp]){ // while not reached break point
					ret = alt_up_sd_card_read(handle);
					assert(ret >= 0);
					file_index++;
					strcat(song_info,&ret);
				}
				alt_up_sd_card_fclose(handle);
			}
			else {
				printf("error opening file\n");
			}
		}
		//debug
		//printf("song info from func\n%s\n", song_info);
		sendToAndroid(song_info);
		free(song_info);
		file_index_old = file_index; // save previous index
		temp++;
	}
	free(break_points);
	return;
}

void setFileId(char** file_names, int* file_count){
	short int first_file = 0;
	char* file_name;

	*file_count = 0;
	alt_up_sd_card_dev* sd = NULL;
	sd = alt_up_sd_card_open_dev("/dev/Altera_UP_SD_Card_Avalon_Interface_0");

	if (sd != NULL && alt_up_sd_card_is_Present() && alt_up_sd_card_is_FAT16()) {
		printf("Card connected.\n");
		while(1) {
			// finding the first file in directory
			if ((first_file == 0) && (alt_up_sd_card_find_first("", file_name)) == 0){
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

void parseCommand(volatile char* command, volatile short int* volume,
				  volatile short int* state,volatile unsigned int* file_id/*,
				  volatile unsigned int* song_index*/)
{
	int vol = atoi((char*)&command[0]);
	if(vol >= 0 && vol < 14 )
		*volume = -4;
	else if (vol >= 14 && vol < 28 )
		*volume = -3;
	else if (vol >= 28 && vol < 42 )
		*volume = -2;
	else if (vol >= 42 && vol < 56 )
		*volume = -1;
	else if (vol >= 56 && vol < 70 )
		*volume = 0;
	else if (vol >= 70 && vol < 84 )
		*volume = 1;
	else
		*volume = 2;
	*state = atoi((char*)&command[3]);
	*file_id = (unsigned int) atoi((char*)&command[5]);
	//*song_index = (unsigned int) atoi ((char*)&command[7]);
	return;
}


void volumecontrol(unsigned int* buf, volatile short int* volume, int buffersize) {
	int i;

	for (i = 0; i < buffersize; i++) {
		if (*volume == 2)
		{
			if (buf[i] == 0x007FFFFF)
			{
				buf [i] = 0x007FFFFF;
			}
			else
			{
				buf[i] = buf[i] << 2;
			}
		}
		if (*volume == 1)
		{
			if (buf[i] == 0x007FFFFF)
			{
				buf [i] = 0x007FFFFF;
			}
			else
			{
				buf[i] = buf[i] << 1;
			}
		}
		if (*volume == 0)
		{
			buf[i] = buf [i];
		}
		if (*volume == -1) {
			if (buf[i] >= 0x00800000) {
				buf[i] = buf[i] >> 1;
				buf[i] = (buf[i] | 0x00800000);
			} else {
				buf[i] = buf[i] >> 1;
			}
		}
		if (*volume == -2) {
			if (buf[i] >= 0x00800000) {
				buf[i] = buf[i] >> 2;
				buf[i] = (buf[i] | 0x00C00000);// & 0x00DFFFFF;
			} else {
				buf[i] = buf[i] >> 2;
			}
		}
		if (*volume == -3){
			if (buf[i] >= 0x00800000) {
				buf[i] = buf[i] >> 3;
				buf[i] = (buf[i] | 0x00E00000);
			} else {
				buf[i] = buf[i] >> 3;
			}
		}
		if (*volume == -4){
			if (buf[i] >= 0x00800000) {
				buf[i] = buf[i] >> 4;
				buf[i] = (buf[i] | 0x00F00000);
			} else {
				buf[i] = buf[i] >> 4;
			}
		}
	}
}

unsigned int loadSong(char* fname,int* handle, int index) {
	short header[44];
	unsigned int size_of_file;
	int i;
	alt_up_sd_card_dev* sd = NULL;
	sd = alt_up_sd_card_open_dev("/dev/Altera_UP_SD_Card_Avalon_Interface_0");
	if (sd != NULL && alt_up_sd_card_is_Present() && alt_up_sd_card_is_FAT16()) {
		*handle = alt_up_sd_card_fopen(fname, false);
		assert(*handle >= 0);
	}
	for (i = 0; i < index; i++) { // read header file
		short ret = alt_up_sd_card_read(*handle);
		assert(ret >= 0);
		header[i] = ret;
	}
	//int sample_rate = (header[27] << 24 | header[26] << 16 | header[25] << 8 | header[24]);
	//printf("sample rate: %ld\n", sample_rate);
	size_of_file = (header[43] << 24 | header[42] << 16 | header[41] << 8 | header[40]);
	//printf("size of file: %d\n", song_size); //debug
	return size_of_file;
}

unsigned int calcSongLength(unsigned int size_of_file){
	return (unsigned int)(size_of_file / BYTES_PER_SECOND);
}



