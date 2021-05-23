#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include "zemaphore.h"

void zem_init(zem_t *s, int value) {
    s->value = value;
    s->cond = PTHREAD_COND_INITIALIZER;
    s->mutex = PTHREAD_MUTEX_INITIALIZER;
}

void zem_down(zem_t *s) {
    // lock the mutex 
    pthread_mutex_lock(&s->mutex);
    // wait if value is less than or equal to 0
    while(s->value <= 0)
        pthread_cond_wait(&s->cond, &s->mutex);
    
    // decrement the value
    s->value--;
    // unlock the mutex
    pthread_mutex_unlock(&s->mutex);
}

void zem_up(zem_t *s) {
    // lock the mutex 
    pthread_mutex_lock(&s->mutex);
    // increment the value
    s->value++;
    // signal the waiting variables
    pthread_cond_signal(&s->cond);
    // unlock the mutex
    pthread_mutex_unlock(&s->mutex);
}
