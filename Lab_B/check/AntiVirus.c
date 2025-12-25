#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// structure to hold virus signature information
typedef struct virus { 
    unsigned short SigSize;
    char virusName[16];
    unsigned char* sig;
} virus;

typedef struct link link;
struct link {
    link *nextVirus;
    virus *vir;
};

// global variables 
link *SignaturesList = NULL;
int g_is_big_endian_file = 0; // global variable to indicate endianness of the file

// buffers
#define INPUT_BUFFER_SIZE 256 // buffer size for user input
#define FILENAME_BUFFER 256 // buffer size for filename input

#define SUSPECTED_BUFFER_SIZE 10000 // buffer size for suspected file scan
unsigned char suspectedBuffer[SUSPECTED_BUFFER_SIZE];
unsigned int suspectedSize = 0;


unsigned short convert_size(unsigned short size_from_file) {
    if (!g_is_big_endian_file) {
        return size_from_file;  // VIRL on little-endian machine
    }
    return (size_from_file >> 8) | (size_from_file << 8);
}

void freeVirus(virus *v) {
    if (v != NULL) {
        free(v->sig);
        free(v);
    }
}

void printSignature(virus *v, FILE *output) {
    if (!v) return;

    fprintf(output, "Virus name: %s\n", v->virusName);
    fprintf(output, "Virus size: %u\n", v->SigSize);
    fprintf(output, "Signature:\n");

    // Temporarily redirect stdout-like behavior
    for (int i = 0; i < v->SigSize; i++) {
        fprintf(output, "%02X", v->sig[i]);
        if (i + 1 < v->SigSize) {
            fprintf(output, " ");
        }
    }
    fprintf(output, "\n\n");
}

virus* readSignature(FILE *fp) {
    virus *v = (virus *)malloc(sizeof(virus)); // allocate memory for virus struct
    if (v == NULL) {
        perror("malloc");
        exit(1);
    }

    // Read first 18 bytes: 2 for SigSize, 16 for virusName
    size_t bytes_read = fread(&v->SigSize, 1, 18, fp); 
    if (bytes_read == 0) {
        // No more data at all – EOF
        free(v);
        return NULL;
    }
    if (bytes_read < 18) {
        // Partial 
        free(v);
        return NULL;
    }

    // Ensure virusName is null-terminated
    v->virusName[15] = '\0';

    // Convert size according to endianness
    v->SigSize = convert_size(v->SigSize);

    // Allocate memory for the signature
    v->sig = (unsigned char *)malloc(v->SigSize);
    if (v->sig == NULL) {
        perror("malloc");
        free(v);
        exit(1);
    }

    // Read the signature bytes
    bytes_read = fread(v->sig, 1, v->SigSize, fp);
    if (bytes_read < v->SigSize) {
        // Not enough bytes – file truncated/corrupt
        free(v->sig);
        free(v);
        return NULL;
    }

    return v;
}

link* list_append(link* virus_list, virus* data) {
    link *newLink = (link *)malloc(sizeof(link));
    if (newLink == NULL) {
        perror("malloc");
        exit(1);
    }
    newLink->vir = data;
    newLink->nextVirus = NULL;

    if (virus_list == NULL) {
        /* List was empty – new node is now the head */
        return newLink;
    }

    /* Walk to the end and append */
    link *curr = virus_list;
    while (curr->nextVirus != NULL) {
        curr = curr->nextVirus;
    }
    curr->nextVirus = newLink;
    return virus_list;  /* Head stays the same */
}

void list_print(link *virus_list, FILE *output) {
    link *curr = virus_list;
    while (curr != NULL) {
        printSignature(curr->vir, output);
        curr = curr->nextVirus;
    }
}

void list_free(link *virus_list) {
    link *curr = virus_list;
    while (curr != NULL) {
        link *next = curr->nextVirus;
        freeVirus(curr->vir);   /* free the virus (sig + struct) */
        free(curr);             /* free the list node itself */
        curr = next;
    }
}

void detect_virus() {

    if (suspectedSize == 0 || SignaturesList == NULL) {
        return; // nothing to scan or no signatures loaded
    }

    
    // Loop\Scan the suspectedBuffer against all signatures
    for (link *curr = SignaturesList; curr != NULL; curr = curr->nextVirus) {
        virus *v = curr->vir;
        // skip invalid signatures
        if (v == NULL || v->sig == NULL || v->SigSize == 0) {
            continue;
        }

        // If signature size is larger than suspected buffer, skip
        unsigned short sig_len = v->SigSize;
        if (sig_len > suspectedSize) {
            continue;
        }

        // Scan through suspectedBuffer
        
        for (unsigned int i = 0; i + sig_len <= suspectedSize; i++) {
            // Compare bytes
            if (memcmp(suspectedBuffer + i, v->sig, sig_len) == 0) {
                printf("Starting byte location: %u\n", i);
                printf("Virus name: %s\n", v->virusName);
                printf("Virus size: %u\n\n", v->SigSize);
            }
        }
       
    }

}

void LoadSignatures() {

    char filename[FILENAME_BUFFER];

    printf("Enter signatures file name:\n");
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        return;  // user hit EOF or error
    }

    /* Strip newline from filename */
    char *newline = strchr(filename, '\n');
    if (newline) {
        *newline = '\0';
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return;
    }

    /* Read magic: 4 bytes "VIRL" or "VIRB" */
    char magic[4];
    if (fread(magic, 1, 4, fp) < 4) {
        fprintf(stderr, "Error: signatures file too short \n");
        fclose(fp);
        return;
    }

    if (memcmp(magic, "VIRL", 4) == 0) {
        g_is_big_endian_file = 0;
    } else if (memcmp(magic, "VIRB", 4) == 0) {
        g_is_big_endian_file = 1;
    } else {
        fprintf(stderr, "Error: bad magic in signatures file\n");
        fclose(fp);
        return;
    }

    /* Free any previous list */
    if (SignaturesList != NULL) {
        list_free(SignaturesList);
        SignaturesList = NULL;
    }

    /* Read all viruses and append to the list */
    virus *v;
    while ((v = readSignature(fp)) != NULL) {
        SignaturesList = list_append(SignaturesList, v);
    }

    fclose(fp);
    printf("Signatures loaded.\n");
}

void PrintSignatures() {
    if (SignaturesList == NULL) {
        printf("No signatures.\n");
        return;
    }
    list_print(SignaturesList, stdout);
}

void Detect_viruses() {

     if (SignaturesList == NULL) {
        printf("No signatures.\n");
        return;
    }

    char filename[FILENAME_BUFFER];

    printf("Enter suspected file name:\n");
    if (fgets(filename,sizeof(filename), stdin) == NULL) {
        return;  // user hit EOF or error
    }
    
    // remove the newline from the end of file name
    char *newline = strchr(filename, '\n');
    if (newline) {
        *newline = '\0';
    }
    // Open the suspected file in binary mode
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return;
    }
    
    // Read up to 10K bytes into the global buffer, byte by byte
    size_t bytes_read = fread(suspectedBuffer, 1, SUSPECTED_BUFFER_SIZE, fp);
    if (ferror(fp)) {
        perror("fread");
        fclose(fp);
        return;
    }
    fclose(fp);

    suspectedSize = (unsigned int)bytes_read;

    // Now scan using the global buffer + signatures list
    detect_virus();

}

void neutralize_virus(char *fileName, int signatureOffset) {

    FILE *fp = fopen(fileName, "r+b");  // open for read + write
    if (!fp) {
        perror("fopen");
        return;
    }

    if (fseek(fp, signatureOffset, SEEK_SET) != 0) { // seek to offset
        perror("fseek");
        fclose(fp);
        return;
    }

    unsigned char ret = 0xC3;  // hex value for RET instruction

    if (fwrite(&ret, 1, 1, fp) != 1) { // write RET byte
        perror("fwrite"); // failed to write
        
    }

    fclose(fp);
}


void Fix_file() {

    if (SignaturesList == NULL) {
        printf("No signatures. \n");
        return;
    }

    char filename[FILENAME_BUFFER];

    printf("Enter suspected file name:\n");
    if (fgets(filename,sizeof(filename), stdin) == NULL) {
        return;  // user hit EOF or error
    }
    
    // remove the newline from the end of file name
    char *newline = strchr(filename, '\n');
    if (newline) {
        *newline = '\0';
    }
    // Open the suspected file in binary mode
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return;
    }
    
    // Read up to 10K bytes into the global buffer, byte by byte
    size_t bytes_read = fread(suspectedBuffer, 1, SUSPECTED_BUFFER_SIZE, fp);
    if (ferror(fp)) {
        perror("fread");
        fclose(fp);
        return;
    }
    fclose(fp);

    suspectedSize = (unsigned int)bytes_read;


    if (suspectedSize == 0) { // signature file checked before
        printf("Suspected file is empty or could not be read.\n");
        return;
    }

    // scan buffer for all viruses, and neutralize each occurrence
    for (link *curr = SignaturesList; curr != NULL; curr = curr->nextVirus) {
        virus *v = curr->vir;
        if (v == NULL || v->sig == NULL || v->SigSize == 0) {
            continue;
        }

        unsigned short sig_len = v->SigSize;
        if (sig_len > suspectedSize) {
            continue;
        }

        for (unsigned int i = 0; i + sig_len <= suspectedSize; i++) {
            if (memcmp(suspectedBuffer + i, v->sig, sig_len) == 0) {
                printf("Neutralizing virus %s at offset %u\n", v->virusName, i);
                neutralize_virus(filename, (int)i);
            }
        }
    }

}

void AI_analysis_of_file() {
    printf("Not implemented.\n");
}

void Quit() {
    if (SignaturesList != NULL) {
        list_free(SignaturesList);
        SignaturesList = NULL;
    }
    exit(0);
}

struct fun_desc {
    char *name;
    void (*fun)();
};

struct fun_desc menu[] = {
    { "Load signatures",     LoadSignatures },
    { "Print signatures",    PrintSignatures },
    { "Detect viruses",      Detect_viruses },
    { "Fix file",            Fix_file }, 
    { "AI analysis of file", AI_analysis_of_file  },    
    { "Quit",                Quit },
    { NULL,                  NULL }          /* terminator */
};


int main(int argc, char **argv) {
    (void)argc; // unused
    (void)argv; // unused

    char line[INPUT_BUFFER_SIZE];
    int menu_len = 0; // number of menu options

    while (menu[menu_len].name != NULL) {
        menu_len++;
    }

    while (1) {
        printf(" select operation from the menu: \n");
        for (int i = 0; i < menu_len; i++) {
            printf("%d) %s\n", i + 1 , menu[i].name);
        }
        printf("Option (number): ");

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("EOF encountered, exiting.\n");
            break;
        }
        int choice;
        if (sscanf(line, "%d", &choice) != 1) {
            printf("Invalid input\n");
            continue;
        }

        if (choice <= 0 || choice > menu_len) {
            printf("Not within bounds\n");
            continue;;
        }

        printf("Within bounds\n");
        if (menu[choice - 1].fun != NULL) {
            menu[choice - 1].fun();
            continue;
        }

        // If we somehow exit the loop without calling Quit(), clean
        if (SignaturesList != NULL) {
            list_free(SignaturesList);
            SignaturesList = NULL;
        }

        return 0;
    }
}