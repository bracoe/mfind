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
#include <sys/sysmacros.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <pthread.h>

/* Prototypes */
int parse_arguments(int argc, char **argv);
void remove_leftover_dirs_from_list(void);
void clean_up_and_exit(int exit_code);
void thread_and_start_search(int num_of_threads);
void initialize_list_and_thread_safety(void);
void *search_through_list(void *no);
char *get_dir_from_list(void);
void check_directory(char *dir);
void check_if_searching_for_this_file(char *file_path);
void add_dir_to_list(char *dir);
void inc_global_err_count(void);

/* Global list */
list *dirs_to_check;

/* The type to check for. 'f' for file, 'd' for directory and 'l' for link.
 * Set once, and only read afterwards. */
char search_for_type = 'f';

/* The filename which will be searched for. */
char *search_for_name;

/* A global semaphore for the list protection */
sem_t sem_list;
sem_t sem_err;

/*The gobal error count */
unsigned int err_count = 0;


int main(int argc, char **argv){

	initialize_list_and_thread_safety();

	int num_of_threads = parse_arguments(argc, argv);

	thread_and_start_search(num_of_threads);

	clean_up_and_exit(err_count);
}

/**
 * thread_and_start_search() -
 */
void thread_and_start_search(int num_of_threads){
	if(num_of_threads > 1){

		pthread_t thread_id[num_of_threads-1];

		for(int i = 0 ; i < num_of_threads-1; i++){
			if(pthread_create(&thread_id[i], NULL, search_through_list, NULL)){
				perror("pthread");
			}
		}

		search_through_list(NULL);

		for(int i = 0 ; i < num_of_threads-1; i++){
			if(pthread_join(thread_id[i], NULL)){
				perror("pthread");
			}
		}

	}
	else {
		search_through_list(NULL);
	}

}

/**
 * search_through_list() -
 */
void *search_through_list(void *no){ //TODO: Send in and return something?

	char *dir;
	unsigned long opened_dirs = 0;

	while((dir = get_dir_from_list()) != NULL){
		check_directory(dir);
		opened_dirs++;
		free(dir);
	}

	fprintf(stdout, "Thread: %lu Reads: %lu\n", pthread_self(), opened_dirs);
	return NULL;
}

/**
 * check_directory() -
 */
void check_directory(char *dir_path){
	struct dirent *dir_pointer;
	char file_path[PATH_MAX];

	DIR *dir_stream = opendir(dir_path);

	if (dir_stream == NULL) {
		perror(dir_path);
		inc_global_err_count();
		return;
	}

	while ((dir_pointer = readdir(dir_stream)) != NULL) {
		//fprintf(stdout, "Found: %s\n", dir_pointer->d_name);

		if((strcmp(dir_pointer->d_name,".") == 0) ||
				(strcmp(dir_pointer->d_name, "..") == 0)){
			continue;
		}

		strcpy(file_path, dir_path);
		strcat(file_path, "/");
		strcat(file_path, dir_pointer->d_name);

		check_if_searching_for_this_file(file_path);
	}

	if(closedir(dir_stream) < 0){
		perror(dir_path);
	}

}

/**
 * check_if_searching_for_this_file() -
 */
void check_if_searching_for_this_file(char *file_path){
	struct stat file_info;

	if (lstat(file_path, &file_info) < 0) {
		perror(file_path);
		return;
	}

	if (S_ISDIR(file_info.st_mode)) { //Check if dir
		if((search_for_type == 'd') &&
				(strcmp(basename(file_path),search_for_name) == 0)){
			fprintf(stdout, "%s\n", file_path);
		}

		add_dir_to_list(file_path);

	}
	else if(S_ISREG(file_info.st_mode)){ //Check if file
		if((search_for_type == 'f') &&
				(strcmp(basename(file_path),search_for_name) == 0)){
			fprintf(stdout, "%s\n", file_path);
		}
	}
	else if(S_ISLNK(file_info.st_mode)){ //Check if symlink
		if((search_for_type == 'l') &&
				(strcmp(basename(file_path),search_for_name) == 0)){
			fprintf(stdout, "%s\n", file_path);
		}
	}
	else{ //Something else
		//TODO: take care of this?
	}


}

/**
 * initialize_list_and_thread_safety() - Creates the list used for storing jobs
 * and initializes the semaphore needed for adding and removing from this list.
 */
void initialize_list_and_thread_safety(void){
	//Create list
	dirs_to_check = list_new();
	if(dirs_to_check == NULL){
		exit(EXIT_FAILURE);
	}

	//Create semaphore for the list.
	if(sem_init(&sem_list, 0, 1) < 0){
		perror("semaphore");
		list_kill(dirs_to_check);
		exit(EXIT_FAILURE);
	}

	if(sem_init(&sem_err, 0, 1) < 0){
		perror("semaphore");

		if(sem_destroy(&sem_list) < 0){
			perror("Semaphore");
		}

		list_kill(dirs_to_check);
		exit(EXIT_FAILURE);
	}

}



/**
 * parse_arguments() - Checks the correct arguments are passed to the program
 * and saves these arguments in the right position.
 *
 * @param argc The number of arguments given to the program.
 * @param argv An array of strings which are passed to the program.
 * @returns The number of threads wanted by the user.
 */
int parse_arguments(int argc, char **argv){
	int c;
	char *end_pointer;
	long int ret;
	int num_threads = 1;


	while ((c = getopt (argc, argv, "t:p:")) != -1){
		switch (c){
			case 't':
				printf("Got type %s\n", optarg);
				if(strcmp(optarg, "f") == 0){
					search_for_type = 'f';
					printf("Searching for file!\n");
				}
				else if(strcmp(optarg, "d") == 0){
					search_for_type = 'd';
					printf("Searching for directory!\n");
				}
				else if(strcmp(optarg, "l") == 0){
					search_for_type = 'l';
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

	//Get the start directories. Must be at least one.
	if(optind == argc){
		fprintf(stderr, "At least one start directory must be given!\n");
		clean_up_and_exit(EXIT_FAILURE);
	}
	else{
		for (int i = optind; i < argc-1; i++){
			add_dir_to_list(argv[i]);
		}
	}

	//Get the filname which to search for.
	search_for_name = argv[argc-1];

	return num_threads;
}

/**
 * clean_up_and_exit() - Destroys the semaphore and frees the list.
 *
 * @param exit_code An exit which to exit with.
 */
void clean_up_and_exit(int exit_code){
	//semaphore
	if(sem_destroy(&sem_list) < 0){
		perror("Semaphore");
	}

	if(sem_destroy(&sem_err) < 0){
		perror("Semaphore");
	}

	//list
	remove_leftover_dirs_from_list();
	list_kill(dirs_to_check);
	exit(exit_code);
}

/**
 *
 */
void inc_global_err_count(void){
	if(sem_wait(&sem_err) < 0){
			fprintf(stderr, "Could not take semaphore!");
	}

	err_count++;

	if(sem_post(&sem_err) < 0){
		fprintf(stderr, "Could not release semaphore! Could not update " \
				"error count");
	}
}

/**
 * remove_leftover_dirs_from_list() - Frees the directories still in the list.
 */
void remove_leftover_dirs_from_list(void){
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
 */
void add_dir_to_list(char *dir){
	if(sem_wait(&sem_list) < 0){ //Take semaphore
		fprintf(stderr, "Could not take semaphore!");
	}

	char *dir_string = (char*) malloc(strlen(dir) + 1);
	strcpy(dir_string,dir);

	if(dir_string == NULL){
		perror("malloc");
	}
	else{
		list_append(dir_string, dirs_to_check);
	}

	if(sem_post(&sem_list) < 0){ //Release semaphore
		fprintf(stderr, "Could not release semaphore! Exiting to prevent " \
				"dead-lock!");
		clean_up_and_exit(EXIT_FAILURE);
	}
}

/**
 * get_dir_from_list() - Takes the list's semaphore and gets the latest
 * directory from the list.
 *
 * @returns A directory or NULL if the list is empty.
 */
char *get_dir_from_list(void){
	char *dir = NULL;

	if(sem_wait(&sem_list) < 0){
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

	if(sem_post(&sem_list) < 0){
		fprintf(stderr, "Could not release semaphore! Exiting to prevent " \
				"dead-lock!");
		clean_up_and_exit(EXIT_FAILURE);
	}

	return dir;
}

