#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *data = malloc(sizeof(int) * 100);

    if (data == NULL) {
        perror("malloc");
        return 1;
    }

    data[10] = 1234;
    free(&data[10]);

    printf("Finished invalid free.\n");
    return 0;
}
