#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "common_threads.h"

typedef struct __barrier_t {
    sem_t mutex;
    sem_t gate;
    int count;
    int num_threads;
} barrier_t;

barrier_t b;

void barrier_init(barrier_t *b, int num_threads) {
    b->count = 0;
    b->num_threads = num_threads;
    Sem_init(&b->mutex, 1);
    Sem_init(&b->gate, 0);
}

void barrier(barrier_t *b) {
    Sem_wait(&b->mutex);
    b->count++;
    if (b->count == b->num_threads) {
        for (int i = 0; i < b->num_threads; i++) {
            Sem_post(&b->gate);
        }
    }
    Sem_post(&b->mutex);

    Sem_wait(&b->gate);
}

typedef struct __tinfo_t {
    int thread_id;
} tinfo_t;

void *child(void *arg) {
    tinfo_t *t = (tinfo_t *)arg;
    printf("child %d: before\n", t->thread_id);
    barrier(&b);
    printf("child %d: after\n", t->thread_id);
    return NULL;
}

int main(int argc, char *argv[]) {
    assert(argc == 2);
    int num_threads = atoi(argv[1]);
    assert(num_threads > 0);

    pthread_t p[num_threads];
    tinfo_t t[num_threads];

    printf("parent: begin\n");
    barrier_init(&b, num_threads);

    for (int i = 0; i < num_threads; i++) {
        t[i].thread_id = i;
        Pthread_create(&p[i], NULL, child, &t[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        Pthread_join(p[i], NULL);
    }

    Sem_destroy(&b.mutex);
    Sem_destroy(&b.gate);
    printf("parent: end\n");

    return 0;
}
