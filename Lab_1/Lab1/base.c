#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* TODO: Complete during task 2.a */
char* map(char *array, int array_length, char (*f) (char)) {
    if (array == NULL || array_length <= 0 || f == NULL) {
        return NULL;
    }
  // heap allocate memory for the mapped array  
  char* mapped_array = (char*)(malloc(array_length*sizeof(char)));
  if (mapped_array == NULL) {
      return NULL; // allocation failed
  } 
  // apply function f to each element of the input array
  for (int i = 0; i < array_length; i++) {
      mapped_array[i] = f(array[i]);
  }
  return mapped_array;
}
 
/* Ignores c, reads and returns a character from stdin using fgetc. */
char my_get(char c){
    (void)c; // to avoid unused parameter warning

    int ch = fgetc(stdin);   // fgetc returns int (to allow EOF = -1)
    if (ch == EOF) {
        return '\0';         
    }

    return (char)ch;
}

/* If c is a number between 0x20 and 0x7E, cprt prints the character of ASCII value c, 
otherwise, the dot ('.') character. 
This is followed by a space character, and then the value of c in a hexadecimal. 
Finally, a new line character is printed. 
After printing, cprt returns the value of c unchanged. */
char cxprt(char c){
    if (c >= 0x20 && c <= 0x7E) {
        printf("%c ", c);
    } else {
        printf(". ");
    }
    printf("%02X\n", (unsigned char)c);
    return c;
}

/* Gets a char c and returns its encrypted form by adding 1 to its value. 
If c is not between 0x1F and 0x7E it is returned unchanged */
char encrypt(char c){
    unsigned char uc = (unsigned char)c;
    if (c >= 0x20 && c <= 0x7E) {
        return uc + 1;
    } else {
        return c;
    }
}

/* Gets a char c and returns its decrypted form by reducing 1 from its value. 
If c is not between 0x21 and 0x7F it is returned unchanged */
char decrypt(char c){
    unsigned char uc = (unsigned char)c;
    if (c >= 0x21 && c <= 0x7F) {
        return uc - 1;
    } else {
        return c;
    }
}


/* dprt prints the value of c in a decimal 
representation followed by a new line, and returns c unchanged. */
char dprt(char c){
    printf("%d\n", (unsigned char)c);
    return c;
}

int main(int argc, char **argv){
  /* TODO: Test your code */
    printf("Please enter at least 5 characters:\n");
    int base_len = 5;
    char arr1[5];
    char* arr2 = map(arr1, base_len, my_get);
    char* arr3 = map(arr2, base_len, dprt);
    char* arr4 = map(arr3, base_len, cxprt);
   
    // encrypt
    char* enc = map(arr2, base_len, encrypt);
    // decrypt
    char* dec = map(enc, base_len, decrypt);

    printf("Original vs Encrypted vs Decrypted (decimal):\n");
    for (int i = 0; i < base_len; i++) {
        printf("i=%d: orig=%3d enc=%3d dec=%3d\n",
               i,
               (unsigned char)arr1[i],
               (unsigned char)enc[i],
               (unsigned char)dec[i]);
    }

    printf("Original chars:  ");
    for (int i = 0; i < base_len; i++) printf("%c", arr1[i]);
    printf("\n");

    printf("Encrypted chars: ");
    for (int i = 0; i < base_len; i++) printf("%c", enc[i]);
    printf("\n");

    printf("Decrypted chars: ");
    for (int i = 0; i < base_len; i++) printf("%c", dec[i]);
    printf("\n");


    free(arr2);
    free(arr3);
    free(arr4);
    free(enc);
    free(dec);
    return 0;
}


