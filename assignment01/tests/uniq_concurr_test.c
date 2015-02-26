// note: compile this file by simply calling 'make'. a file a.out
// which you can execute will be created.
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
# include "syscalls.h"

#define DEBUG 0 // prints all uuids generated
#define ARRAY_SIZE 20000000 // number of generated uuid
#define THREAD_NUMBER 4 // must divide ARRAY_SIZE

const int cells_to_fill = ARRAY_SIZE / THREAD_NUMBER;

volatile int running_threads = 0;
pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;

int array[ARRAY_SIZE];
 
void *startThread(void *vargp)
{
    // Store the value argument passed to this thread
    int myid = (int)vargp;
    
    printf("Thread %d starting...\n", myid);
    
    // Where to start filling array
    int start = myid * cells_to_fill;
    int end = start + cells_to_fill;
    
    int uuid;
    
    int i;
    for (i = start; i < end; i++) {
		if (get_unique_id(&uuid) != 0) {
			printf("get_unique_id returned an error!\n");
			exit(1);
		}
		array[i] = uuid;
	}
	
	pthread_mutex_lock(&running_mutex);
	running_threads--;
	pthread_mutex_unlock(&running_mutex);
}

// Returns 1 if array contains at least one repetition, and 0 otherwise
void arrayContainsDouble() {
	int i;
	int j;
	for (i = 0; i < ARRAY_SIZE; i++) {
		for (j = 0; j < ARRAY_SIZE; j++) {
			if ( (array[i] == array[j]) && (i != j) ) {
				printf("The element %d was generated twice!\n", array[i]);
			}
		}
	}
	
	printf("\nNo repetition found for %d calls!\n", ARRAY_SIZE);
}
 
int main()
{
    int i;
    pthread_t tid;
 
    // Create THREAD_NUMBER threads
    printf("Creating threads...\n");
    for (i = 0; i < THREAD_NUMBER ; i++) {
		pthread_mutex_lock(&running_mutex);
		running_threads++;
		pthread_mutex_unlock(&running_mutex);
		
        pthread_create(&tid, NULL, startThread, (void *)i);
    }
    
    // wait for all threads to finish working
    while (running_threads > 0) {
		sleep(1);
	}
    
    // Display array
    if (DEBUG) {
		printf("\nArray: \n");
		for (i = 0; i < ARRAY_SIZE; i++) {
			printf("[%d] = %d, ", i, array[i]);
		}
	}
	
	arrayContainsDouble();
	
	pthread_exit(NULL);
    
    return 0;
}
