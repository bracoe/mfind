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

#define UNUSED(x) (void)(x)

/* Prototypes */
int parse_arguments(int argc, char **argv);
void remove_leftover_dirs_from_list(void);
void clean_up_and_exit(int exit_code);
void thread_and_start_search(int num_of_threads);
void initialize_list(void);
void *search_through_list(void *not_used __attribute__((unused)));
char *get_dir_from_list(void);
void check_directory(char *dir);
void check_file(char *file_path);
void add_dir_to_list(char *dir);
void inc_global_err_count(void);
void initialize_sem_active_threads(int threads);
void initialize_sem_err_count(void);
void add_argument_to_list_if_sym_link(char *arg);
void check_input_argument(char *arg);

/* Global list */
list *dirs_to_check;

/* The type to check for. 'f' for file, 'd' for directory and 'l' for link.
 * Set once, and only read afterwards. Default 'a' is for all types.*/
char search_for_type = 'a';

/* The filename which will be searched for. */
char *search_for_name;

/* A global semaphore for the list protection */
sem_t sem_list;
sem_t sem_err;
sem_t sem_active_threads;

/*The gobal error count */
unsigned int err_count = 0;


int main(int argc, char **argv){

	initialize_list();

	initialize_sem_err_count();

	int num_of_threads = parse_arguments(argc, argv);

	initialize_sem_active_threads(num_of_threads);

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
void *search_through_list(void *not_used __attribute__((unused))){

	char *dir;
	unsigned long opened_dirs = 0;
	int active_threads = 0;

	do{
		while((dir = get_dir_from_list()) != NULL){
			//Release semaphore to show thread is active
			if(sem_post(&sem_active_threads) < 0){
				fprintf(stderr, "Could not release semaphore! Exiting to prevent " \
						"dead-lock!");
				clean_up_and_exit(EXIT_FAILURE);
			}


			check_directory(dir);
			opened_dirs++;
			free(dir);

			if(sem_wait(&sem_active_threads) < 0){ //Take semaphore
				fprintf(stderr, "Could not take semaphore!");
				clean_up_and_exit(EXIT_FAILURE);
			}
		}

		//Get sem value to see if thread should end
		if(sem_getvalue(&sem_active_threads, &active_threads) != 0){
			perror("sem_getvalue");
			fprintf(stderr, "Thread could not get sem value. Killing thread!");
			active_threads = 0;
		}
	}while(active_threads);

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
		if(errno != EACCES){
			inc_global_err_count();
		}
		perror(dir_path);

		return;
	}

	while ((dir_pointer = readdir(dir_stream)) != NULL) {

		if((strcmp(dir_pointer->d_name,".") == 0) ||
				(strcmp(dir_pointer->d_name, "..") == 0)){
			continue;
		}

		strcpy(file_path, dir_path);
		strcat(file_path, "/");
		strcat(file_path, dir_pointer->d_name);

		check_file(file_path);
	}

	if(closedir(dir_stream) < 0){
		perror(dir_path);
	}

}

/**
 * check_file() -
 */
void check_file(char *file_path){
	struct stat file_info;

	if (lstat(file_path, &file_info) < 0) {
		perror(file_path);
		return;
	}

	if (S_ISDIR(file_info.st_mode)) { //Check if dir
		if(((search_for_type == 'd')||(search_for_type == 'a')) &&
				(strcmp(basename(file_path),search_for_name) == 0)){
			fprintf(stdout, "%s\n", file_path);
		}

		add_dir_to_list(file_path);

	}
	if(S_ISREG(file_info.st_mode)){ //Check if file
		if(((search_for_type == 'f')||(search_for_type == 'a')) &&
				(strcmp(basename(file_path),search_for_name) == 0)){
			fprintf(stdout, "%s\n", file_path);
		}
	}
	if(S_ISLNK(file_info.st_mode)){ //Check if symlink
		if(((search_for_type == 'l')||(search_for_type == 'a')) &&
				(strcmp(basename(file_path),search_for_name) == 0)){
			fprintf(stdout, "%s\n", file_path);
		}
	}
	/* else{
		Something else, we don't care about
	}*/


}

/**
 * initialize_sem_active_threads() -
 */
void initialize_sem_active_threads(int threads){
	//Create semaphore for the list.

	if(sem_init(&sem_active_threads, 0, threads) < 0){
		perror("semaphore");

		if(sem_destroy(&sem_list) < 0){
			perror("Semaphore");
		}

		if(sem_destroy(&sem_err) < 0){
			perror("Semaphore");
		}

		list_kill(dirs_to_check);
		exit(EXIT_FAILURE);
	}

	//Take the active thread count down
	for(int i = 0 ; i < threads ; i++){
		if(sem_wait(&sem_active_threads) < 0){ //take semaphore
			fprintf(stderr, "Could not take semaphore! Exiting to prevent " \
					"further errors!");
			clean_up_and_exit(EXIT_FAILURE);
		}
	}
}

void initialize_sem_err_count(void){
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
 * initialize_list() - Creates the list used for storing jobs
 * and initializes the semaphore needed for adding and removing from this list.
 */
void initialize_list(void){
	//Create list
	dirs_to_check = list_new();
	if(dirs_to_check == NULL){
		exit(EXIT_FAILURE);
	}

	if(sem_init(&sem_list, 0, 1) < 0){
		perror("semaphore");
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
				if(strcmp(optarg, "f") == 0){
					search_for_type = 'f';
				}
				else if(strcmp(optarg, "d") == 0){
					search_for_type = 'd';
				}
				else if(strcmp(optarg, "l") == 0){
					search_for_type = 'l';
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
				//printf("Got threads %d\n", num_threads);

				break;
			default:
				fprintf (stderr, "Unknown option '-%c'.\n", optopt);
				clean_up_and_exit(EXIT_FAILURE);
		}
	}

	//Get the filname which to search for.
	search_for_name = argv[argc-1];

	//Get the start directories. Must be at least one.
	if(optind == argc -1){
		fprintf(stderr, "At least one start directory must be given!\n");
		clean_up_and_exit(EXIT_FAILURE);
	}
	else{
		for (int i = optind; i < argc-1; i++){
			check_input_argument(argv[i]);
		}
	}



	return num_threads;
}

void check_input_argument(char *arg){
	check_file(arg);

	/* Normal directories are added by check_file */
	add_argument_to_list_if_sym_link(arg);
}

void add_argument_to_list_if_sym_link(char *arg){
	struct stat file_info;

	if (lstat(arg, &file_info) < 0) {
		perror(arg);
		return;
	}

	if(S_ISLNK(file_info.st_mode)){ //Check if symlink
		add_dir_to_list(arg);
	}
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

	if(sem_destroy(&sem_active_threads) < 0){
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

