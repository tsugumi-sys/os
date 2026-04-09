#include <stdio.h>

int main(void) {
    int *ptr = NULL;

    printf("About to dereference a NULL pointer...\n");
    printf("%d\n", *ptr);

    return 0;
}
