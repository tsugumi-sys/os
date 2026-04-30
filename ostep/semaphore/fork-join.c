#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "common_threads.h"

sem_t s;

void *child(void *arg) {
    (void)arg;
    sleep(1);
    printf("child\n");
    Sem_post(&s);
    return NULL;
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    pthread_t p;

    printf("parent: begin\n");
    Sem_init(&s, 0);
    Pthread_create(&p, NULL, child, NULL);
    Sem_wait(&s);
    printf("parent: end\n");
    Pthread_join(p, NULL);
    Sem_destroy(&s);

    return 0;
}
