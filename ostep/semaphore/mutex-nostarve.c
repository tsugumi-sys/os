#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common_threads.h"

typedef struct __ns_mutex_t {
    sem_t lock;
    sem_t *turns;
    int next_ticket;
    int now_serving;
    int max_tickets;
} ns_mutex_t;

typedef struct __worker_arg_t {
    int thread_id;
} worker_arg_t;

int loops;
int counter = 0;
int in_critical = 0;
ns_mutex_t mutex;

void ns_mutex_init(ns_mutex_t *m, int max_tickets) {
    assert(max_tickets > 0);

    m->turns = malloc(sizeof(sem_t) * max_tickets);
    assert(m->turns != NULL);

    m->next_ticket = 0;
    m->now_serving = 0;
    m->max_tickets = max_tickets;
    Sem_init(&m->lock, 1);

    for (int i = 0; i < max_tickets; i++) {
        Sem_init(&m->turns[i], 0);
    }
}

void ns_mutex_destroy(ns_mutex_t *m) {
    for (int i = 0; i < m->max_tickets; i++) {
        Sem_destroy(&m->turns[i]);
    }
    free(m->turns);
    Sem_destroy(&m->lock);
}

void ns_mutex_acquire(ns_mutex_t *m) {
    Sem_wait(&m->lock);
    assert(m->next_ticket < m->max_tickets);
    int my_ticket = m->next_ticket++;
    if (my_ticket == m->now_serving) {
        Sem_post(&m->lock);
        return;
    }
    Sem_post(&m->lock);

    Sem_wait(&m->turns[my_ticket]);
}

void ns_mutex_release(ns_mutex_t *m) {
    Sem_wait(&m->lock);
    m->now_serving++;
    if (m->now_serving < m->next_ticket) {
        Sem_post(&m->turns[m->now_serving]);
    }
    Sem_post(&m->lock);
}

void *worker(void *arg) {
    worker_arg_t *worker_arg = (worker_arg_t *)arg;

    for (int i = 0; i < loops; i++) {
        ns_mutex_acquire(&mutex);

        assert(in_critical == 0);
        in_critical = 1;

        int old = counter;
        usleep(1000);
        counter = old + 1;
        printf("thread %d: counter %d\n", worker_arg->thread_id, counter);

        in_critical = 0;
        ns_mutex_release(&mutex);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    assert(argc == 3);
    int num_threads = atoi(argv[1]);
    loops = atoi(argv[2]);
    assert(num_threads > 0);
    assert(loops > 0);

    pthread_t threads[num_threads];
    worker_arg_t args[num_threads];

    ns_mutex_init(&mutex, num_threads * loops);

    printf("parent: begin\n");
    for (int i = 0; i < num_threads; i++) {
        args[i].thread_id = i;
        Pthread_create(&threads[i], NULL, worker, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        Pthread_join(threads[i], NULL);
    }
    printf("parent: end counter %d\n", counter);

    ns_mutex_destroy(&mutex);

    return 0;
}
