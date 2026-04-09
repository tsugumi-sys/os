#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int *data;
    size_t size;
    size_t capacity;
} IntVector;

typedef struct Node {
    int value;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    size_t size;
} IntList;

static double elapsed_seconds(struct timespec start, struct timespec end) {
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

static void die(const char *message) {
    perror(message);
    exit(1);
}

static void vector_init(IntVector *vector) {
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

static void vector_push(IntVector *vector, int value) {
    if (vector->size == vector->capacity) {
        size_t new_capacity = vector->capacity == 0 ? 4 : vector->capacity * 2;
        int *new_data = realloc(vector->data, new_capacity * sizeof(int));

        if (new_data == NULL) {
            free(vector->data);
            die("realloc");
        }

        vector->data = new_data;
        vector->capacity = new_capacity;
    }

    vector->data[vector->size++] = value;
}

static long long vector_sum(const IntVector *vector) {
    long long sum = 0;

    for (size_t i = 0; i < vector->size; i++) {
        sum += vector->data[i];
    }

    return sum;
}

static void vector_destroy(IntVector *vector) {
    free(vector->data);
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

static void list_init(IntList *list) {
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

static void list_push(IntList *list, int value) {
    Node *node = malloc(sizeof(Node));

    if (node == NULL) {
        die("malloc");
    }

    node->value = value;
    node->next = NULL;

    if (list->tail == NULL) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }

    list->size++;
}

static long long list_sum(const IntList *list) {
    long long sum = 0;

    for (Node *node = list->head; node != NULL; node = node->next) {
        sum += node->value;
    }

    return sum;
}

static void list_destroy(IntList *list) {
    Node *node = list->head;

    while (node != NULL) {
        Node *next = node->next;
        free(node);
        node = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

int main(int argc, char *argv[]) {
    size_t count = 1000000;
    IntVector vector;
    IntList list;
    struct timespec start;
    struct timespec end;
    double vector_push_time;
    double list_push_time;
    double vector_sum_time;
    double list_sum_time;
    long long vector_total;
    long long list_total;

    if (argc >= 2) {
        count = strtoull(argv[1], NULL, 10);
    }

    vector_init(&vector);
    list_init(&list);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < count; i++) {
        vector_push(&vector, (int)i);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    vector_push_time = elapsed_seconds(start, end);

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < count; i++) {
        list_push(&list, (int)i);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    list_push_time = elapsed_seconds(start, end);

    clock_gettime(CLOCK_MONOTONIC, &start);
    vector_total = vector_sum(&vector);
    clock_gettime(CLOCK_MONOTONIC, &end);
    vector_sum_time = elapsed_seconds(start, end);

    clock_gettime(CLOCK_MONOTONIC, &start);
    list_total = list_sum(&list);
    clock_gettime(CLOCK_MONOTONIC, &end);
    list_sum_time = elapsed_seconds(start, end);

    printf("elements: %zu\n", count);
    printf("vector: size=%zu capacity=%zu push=%.6f s sum=%.6f s total=%lld\n",
           vector.size, vector.capacity, vector_push_time, vector_sum_time,
           vector_total);
    printf("list:   size=%zu push=%.6f s sum=%.6f s total=%lld\n",
           list.size, list_push_time, list_sum_time, list_total);

    list_destroy(&list);
    vector_destroy(&vector);
    return 0;
}
