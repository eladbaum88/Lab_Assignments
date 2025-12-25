
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>   


bool debug_on = true; // Global variable to control debug mode
FILE *infile = NULL; // Input file pointer initialized to standard input
FILE *outfile = NULL; // Output file pointer initialized to standard output
const unsigned char password[] = "my_password1";

const unsigned char *encoding_key = (const unsigned char *)"0";
int  encoding_sign  = +1; //add mode 
int  encoding_index = 0;   


// compare two strings for equality
int str_eq(const unsigned char *a, const unsigned char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
            ++a; // move to next character in a
            ++b; // move to next character in b
    }
    return (*a == '\0' && *b == '\0'); // both strings ended simultaneously
}

int encode(int c) { 

    unsigned char uc = (unsigned char)c;
    int key_digit = (int)(encoding_key[encoding_index] - '0');

    // making the key cyclic
    if (encoding_key[encoding_index + 1] == '\0') { 
        encoding_index = 0;        // wrap to start of key
    } else {
        encoding_index += 1;
    }

    int shift = encoding_sign * key_digit; // how mutch to shift value + or -   

    // apply to uppercase letters
    if (uc >= 'A' && uc <= 'Z') {
        int base = 'A'; // ASCII 65, for comfortable calculations
        int offset = (uc - base) + shift; 

        if (offset >= 26)
            offset = offset % 26;
        else if (offset < 0)
            offset = 26 + (offset % 26);

        return base + offset;
    }
    // apply to digits 
    if (uc >= '0' && uc <= '9') {
        int base = '0'; // ASCII 48, for comfortable calculations
        int offset = (uc - base) + shift; 

        if (offset >= 10)
            offset = offset % 10;
        else if (offset < 0)
            offset = 10 + (offset % 10);

        return base + offset;
    }

    return uc;

}

int main(int argc, char **argv) {

    infile  = stdin;
    outfile = stdout;

    bool next_debug_on = debug_on;
    
    for (int i = 0; i < argc; ++ i) {
        
        if (debug_on) {
            fprintf( stderr, "%s\n", argv[i]);
        }

        
        const unsigned char *curr_arg = (const unsigned char *)argv[i];

        // -D turn debug OFF 
        if (curr_arg[0] == '-' && curr_arg[1] == 'D' && curr_arg[2] == '\0') {
            next_debug_on = false;
        }

        // +D turn debug ON if password correct
        else if (curr_arg[0] == '+' && curr_arg[1] == 'D') {
            const unsigned char *pwd_from_arg = curr_arg + 2;  /* password starts after "+D" */
            if (str_eq(pwd_from_arg, password)) {
                next_debug_on = true;  /* correct password → enable for NEXT arg */
            } else 
                fprintf( stderr ,"wrong password\n"); // wrong password → no change */
        }

        else if (( curr_arg[0] == '-' || curr_arg[0] == '+' ) && curr_arg[1] == 'E') {
            // Change encoding mode
            const unsigned char *new_key = curr_arg + 2; // key starts after "-E" or "+E"
            if (*new_key != '\0') {
                encoding_key = new_key;
                encoding_sign = (curr_arg[0] == '-') ? -1 : +1;
                encoding_index = 0; // reset index for new key
                if (debug_on) {
                    fprintf( stderr, "encoding key set to %s, sign %c\n",
                            encoding_key, (encoding_sign > 0 ? '+' : '-'));
                }
            
            } else {
                if (debug_on)
                    fprintf( stderr, "empty encoding key - ignored\n"); 
            }
        }
        else if (curr_arg[0] == '-' && curr_arg[1] == 'i') {
            const char *fname = (const char *)(curr_arg + 2); // file name after "-i"
            if (*fname == '\0') {
                fprintf(stderr, "empty input file name\n");
                return 1;
            }
            FILE *f = fopen(fname, "r"); // open for reading

            if (!f) {
                fprintf(stderr, "cannot open input file %s\n", fname);
                return 1;
            }
            infile = f;

            if (debug_on) {
                fprintf(stderr, "input file set to %s\n", fname);
            }
        }
        else if (curr_arg[0] == '-' && curr_arg[1] == 'o') {
            const char *fname = (const char *)(curr_arg + 2); // file name after "-o"
            if (*fname == '\0') {
                fprintf(stderr, "empty output file name\n");
                return 1;
            }
            FILE *f = fopen(fname, "w");
            if (!f) {
                fprintf(stderr, "cannot open output file %s\n", fname);
                return 1;
            }
            outfile = f;

            if (debug_on) {
                fprintf(stderr, "output file set to %s\n", fname);
            }
        }

        // Apply debug mode change for next argument 
        debug_on = next_debug_on;
    }

    
    int ch;
    while ((ch = fgetc(infile)) != EOF) {
        ch = encode(ch);          
        fputc(ch, outfile);
    }

     // Close files if they are not stdin/stdout 
    if (infile != stdin) {
        fclose(infile);
    }
    if (outfile != stdout) {
        fclose(outfile);
    }
    return 0;
}