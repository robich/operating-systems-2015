// note: compile this file by simply calling 'make'. A file a.out
// which you can execute will be created.
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "syscalls.h"

#define DEBUG 0 // prints all uuids generated
#define ITERATIONS_PER_THREAD 10
#define THREAD_NUMBER 4 // must divide ARRAY_SIZE

volatile int running_threads = 0;
pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;


 
void *startThread(void *vargp)
{
    // Store the value argument passed to this thread
    int myid = (int)vargp;
    
    if (DEBUG) {
    	printf("Thread %d starting...\n", myid);
    }

	if (0) {
		printf("get_unique_id returned an error!\n");
		exit(1);
	}
	
	int i;
	for (i = 0; i < ITERATIONS_PER_THREAD; i++) {
		long res;
		int child_pid = fork();
		
		if (child_pid < 0 )
			printf("Fork failed %i \n", child_pid);
		else if (child_pid > 0) {
			size_t limit = 3;
			size_t nr_children;
			pid_t pid_list[limit];

			// CASE : Normal execution, num_children < limit
			res= get_child_pids(pid_list, limit, &nr_children) ? errno : 0;
			
		}
	}
	
	pthread_mutex_lock(&running_mutex);
	running_threads--;
	pthread_mutex_unlock(&running_mutex);
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
    
    // Wait for all threads to finish working
    while (running_threads > 0) {
		sleep(1);
	}
	
	printf("Things look good!\n");
	
	pthread_exit(NULL);
    
    return 0;
}
