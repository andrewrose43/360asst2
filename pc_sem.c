#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_sem.h"

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;

uthread_sem_t mutex1; //The mutex!
uthread_sem_t empty; //number of free item slots
uthread_sem_t full; //number of occupied item slots

void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    uthread_sem_wait(empty); //wait until there is a space to fill
    uthread_sem_wait(mutex1); //wait until the lock can be grabbed
    ++items; 
    assert(0<=items && items <=MAX_ITEMS);
    ++histogram[items];
    uthread_sem_signal(mutex1); //release the lock
    uthread_sem_signal(full); //there is now one more full slot
  }
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    uthread_sem_wait(full); //wait until there are items to consume
    uthread_sem_wait(mutex1); //wait until the lock can be grabbed
    --items; 
    assert(0<=items && items <=MAX_ITEMS);
    ++histogram[items];
    uthread_sem_signal(mutex1); //release the lock
    uthread_sem_signal(empty); //there is now one more empty slot
  }
  return NULL;
}

int main (int argc, char** argv) {
  uthread_t t[4];

  uthread_init (4);

  mutex1 = uthread_sem_create(1);
  empty = uthread_sem_create(MAX_ITEMS);
  full = uthread_sem_create(0);

 //Create the threads!
  for (int i = 0; i < NUM_PRODUCERS; ++i){
    t[i] = uthread_create(producer, 0);
  }
  for (int i = 0; i < NUM_CONSUMERS; ++i){
    t[NUM_PRODUCERS+i] = uthread_create(consumer, 0);
  }

  //Join the threads!
  for (int i = 0; i < NUM_PRODUCERS+NUM_CONSUMERS; ++i){
    uthread_join(t[i], NULL);
  }

  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (t) / sizeof (uthread_t) * NUM_ITERATIONS);
}
