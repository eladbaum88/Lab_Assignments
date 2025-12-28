#include <stdio.h>

int count_digits(char *str) {
    int count = 0;
    if (str == NULL) {
        return 0;
    }

    while (*str) {
        if (*str >= '0' && *str <= '9')
            count++;
        str++;
    }
    return count;
}

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("Usage: %s <string>\n", argv[0]);
        return 1;
    }
    printf("String contains %d digits, right???\n", count_digits(argv[1]));
    return 0;
}
