#ifndef __common_threads_h__
#define __common_threads_h__

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>

#ifdef __linux__
#include <semaphore.h>
#endif

#define Pthread_create(thread, attr, start_routine, arg) assert(pthread_create(thread, attr, start_routine, arg) == 0)
#define Pthread_join(thread, value_ptr)                  assert(pthread_join(thread, value_ptr) == 0)

#define Pthread_mutex_init(m, v)                         assert(pthread_mutex_init(m, v) == 0)
#define Pthread_mutex_lock(m)                            assert(pthread_mutex_lock(m) == 0)
#define Pthread_mutex_unlock(m)                          assert(pthread_mutex_unlock(m) == 0)

#define Pthread_cond_init(cond, v)                       assert(pthread_cond_init(cond, v) == 0)
#define Pthread_cond_signal(cond)                        assert(pthread_cond_signal(cond) == 0)
#define Pthread_cond_wait(cond, mutex)                   assert(pthread_cond_wait(cond, mutex) == 0)

#define Mutex_init(m)                                    assert(pthread_mutex_init(m, NULL) == 0)
#define Mutex_lock(m)                                    assert(pthread_mutex_lock(m) == 0)
#define Mutex_unlock(m)                                  assert(pthread_mutex_unlock(m) == 0)

#define Cond_init(cond)                                  assert(pthread_cond_init(cond, NULL) == 0)
#define Cond_signal(cond)                                assert(pthread_cond_signal(cond) == 0)
#define Cond_wait(cond, mutex)                           assert(pthread_cond_wait(cond, mutex) == 0)

#ifndef __linux__
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    unsigned int value;
} sem_t;

static inline int sem_init(sem_t *sem, int pshared, unsigned int value) {
    if (sem == NULL || pshared != 0) {
        errno = EINVAL;
        return -1;
    }

    int rc = pthread_mutex_init(&sem->mutex, NULL);
    if (rc != 0) {
        errno = rc;
        return -1;
    }

    rc = pthread_cond_init(&sem->cond, NULL);
    if (rc != 0) {
        pthread_mutex_destroy(&sem->mutex);
        errno = rc;
        return -1;
    }

    sem->value = value;
    return 0;
}

static inline int sem_wait(sem_t *sem) {
    if (sem == NULL) {
        errno = EINVAL;
        return -1;
    }

    int rc = pthread_mutex_lock(&sem->mutex);
    if (rc != 0) {
        errno = rc;
        return -1;
    }

    while (sem->value == 0) {
        rc = pthread_cond_wait(&sem->cond, &sem->mutex);
        if (rc != 0) {
            pthread_mutex_unlock(&sem->mutex);
            errno = rc;
            return -1;
        }
    }

    sem->value--;

    rc = pthread_mutex_unlock(&sem->mutex);
    if (rc != 0) {
        errno = rc;
        return -1;
    }

    return 0;
}

static inline int sem_post(sem_t *sem) {
    if (sem == NULL) {
        errno = EINVAL;
        return -1;
    }

    int rc = pthread_mutex_lock(&sem->mutex);
    if (rc != 0) {
        errno = rc;
        return -1;
    }

    if (sem->value == UINT_MAX) {
        pthread_mutex_unlock(&sem->mutex);
        errno = EOVERFLOW;
        return -1;
    }

    sem->value++;
    rc = pthread_cond_signal(&sem->cond);
    if (rc != 0) {
        pthread_mutex_unlock(&sem->mutex);
        errno = rc;
        return -1;
    }

    rc = pthread_mutex_unlock(&sem->mutex);
    if (rc != 0) {
        errno = rc;
        return -1;
    }

    return 0;
}

static inline int sem_destroy(sem_t *sem) {
    if (sem == NULL) {
        errno = EINVAL;
        return -1;
    }

    int rc = pthread_cond_destroy(&sem->cond);
    if (rc != 0) {
        errno = rc;
        return -1;
    }

    rc = pthread_mutex_destroy(&sem->mutex);
    if (rc != 0) {
        errno = rc;
        return -1;
    }

    return 0;
}
#endif

#define Sem_init(sem, value)                             assert(sem_init(sem, 0, value) == 0)
#define Sem_wait(sem)                                    assert(sem_wait(sem) == 0)
#define Sem_post(sem)                                    assert(sem_post(sem) == 0)
#define Sem_destroy(sem)                                 assert(sem_destroy(sem) == 0)

#endif
