#include <stdio.h>
#include <unistd.h>

#include "common_threads.h"

sem_t done;

static void *child(void *arg) {
    (void)arg;
    sleep(1);
    printf("child: post\n");
    Sem_post(&done);
    return NULL;
}

int main(void) {
    pthread_t thread;

    Sem_init(&done, 0);
    printf("parent: wait\n");
    Pthread_create(&thread, NULL, child, NULL);
    Sem_wait(&done);
    printf("parent: done\n");
    Pthread_join(thread, NULL);
    Sem_destroy(&done);

    return 0;
}
