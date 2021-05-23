#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>
#include "zemaphore.h"

#define NUM_THREADS 3
#define NUM_ITER 10

// define zemaphore array to take care of each threads
zem_t zem[NUM_THREADS];

void *justprint(void *data) {
  	int thread_id = *((int *)data);

	for(int i=0; i < NUM_ITER; i++) {
		// check if the previous thread has printed or not
		zem_down(&zem[(NUM_THREADS+thread_id-1)%NUM_THREADS]);
		
		printf("This is thread %d\n", thread_id);

		// update zemaphore value of current thread
		zem_up(&zem[thread_id]);
	}
  	return 0;
}

int main(int argc, char *argv[]) {

	pthread_t mythreads[NUM_THREADS];
	int mythread_id[NUM_THREADS];

	// initialize all the thread's zemaphore value as 0 except for the last thread, which is initialized as 1
	for(int i = 0; i < NUM_THREADS-1; i++) {
		zem_init(&zem[i], 0);
	}
	zem_init(&zem[NUM_THREADS-1], 1);
  
	for(int i = 0; i < NUM_THREADS; i++) {
		mythread_id[i] = i;
		pthread_create(&mythreads[i], NULL, justprint, (void *)&mythread_id[i]);
	}
  
	for(int i = 0; i < NUM_THREADS; i++) {
		pthread_join(mythreads[i], NULL);
	}
	
	return 0;
}
