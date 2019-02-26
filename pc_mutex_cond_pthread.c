#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
#define MAX_ITEMS 10
#define NUM_ITERATIONS 200
#define NUM_PRODUCERS 2
#define NUM_CONSUMERS 2

int items = 0;

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
	printf("pc_mutex_cond_pthread runs!");
	exit(0);
}

