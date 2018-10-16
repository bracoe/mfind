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
#include <semaphore.h>

/* Prototypes */
int parse_arguments(int argc, char **argv);
void remove_leftover_dirs_from_list();
void clean_up_and_exit(int exit_code);

/* Global list */
list *dirs_to_check;

/* The type to check for. 'f' for file, 'd' for directory and 'l' for link */
char search_for = 'f';

/* A global semaphore for the list protection */
sem_t *sem_list;



int main(int argc, char **argv){
	//Create list
	dirs_to_check = list_new();
	if(dirs_to_check == NULL){
		exit(EXIT_FAILURE);
	}

	//Create semaphore for the list.
	if(sem_init(sem_list, 0, 1) < 0){
		perror("semaphore");
		list_kill(dirs_to_check);
		exit(EXIT_FAILURE);
	}

	int num_threads = parse_arguments(argc, argv);

}

/**
 * parse_arguments() - Checks the correct arguments are passed to the program
 * and saves these arguments in the right position.
 *
 * @param argc The number of arguments given to the program.
 * @param argv An array of strings which are passed to the program.
 */
int parse_arguments(int argc, char **argv){
	int c;
	char *end_pointer;
	long int ret;
	int num_threads = 0;


	while ((c = getopt (argc, argv, "t:p:")) != -1){
		switch (c){
			case 't':
				printf("Got type %s\n", optarg);
				if(strcmp(optarg, "f") == 0){
					search_for = 'f';
					printf("Searching for file!\n");
				}
				else if(strcmp(optarg, "d") == 0){
					search_for = 'd';
					printf("Searching for directory!\n");
				}
				else if(strcmp(optarg, "l") == 0){
					search_for = 'l';
					printf("Searching for link!\n");
				}
				else{
					fprintf(stderr, "Wrong type, got: %s\n", optarg);
					clean_up_and_exit(EXIT_FAILURE);
				}
				break;
			case 'p':
				ret = strtol(optarg, &end_pointer, 10);

				//Check return value of strtol().
				if ((errno == ERANGE && \
						(ret == LONG_MAX || ret == LONG_MIN)) || \
						(errno != 0 && ret == 0)) {
					perror("strtol");
					clean_up_and_exit(EXIT_FAILURE);
				}
				else if (end_pointer == optarg) {
					fprintf(stderr, "No digits were found\n");
					clean_up_and_exit(EXIT_FAILURE);
				}

				//Check if the given strings fit in an int, else it's too big.
				if((ret > INT_MAX) && (ret < 1)){
					fprintf(stderr, "Number of threads exceeds int or is " \
							"negative!\n");
					clean_up_and_exit(EXIT_FAILURE);
				}


				num_threads = (int)ret;
				printf("Got threads %d\n", num_threads);

				break;
			default:
				fprintf (stderr, "Unknown option '-%c'.\n", optopt);
				clean_up_and_exit(EXIT_FAILURE);
		}
	}

	if(optind == argc){
		fprintf(stderr, "At least one start directory must be given!");
		clean_up_and_exit(EXIT_FAILURE);
	}
	else{
		for (int i = optind; i < argc; i++){
			list_append(argv[i], dirs_to_check); //No semaphore needed, yet...
			printf ("Argument %s\n", argv[i]);
		}
	}


	return num_threads;
}

/**
 * clean_up_and_exit() - Destroys the semaphore and frees the list.
 *
 * @param exit_code An exit which to exit with.
 */
void clean_up_and_exit(int exit_code){
	//semaphore
	if(sem_destroy(sem_list) < 0){
		perror("Semaphore");
	}

	//list
	remove_leftover_dirs_from_list();
	list_kill(dirs_to_check);
	exit(exit_code);
}

/**
 * remove_leftover_dirs_from_list() - Frees the directories still in the list.
 */
void remove_leftover_dirs_from_list(){
	list_pos first_pos = list_get_first_position(dirs_to_check);
	list_pos current_pos = \
	list_get_previous_position(list_get_last_position(dirs_to_check), \
			dirs_to_check);

	while(current_pos != first_pos){
		char *dir = (char*)list_get_value(current_pos);
		list_remove_element(current_pos, dirs_to_check);
		free(dir);

		current_pos = list_get_previous_position(current_pos, dirs_to_check);
	}
}

/**
 * add_dir_to_list() - Takes the list's semaphore and adds the given directory
 * to the list.
 * @param dir A directory to add to the list.
 * @return 0 if the directory was successfully added to the list, else -1.
 */
int add_dir_to_list(char *dir){

	if(sem_wait(sem_list) < 0){ //Take semaphore
		fprintf(stderr, "Could not take semaphore!");
	}

	list_append(dir, dirs_to_check);

	if(sem_post(sem_list) < 0){
		fprintf(stderr, "Could not release semaphore! Exiting to prevent " \
				"dead-lock!");
		clean_up_and_exit(EXIT_FAILURE);
	}

	return 0;
}

/**
 * get_dir_from_list() - Takes the list's semaphore and gets the latest
 * directory from the list.
 *
 * @returns A directory or NULL if the list is empty.
 */
char *get_dir_from_list(){
	char * dir = NULL;

	if(sem_wait(sem_list) < 0){ //Take semaphore
		fprintf(stderr, "Could not take semaphore!");
	}

	if(list_is_empty(dirs_to_check)){
		dir = NULL;
	}
	else{
		list_pos first_item_pos = \
				list_get_next_position(list_get_first_position(dirs_to_check), \
						dirs_to_check);

		dir = (char *)list_get_value(first_item_pos);

		list_remove_element(first_item_pos, dirs_to_check);
	}

	list_append(dir, dirs_to_check);

	if(sem_post(sem_list) < 0){
		fprintf(stderr, "Could not release semaphore! Exiting to prevent " \
				"dead-lock!");
		clean_up_and_exit(EXIT_FAILURE);
	}

	return dir;
}

