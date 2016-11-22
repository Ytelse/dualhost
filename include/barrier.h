#ifndef __BARRIER_H
#define __BARRIER_H

#include <pthread.h>

/* Need to implement pthread barriers as it is not implemented on MacOS */

typedef struct Barrier {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int count;
	int trip_count; /* Number of threads to syncronize */
} barrier_t;

void barrier_init(barrier_t* barrier, int count);
void barrier_wait(barrier_t* barrier);


#endif /* __BARRIER_H */

