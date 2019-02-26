#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int items = 0; //The buffer

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
#define MAX_ITEMS 10 //Max size of buffer
#define NUM_ITERATIONS 200 //Items to pass through program
#define NUM_PRODUCERS 2 //Number of threads adding to buffer
#define NUM_CONSUMERS 2 //Number of threads removing from buffer
#define RUNS 50 //The number of times to run the threads, to flush out errors

void* producer(void* v){
	for (int i = 0; i < NUM_ITERATIONS; i++){
		pthread_mutex_lock(&mutex1);
		items++;
		pthread_mutex_unlock(&mutex1);
	}
	return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
	  if (items){
		  pthread_mutex_lock(&mutex1);
		  items--;
		  pthread_mutex_unlock(&mutex1);
	  }
	  //iterations should only count if they successfully consumed
	  else i--;
  }
  return NULL;
}

int main(){
	int error_number;

	pthread_t pros[NUM_PRODUCERS];
	pthread_t cons[NUM_CONSUMERS];

	//Run it many times to flush out problems
	for (int run = 1; run <= RUNS; run++){
	
		for (int i = 0; i < NUM_PRODUCERS; i++){
			if ((error_number=pthread_create(&pros[i], NULL, &producer, NULL))){
				printf("Producer thread creation failed: %d\n", error_number);
			}
		}
		for (int i = 0; i < NUM_CONSUMERS; i++){
			if ((error_number=pthread_create(&cons[i], NULL, &consumer, NULL))){
				printf("Consumer thread creation failed: %d\n", error_number);
			}
		}

		//Wait until threads are done...
		for (int i = 0; i < NUM_PRODUCERS; i++){
			pthread_join(pros[i], NULL);
		}
		for (int i = 0; i < NUM_CONSUMERS; i++){
			pthread_join(cons[i], NULL);
		}
		printf("pc_mutex_cond_pthread run #%d\n", run);
	}
	printf("All runs complete! AW YISS\n");
	exit(0);
}

