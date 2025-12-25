#include <stdio.h>
#include <stdlib.h>

#define CHUNK_SIZE 16   /* number of bytes to read at a time */

/* Print `length` bytes from `buffer` in hex, separated by spaces */
void PrintHex(const unsigned char *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        printf("%02X", buffer[i]);    // two-digit hex, uppercase
        if (i + 1 < length) {
            printf(" ");
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) { // expects program name and file name
        fprintf(stderr, "Usage: %s FILE\n", argv[0]);
        return 1; 
    }

    const char *filename = argv[1]; // input file name
    FILE *fp = fopen(filename, "rb");   // "rb" for binary files
    if (!fp) {
        perror("fopen");
        return 1;
    }

    unsigned char buffer[CHUNK_SIZE];
    size_t bytesRead;
    int first = 1;

    while ((bytesRead = fread(buffer,1, CHUNK_SIZE, fp)) > 0) { // read chunk
        if (!first) {
            printf(" ");                // space between chunks
        }
        PrintHex(buffer, bytesRead); // print the bytes in hex
        first = 0;
    }

    printf("\n");
    fclose(fp);
    return 0;
}
