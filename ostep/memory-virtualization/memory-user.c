#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <memory_in_MB> [duration_in_sec]\n", argv[0]);
        return 1;
    }

    // parse arguments
    long mem_mb = atol(argv[1]);
    long duration = -1; // run indefinitely by default

    if (argc >= 3) {
        duration = atol(argv[2]);
    }

    printf("Process ID: %d\n", getpid());

    // convert MB to bytes
    size_t size = mem_mb * 1024 * 1024;

    printf("Allocating %ld MB (%zu bytes)\n", mem_mb, size);

    // allocate memory
    char *array = (char *) malloc(size);
    if (!array) {
        perror("malloc failed");
        return 1;
    }

    // initialize memory (touch each page once)
    for (size_t i = 0; i < size; i += 4096) {
        array[i] = 0;
    }

    printf("Starting memory access loop...\n");

    time_t start = time(NULL);

    // repeatedly stream through memory
    while (1) {

        for (size_t i = 0; i < size; i += 64) {
            array[i]++;
        }

        // stop if duration specified
        if (duration > 0 && (time(NULL) - start) >= duration) {
            break;
        }
    }

    printf("Done.\n");

    free(array);
    return 0;
}
