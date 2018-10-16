/*
 * mfind.c
 *
 *  Created on: 16 Oct 2018
 *      Author: bram
 */

#include "list.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

int main(int argc, char **argv){
	int c;
	char *end_pointer;
	long int ret;
	int num_threads;


	while ((c = getopt (argc, argv, "t:p:")) != -1){
		switch (c){
			case 't':
				printf("Got type %s\n", optarg);
				if(strcmp(optarg, "f") == 0){
					printf("Got file!\n");
				}
				else if(strcmp(optarg, "d") == 0){
					printf("Got directory!\n");
				}
				else if(strcmp(optarg, "l") == 0){
					printf("Got link!\n");
				}
				else{
					fprintf(stderr, "Wrong type, got: %s\n", optarg);
				}
				break;
			case 'p':
				ret = strtol(optarg, &end_pointer, 10);

				//Check return value of strtol().
				if ((errno == ERANGE && \
						(ret == LONG_MAX || ret == LONG_MIN)) || \
						(errno != 0 && ret == 0)) {
					perror("strtol");
				  	exit(EXIT_FAILURE);
				}
				else if (end_pointer == optarg) {
					fprintf(stderr, "No digits were found\n");
					exit(EXIT_FAILURE);
				}

				//Check if the given strings fit in an int, else it's too big.
				if((ret > INT_MAX) && (ret < 1)){
					fprintf(stderr, "Number of threads exceeds int or is " \
							"negative!\n");
					exit(EXIT_FAILURE);
				}


				num_threads = (int)ret;
				printf("Got threads %d\n", num_threads);

				break;
			default:
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				exit(EXIT_FAILURE);
		}
	}



}
