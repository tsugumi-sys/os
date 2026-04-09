#include <stdio.h>
#include <stdlib.h>

int main(void) {
    int *data = malloc(sizeof(int) * 100);

    if (data == NULL) {
        perror("malloc");
        return 1;
    }

    data[100] = 0;
    printf("Wrote past the end of the array.\n");

    free(data);
    return 0;
}
