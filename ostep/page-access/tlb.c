#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#define REPEAT 20000000

static inline uint64_t now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + ts.tv_nsec;
}

void shuffle(size_t *arr, size_t n) {
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = (size_t)(rand() % (int)(i + 1));
        size_t tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

double bench_random_pointer_chase(char *buf, size_t page_size, size_t pages) {
    size_t *order = malloc(pages * sizeof(size_t));
    if (!order) {
        perror("malloc");
        exit(1);
    }

    for (size_t i = 0; i < pages; i++) {
        order[i] = i;
    }
    shuffle(order, pages);

    // 各ページ先頭に「次に行くページ番号」を格納
    for (size_t i = 0; i < pages; i++) {
        size_t cur = order[i];
        size_t next = order[(i + 1) % pages];
        *(size_t *)(buf + cur * page_size) = next;
    }

    size_t p = order[0];

    // warmup
    for (size_t i = 0; i < pages * 20; i++) {
        p = *(size_t *)(buf + p * page_size);
    }

    uint64_t start = now_ns();
    for (size_t i = 0; i < REPEAT; i++) {
        p = *(size_t *)(buf + p * page_size);
    }
    uint64_t end = now_ns();

    volatile size_t sink = p;
    (void)sink;

    free(order);
    return (double)(end - start) / REPEAT;
}

double bench_sequential_pages(char *buf, size_t page_size, size_t pages) {
    volatile uint64_t sum = 0;

    // warmup
    for (size_t r = 0; r < 20; r++) {
        for (size_t p = 0; p < pages; p++) {
            sum += *(uint64_t *)(buf + p * page_size);
        }
    }

    size_t total_accesses = pages * 10000;

    uint64_t start = now_ns();
    for (size_t r = 0; r < 10000; r++) {
        for (size_t p = 0; p < pages; p++) {
            sum += *(uint64_t *)(buf + p * page_size);
        }
    }
    uint64_t end = now_ns();

    (void)sum;
    return (double)(end - start) / total_accesses;
}

int main() {
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    srand(42);

    printf("page size = %zu bytes\n\n", page_size);
    printf("%-8s %-12s %-18s %-18s\n",
           "pages", "footprint", "seq ns/access", "rand ns/access");

    for (size_t pages = 32; pages <= 16384; pages *= 2) {
        size_t total_size = pages * page_size;

        char *buf = aligned_alloc(page_size, total_size);
        if (!buf) {
            perror("aligned_alloc");
            return 1;
        }

        // 各ページ先頭に適当な値を入れる
        for (size_t p = 0; p < pages; p++) {
            *(uint64_t *)(buf + p * page_size) = (uint64_t)p;
        }

        double seq_ns = bench_sequential_pages(buf, page_size, pages);
        double rand_ns = bench_random_pointer_chase(buf, page_size, pages);

        printf("%-8zu %-8.2f MB %-18.2f %-18.2f\n",
               pages,
               (double)total_size / (1024.0 * 1024.0),
               seq_ns,
               rand_ns);

        free(buf);
    }

    return 0;
}
