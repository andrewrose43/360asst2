#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

int items = 0; //The buffer

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
#define MAX_ITEMS 10 //Max size of buffer
#define NUM_ITERATIONS 200 //Items to pass through program
#define NUM_PRODUCERS 2 //Number of threads adding to buffer
#define NUM_CONSUMERS 2 //Number of threads removing from buffer
#define RUNS 50 //The number of times to run the threads, to flush out errors

void* producer(void* v){
	printf("Producer thread begins!\n");
	for (int i = 0; i < NUM_ITERATIONS; i++){
		pthread_mutex_lock(&mutex1);
		if (items < MAX_ITEMS){
			//iterations should only count if they successfully produced
			items++;
		}
		else {
			i--;
		}
		assert(0<=items && items <=MAX_ITEMS);
		printf("%d ", items);
		pthread_mutex_unlock(&mutex1);
	}
	return NULL;
}

void* consumer (void* v) {
	printf("Consumer thread begins!\n");
	for (int i=0; i<NUM_ITERATIONS; i++) {
		pthread_mutex_lock(&mutex1);
		if (items){
			items--;
			assert(0<=items && items <=MAX_ITEMS);
		}
		//iterations should only count if they successfully consumed
		else i--;
		printf("%d ", items);
		pthread_mutex_unlock(&mutex1);
	}
  return NULL;
}

int main(){
	int error_number;

	pthread_t pros[NUM_PRODUCERS];
	pthread_t cons[NUM_CONSUMERS];

	//Run it many times to flush out problems
	for (int run = 1; run <= RUNS; run++){

		printf("pc_mutex_cond_pthread run #%d\n", run);
	
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
			printf("Producer thread #%d is finished\n", i);
		}
		for (int i = 0; i < NUM_CONSUMERS; i++){
			pthread_join(cons[i], NULL);
			printf("Consumer thread #%d is finished\n", i);
		}
	}
	printf("All runs complete! AW YISS\n");
	exit(0);
}

