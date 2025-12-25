#include <stdio.h>
#include <stdlib.h>

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
 
char my_get(char c){
    (void)c; // to avoid a warning

    int ch = fgetc(stdin);   // fgetc returns int (to allow EOF = -1)
    if (ch == EOF) {
        return 0;         
    }

    return (char)ch;
}

char cxprt(char c){
    if (c >= 0x20 && c <= 0x7E) {
        printf("%c ", c);
    } else {
        printf(". ");
    }
    printf("%02X\n", (unsigned char)c);
    return c;
}

char encrypt(char c){
    unsigned char uc = (unsigned char)c;
    if (c >= 0x20 && c <= 0x7E) {
        return uc + 1;
    } else {
        return c;
    }
}

char decrypt(char c){
    unsigned char uc = (unsigned char)c;
    if (c >= 0x21 && c <= 0x7F) {
        return uc - 1;
    } else {
        return c;
    }
}

char dprt(char c){
    printf("%d\n", (unsigned char)c);
    return c;
}

struct fun_desc {
    char *name;
    char (*fun)(char);
};

struct fun_desc menu[] = {
    { "Get String", my_get  },
    { "Print Decimal", dprt },
    { "Print Char and hexadecimal", cxprt },
    { "Encrypt", encrypt  },
    { "Decrypt", decrypt  },
    { NULL, NULL}   // terminator
};

int main(void) {

    #define base_len 5 
    char *carray = calloc(base_len, sizeof(char));  // calloc all bytes to 0
    if (!carray) {
        perror("calloc failed");
        return 1;
    }

    int menu_len = 0;
    while (menu[menu_len].name != NULL) {
        menu_len++;
    }


    char line[128];
    while(1) {
        
        printf("Select operation number from the following menu:\n");
        for (int i = 0; i < menu_len; i++) {
            printf("%d) %s\n", i, menu[i].name);
            }
        printf("Option (number): ");


        if (fgets(line, sizeof(line), stdin) == NULL) {
        // EOF
            printf("EOF detected, exiting.\n");
            break;
        }

        int choice;

        if (sscanf(line, "%d", &choice) != 1) {
        printf("Not within bounds\n");
        break;
    }

        if (choice < 0 || choice >= menu_len) {
            printf("Not within bounds\n");
            break;
        }

        printf("Within bounds\n");

        char *new_carray = map(carray, base_len, menu[choice].fun);
        if (!new_carray) {
            printf("map failed\n");
            break;
        }

        free(carray);
        carray = new_carray;
        
    }
    free(carray);
    return 0;
    
    
    
    
    
    
    
    
    
    
    
    
    /*while (1) {
        printf("Enter a word:\n");

        // Try to read a line from stdin
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            // EOF or error → exit normally
            printf("EOF detected, exiting.\n");
            break;
        printf("You entered: %s", buffer);
        // Loop continues and asks again
    }
   }

        // For now, just echo what was typed (optional, for debugging)
     
    return 0; */
}
