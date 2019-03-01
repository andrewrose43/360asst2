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

spinlock_t lock;	     // The spinlock
int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;

void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {

    spinlock_lock(&lock);
    
    //Spin on items until items<MAX_ITEMS and there's space in the buffer
    while(items==MAX_ITEMS){
      producer_wait_count = producer_wait_count+1;
      //Give the consumer an opportunity to grab the lock and add an item
      spinlock_unlock(&lock);
      spinlock_lock(&lock);
    }

    //Produce
    ++items;
    assert(0<=items && items <=MAX_ITEMS);

    ++histogram[items];
    
    spinlock_unlock(&lock);
  }
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    
    spinlock_lock(&lock);

    //Spin on items until items>0 and there is an item to consume
    while(items==0){
      consumer_wait_count = consumer_wait_count+1;
      //Give the producer an opportunity to grab the lock and add an item
      spinlock_unlock(&lock);
      spinlock_lock(&lock);
    }

    //Consume
    --items;
    assert(0<=items && items <=MAX_ITEMS);

    ++histogram[items];

    spinlock_unlock(&lock);
  }
  return NULL;
}

int main (int argc, char** argv) {
  uthread_t t[NUM_CONSUMERS+NUM_PRODUCERS];

  uthread_init(NUM_CONSUMERS+NUM_PRODUCERS);
  
  spinlock_create(&lock); //Initialize the spinlock

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
