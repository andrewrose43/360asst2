#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

struct Agent {
  uthread_mutex_t mutex;
  uthread_cond_t  match;
  uthread_cond_t  paper;
  uthread_cond_t  tobacco;
  uthread_cond_t  smoke; //smoke is the convar signalled by smokers. agent waits for it
};

struct Agent* createAgent() {
  struct Agent* agent = malloc (sizeof (struct Agent));
  agent->mutex   = uthread_mutex_create();
  agent->paper   = uthread_cond_create (agent->mutex);
  agent->match   = uthread_cond_create (agent->mutex);
  agent->tobacco = uthread_cond_create (agent->mutex);
  agent->smoke   = uthread_cond_create (agent->mutex);
  return agent;
}

//Used to pass the correct resource into each thread
struct AgentPlus {
  Agent* ptr_agent;
  int r;
};

struct AgentPlus* createAgentPlus(Agent* a, int r_in){
  struct AgentPlus* ap = malloc (sizeof (struct AgentPlus));
  ap->ptr_agent = a;
  ap->r = r_in;
}

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource            {    MATCH = 1, PAPER = 2,   TOBACCO = 4};
char* resource_name [] = {"", "match",   "paper", "", "tobacco"};

int signal_count [5];  // # of times resource signalled
int smoke_count  [5];  // # of times smoker with resource smoked
uthread_t smokers[5];  // all the smokers!
uthread_t listeners[5];  // all the listeners!

int availables; //the variable where currently-available resources accumulate
uthread_cond_t need_tobacco;
uthread_cond_t need_match;
uthread_cond_t need_paper;

struct AgentPlus* ap; //used to pass a resource into each smoker and listener thread

//FUNCTION DECLARATIONS
void* wake_smoker(int availables);

void* agent (void* av) {
  struct Agent* a = av;
  static const int choices[]         = {MATCH|PAPER, MATCH|TOBACCO, PAPER|TOBACCO};
  static const int matching_smoker[] = {TOBACCO,     PAPER,         MATCH};
  
  uthread_mutex_lock (a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
      int r = random() % 3;
      signal_count [matching_smoker [r]] ++;
      int c = choices [r];
      if (c & MATCH) {
        VERBOSE_PRINT ("match available\n");
        uthread_cond_signal (a->match);
      }
      if (c & PAPER) {
        VERBOSE_PRINT ("paper available\n");
        uthread_cond_signal (a->paper);
      }
      if (c & TOBACCO) {
        VERBOSE_PRINT ("tobacco available\n");
        uthread_cond_signal (a->tobacco);
      }
      VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
      uthread_cond_wait (a->smoke);
    }
  uthread_mutex_unlock (a->mutex);
  return NULL;
}

void* listener (void* av){
  struct Agent* a = av;
  int r = a->resource_tmp;
  uthread_mutex_lock(a->mutex);
  for(;;){
    if (r==MATCH){
      uthread_cond_wait(a->match);
    }
    else if (r==PAPER){
      uthread_cond_wait(a->paper);
    }
    else { //r==TOBACCO
      uthread_cond_wait(a->tobacco);
    }
    availables += r;
    wake_smoker(availables);
  }
  uthread_mutex_unlock(a->mutex);
}

void* wake_smoker(int availables){
  switch(availables){
    case MATCH+PAPER: //wake tobacco smoker
      VERBOSE_PRINT ("Waking tobacco smoker\n");
      uthread_cond_signal(need_tobacco);
      break;
    case PAPER+TOBACCO: //wake match smoker
      VERBOSE_PRINT ("Waking match smoker\n");
      uthread_cond_signal(need_match);
      break;
    case TOBACCO+MATCH: //wake paper smoker
      VERBOSE_PRINT ("Waking paper smoker\n");
      uthread_cond_signal(need_paper);
      break;
    default: //wake ZILCH
      break;
  }
  availables=0;
}

void* smoker (void* av){
  struct Agent* a = av;
  int r = a->resource_tmp;
  uthread_mutex_lock(a->mutex);
  switch(r){
    case MATCH:
      for(;;){
        uthread_cond_wait(need_match);
	VERBOSE_PRINT("Match smoker is smoking\n");
	uthread_cond_signal(a->smoke);
	++smoke_count[MATCH];
      }
      break;
    case TOBACCO:
      for(;;){
        uthread_cond_wait(need_tobacco);
        VERBOSE_PRINT("Tobacco smoker is smoking\n");
        uthread_cond_signal(a->smoke);
        ++smoke_count[TOBACCO];
      }
      break;
    case PAPER:
      for(;;){
        uthread_cond_wait(need_paper);
        VERBOSE_PRINT("Paper smoker is smoking\n");
        uthread_cond_signal(a->smoke);
        ++smoke_count[PAPER];
      }
      break;
    default: //Do nothing
      break;
  }
  uthread_mutex_unlock(a->mutex);
}

int main (int argc, char** argv) {
  uthread_init (7);
  struct Agent*  a = createAgent();
  // Make the convars
  need_tobacco = uthread_cond_create(a->mutex);
  need_match = uthread_cond_create(a->mutex);
  need_paper = uthread_cond_create(a->mutex);
  // Create 2*3=6 threads, fed the three resources
  for (int r = 1; r < 5; r=r*2){
    // Pass the correct resource into each thread
    ap = createAgentPlus(a, r);
    listeners[r] = uthread_create(listener, ap);
    smokers[r] = uthread_create(smoker, ap);
    free(ap);
  }
  // Join the agent thread
  uthread_join (uthread_create (agent, a), 0);
  // Join the 6 other threads
  for (int r = 1; r < 5; r=r*2){
    uthread_join(listeners[r], NULL);
    uthread_join(smokers[r], NULL);
  }
  assert (signal_count [MATCH]   == smoke_count [MATCH]);
  assert (signal_count [PAPER]   == smoke_count [PAPER]);
  assert (signal_count [TOBACCO] == smoke_count [TOBACCO]);
  assert (smoke_count [MATCH] + smoke_count [PAPER] + smoke_count [TOBACCO] == NUM_ITERATIONS);
  printf ("Smoke counts: %d matches, %d paper, %d tobacco\n",
          smoke_count [MATCH], smoke_count [PAPER], smoke_count [TOBACCO]);
}
