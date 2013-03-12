/*
 * main.c
 *
 *  Created on: Mar 12, 2013
 *
 */

#include <stdio.h>
#include "altera_up_avalon_rs232.h"
#include <string.h>
#include "system.h"

int main()
{
		int i;
		unsigned char data;
		unsigned char parity;
        unsigned char message[] = "EECE381 is so much fun";

        printf("UART Initialization\n");
        alt_up_rs232_dev* uart = alt_up_rs232_open_dev(RS232_0_NAME);

        while (1){
			printf("Clearing read buffer to start\n");
			while (alt_up_rs232_get_used_space_in_read_FIFO(uart)) {
				alt_up_rs232_read_data(uart, &data, &parity);
			}

			printf("Waiting for data to come back from the Middleman\n");
			while (alt_up_rs232_get_used_space_in_read_FIFO(uart) == 0);

			// First byte is the number of characters in our message

			alt_up_rs232_read_data(uart, &data, &parity);
			int num_to_receive = (int)data;
			unsigned char temp[num_to_receive];

			printf("About to receive %d characters:\n", num_to_receive);

			for (i = 0; i < num_to_receive; i++) {
				while (alt_up_rs232_get_used_space_in_read_FIFO(uart) == 0);
				alt_up_rs232_read_data(uart, &data, &parity);

				temp[num_to_receive-i-1] = data;

				printf("%c", data);
			}
			printf("\n");

			printf("Sending the message to the Middleman\n");

			// Start with the number of bytes in our message

			alt_up_rs232_write_data(uart, (unsigned char) strlen(temp));

			// Now send the actual message to the Middleman

			for (i = 0; i < strlen(temp); i++) {
					alt_up_rs232_write_data(uart, temp[i]);
			}
			//printf("Message Echo Complete\n");
        }
        return 0;
}


