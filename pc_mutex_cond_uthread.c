#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"
#include "spinlock.h"
#include <string.h>

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int items = 0;
uthread_mutex_t mutex1;      // The mutex!
uthread_cond_t item_available; //convar 1
uthread_cond_t space_available; //convar 2
int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {

    uthread_mutex_lock(mutex1);
 
    //Spin on items until items<MAX_ITEMS and there's space in the buffer
    while(items==MAX_ITEMS){
      producer_wait_count = producer_wait_count+1;
      uthread_cond_wait(space_available);
    }

    //Produce
    ++items;
    uthread_cond_signal(item_available);
    ++histogram[items];
    assert(0<=items && items <=MAX_ITEMS);
    
    uthread_mutex_unlock(mutex1);
  }
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    
    uthread_mutex_lock(mutex1);

    //Spin on items until items>0 and there is an item to consume
    while(items==0){
      consumer_wait_count = consumer_wait_count+1;
      uthread_cond_wait(item_available);
    }

    //Consume
    --items;
    uthread_cond_signal(space_available);
    ++histogram[items];
    assert(0<=items && items <=MAX_ITEMS);
    uthread_mutex_unlock(mutex1);
  }
  return NULL;
}

int main (int argc, char** argv) {
  uthread_t t[NUM_CONSUMERS+NUM_PRODUCERS];

  uthread_init(NUM_CONSUMERS+NUM_PRODUCERS);
  
  mutex1 = uthread_mutex_create();

  item_available = uthread_cond_create(mutex1);
  space_available = uthread_cond_create(mutex1);

  //Resetting key variables
  producer_wait_count = 0;
  consumer_wait_count = 0;
  
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

  printf ("producer_wait_count=%d\nconsumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (t) / sizeof (uthread_t) * NUM_ITERATIONS);
}
