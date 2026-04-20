#ifndef MUTEX_H
#define MUTEX_H

#include "types.h"
#include "thread.h"

typedef struct {
    volatile uint32_t lock;    /* 0 = free, 1 = held */
    tcb_t*            waiters; /* linked list of blocked threads */
} mutex_t;

void mutex_init(mutex_t* m);
void mutex_lock(mutex_t* m);
void mutex_unlock(mutex_t* m);

#endif /* MUTEX_H */
