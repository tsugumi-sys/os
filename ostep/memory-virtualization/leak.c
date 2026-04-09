#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *data = malloc(sizeof(int) * 100);

    if (data == NULL) {
        perror("malloc");
        return 1;
    }

    data[0] = 42;
    printf("Allocated memory, first value = %d\n", data[0]);

    return 0;
}
