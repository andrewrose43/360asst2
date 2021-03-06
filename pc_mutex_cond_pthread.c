#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>

#define MAX_ITEMS 10 //Max size of buffer
#define NUM_ITERATIONS 200 //Items to pass through program
#define NUM_PRODUCERS 2 //Number of threads adding to buffer
#define NUM_CONSUMERS 2 //Number of threads removing from buffer
#define RUNS 50 //The number of times to run the threads, to flush out errors

int items = 0; //The buffer
int producer_wait_count;
int consumer_wait_count;
int histogram[MAX_ITEMS+1];
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t item_available = PTHREAD_COND_INITIALIZER;
pthread_cond_t space_available = PTHREAD_COND_INITIALIZER;

void* producer(void* v){
	//printf("Producer thread begins!\n");
	for (int i = 0; i < NUM_ITERATIONS; ++i){
		pthread_mutex_lock(&mutex1);
		
		while (items==MAX_ITEMS){
			++producer_wait_count;
			pthread_cond_wait(&space_available, &mutex1);
		}
		++items;
		++histogram[items];
		assert(0<=items && items <=MAX_ITEMS);
		pthread_cond_signal(&item_available);
		
		pthread_mutex_unlock(&mutex1);
	}
	return NULL;
}

void* consumer (void* v) {
	//printf("Consumer thread begins!\n");
	for (int i=0; i<NUM_ITERATIONS; ++i) {
		pthread_mutex_lock(&mutex1);

		while (items==0){
			++consumer_wait_count;
			pthread_cond_wait(&item_available, &mutex1);
		}
		--items;
		++histogram[items];
		assert(0<=items && items <=MAX_ITEMS);
		pthread_cond_signal(&space_available);

		pthread_mutex_unlock(&mutex1);
	}
  return NULL;
}

int main(){
	int error_number;

	pthread_t pros[NUM_PRODUCERS];
	pthread_t cons[NUM_CONSUMERS];

	#ifdef REPEAT
	//Run it many times to flush out problems
	for (int run = 1; run <= RUNS; run++){
		printf("pc_mutex_cond_pthread run #%d\n", run);
		memset(histogram, 0, sizeof(histogram)); //all elements to 0; only necessary on repeating
	#endif
		
		//Resetting wait counters
		producer_wait_count = 0;
		consumer_wait_count = 0;
	
		for (int i = 0; i < NUM_PRODUCERS; ++i){
			if ((error_number=pthread_create(&pros[i], NULL, &producer, NULL))){
				printf("Producer thread creation failed: %d\n", error_number);
			}
		}
		for (int i = 0; i < NUM_CONSUMERS; ++i){
			if ((error_number=pthread_create(&cons[i], NULL, &consumer, NULL))){
				printf("Consumer thread creation failed: %d\n", error_number);
			}
		}

		//Wait until threads are done...
		for (int i = 0; i < NUM_PRODUCERS; ++i){
			pthread_join(pros[i], NULL);
			//printf("Producer thread #%d is finished\n", i);
		}
		for (int i = 0; i < NUM_CONSUMERS; ++i){
			pthread_join(cons[i], NULL);
			//printf("Consumer thread #%d is finished\n", i);
		}
	
		//Print the wait counters
		printf ("producer_wait_count=%d\nconsumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
		//Print the histogram
		printf ("items value histogram:\n");
		int sum=0;
		for (int i = 0; i <= MAX_ITEMS; i++) {
			printf ("  items=%d, %d times\n", i, histogram [i]);
			sum += histogram [i];
		}
		assert (sum==(NUM_PRODUCERS+NUM_CONSUMERS)*NUM_ITERATIONS);
	
	#ifdef REPEAT
		printf("\n\n");
	}
	printf("All runs complete!\n\n");
	#endif

	exit(0);
}

