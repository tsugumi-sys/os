#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common_threads.h"

typedef struct __rwlock_t {
    sem_t lock;
    sem_t writelock;
    sem_t turnstile;
    int readers;
} rwlock_t;

void rwlock_init(rwlock_t *rw) {
    rw->readers = 0;
    Sem_init(&rw->lock, 1);
    Sem_init(&rw->writelock, 1);
    Sem_init(&rw->turnstile, 1);
}

void rwlock_acquire_readlock(rwlock_t *rw) {
    Sem_wait(&rw->turnstile);
    Sem_post(&rw->turnstile);

    Sem_wait(&rw->lock);
    rw->readers++;
    if (rw->readers == 1) {
        Sem_wait(&rw->writelock);
    }
    Sem_post(&rw->lock);
}

void rwlock_release_readlock(rwlock_t *rw) {
    Sem_wait(&rw->lock);
    rw->readers--;
    if (rw->readers == 0) {
        Sem_post(&rw->writelock);
    }
    Sem_post(&rw->lock);
}

void rwlock_acquire_writelock(rwlock_t *rw) {
    Sem_wait(&rw->turnstile);
    Sem_wait(&rw->writelock);
}

void rwlock_release_writelock(rwlock_t *rw) {
    Sem_post(&rw->writelock);
    Sem_post(&rw->turnstile);
}

int loops;
int value = 0;

rwlock_t lock;

void *reader(void *arg) {
    (void)arg;

    for (int i = 0; i < loops; i++) {
        rwlock_acquire_readlock(&lock);
        printf("read %d\n", value);
        usleep(1000);
        rwlock_release_readlock(&lock);
    }
    return NULL;
}

void *writer(void *arg) {
    (void)arg;

    for (int i = 0; i < loops; i++) {
        rwlock_acquire_writelock(&lock);
        value++;
        printf("write %d\n", value);
        usleep(1000);
        rwlock_release_writelock(&lock);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    assert(argc == 4);
    int num_readers = atoi(argv[1]);
    int num_writers = atoi(argv[2]);
    loops = atoi(argv[3]);

    pthread_t pr[num_readers];
    pthread_t pw[num_writers];

    rwlock_init(&lock);

    printf("begin\n");

    for (int i = 0; i < num_readers; i++) {
        Pthread_create(&pr[i], NULL, reader, NULL);
    }
    for (int i = 0; i < num_writers; i++) {
        Pthread_create(&pw[i], NULL, writer, NULL);
    }

    for (int i = 0; i < num_readers; i++) {
        Pthread_join(pr[i], NULL);
    }
    for (int i = 0; i < num_writers; i++) {
        Pthread_join(pw[i], NULL);
    }

    Sem_destroy(&lock.lock);
    Sem_destroy(&lock.writelock);
    Sem_destroy(&lock.turnstile);

    printf("end: value %d\n", value);

    return 0;
}
