/* hexeditplus.c - Lab 4 Task 0b (final)
 * Menu scheme based on Lab 1 (struct array + {NULL,NULL} terminator).
 * Debug mode is a boolean.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>

#define FILE_NAME_MAX 127
#define MEM_BUF_SIZE 10000

/* Required global state */
bool debug_mode = false;
bool display_decimal = false;  /* false = hexadecimal (default), true = decimal */


char file_name[128] = {0};          /* empty string initially */
int unit_size = 1;                  /* default 1 */
unsigned char mem_buf[MEM_BUF_SIZE];
size_t mem_count = 0;

/* Format arrays recommended by the lab for unit-based printing */
static char* hex_formats[] = {"%#hhx\n", "%#hx\n", "No such unit\n", "%#x\n"};
static char* dec_formats[] = {"%#hhd\n", "%#hd\n", "No such unit\n", "%#d\n"};

/* Task 1c helper:
 * Print a single unit (1/2/4 bytes) from address p, according to display_decimal.
 * Uses memcpy to avoid alignment issues.
 */


/* Lab 1 style menu descriptor (but fun is void(void) for Lab 4) */
struct fun_desc {
    char *name;
    void (*fun)(void);
};


/* remove newline from string */
static void strip_newline(char *s) {
    if (s == NULL) return;
    s[strcspn(s, "\n")] = '\0';
}

void toggle_debug_mode(void) {
    debug_mode = !debug_mode;
    if (debug_mode)
        fprintf(stderr, "Debug flag now on\n");
    else
        fprintf(stderr, "Debug flag now off\n");
}

void set_file_name(void) {
    char line[256];

    printf("Please enter file name\n");
    if (fgets(line, sizeof(line), stdin) == NULL) {
        printf("EOF detected, exiting.\n");
        exit(0);
    }
    strip_newline(line);

    /* assume <=100 chars per instructions, still clamp safely */
    strncpy(file_name, line, FILE_NAME_MAX);
    file_name[FILE_NAME_MAX] = '\0';

    if (debug_mode) {
        fprintf(stderr, "Debug: file name set to '%s'\n", file_name);
    }
}

void set_unit_size(void) {
    char line[256];
    int val = 0;

    printf("Please enter unit size\n");
    if (fgets(line, sizeof(line), stdin) == NULL) {
        printf("EOF detected, exiting.\n");
        exit(0);
    }

    if (sscanf(line, "%d", &val) != 1) {
        printf("Error: invalid number\n");
        return;
    }

    if (val == 1 || val == 2 || val == 4) {
        unit_size = val;
        if (debug_mode) {
            fprintf(stderr, "Debug: set size to %d\n", unit_size);
        }
    } else {
        printf("Error: invalid unit size (must be 1, 2, or 4)\n");
    }
}

void quit_program(void) {
    if (debug_mode) {
        fprintf(stderr, "quitting\n");
    }
    exit(0);
}

void load_into_memory(void) {
    if (file_name[0] == '\0') {
        printf("Error: file name is not set\n");
        return;
    }

    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL) {
        printf("Error: failed to open file '%s'\n", file_name);
        return;
    }

    char line[256];
    unsigned int location = 0; /* hex input */
    int length_units = 0;      /* decimal input */

    printf("Please enter <location> <length>\n");
    if (fgets(line, sizeof(line), stdin) == NULL) {
        printf("EOF detected, exiting.\n");
        fclose(fp);
        exit(0);
    }

    /* location in hex, length in decimal */
    if (sscanf(line, "%x %d", &location, &length_units) != 2) {
        printf("Error: invalid input (expected: <hex_location> <decimal_length>)\n");
        fclose(fp);
        return;
    }

    if (length_units < 0) {
        printf("Error: length must be non-negative\n");
        fclose(fp);
        return;
    }

    size_t bytes_to_read = (size_t)length_units * (size_t)unit_size;
    if (bytes_to_read > MEM_BUF_SIZE) {
        printf("Error: requested size (%zu bytes) exceeds mem_buf size (%d bytes)\n",
               bytes_to_read, MEM_BUF_SIZE);
        fclose(fp);
        return;
    }

    if (debug_mode) {
        fprintf(stderr, "Debug: file_name='%s'\n", file_name);
        fprintf(stderr, "Debug: location=0x%x\n", location);
        fprintf(stderr, "Debug: length=%d\n", length_units);
    }

    /* seek to location */
    if (fseek(fp, (long)location, SEEK_SET) != 0) {
        printf("Error: failed to seek to 0x%x in file '%s'\n", location, file_name);
        fclose(fp);
        return;
    }

    /* read bytes into mem_buf */
    size_t bytes_read = fread(mem_buf, 1, bytes_to_read, fp);
    mem_count = bytes_read;

    /* If read ended early: error or EOF */
    if (bytes_read < bytes_to_read) {
        if (ferror(fp)) {
            printf("Error: failed while reading from file '%s'\n", file_name);
            fclose(fp);
            return;
        }

        /* EOF reached early (not fatal) */
        size_t requested_units = (unit_size == 0) ? 0 : (bytes_to_read / (size_t)unit_size);
        size_t actual_units    = (unit_size == 0) ? 0 : (bytes_read   / (size_t)unit_size);

        printf("Warning: reached end of file early (requested %zu units, loaded %zu units)\n",
               requested_units, actual_units);

        if (debug_mode) {
            fprintf(stderr, "Debug: EOF reached early, read %zu/%zu bytes\n",
                    bytes_read, bytes_to_read);
        }
    }

    fclose(fp);

    /* Report actual loaded units */
    size_t units_read = (unit_size == 0) ? 0 : (bytes_read / (size_t)unit_size);
    printf("Loaded %zu units into memory\n", units_read);
}

void toggle_display_mode() {
    if (!display_decimal) {
        display_decimal = true;
        printf("Decimal display flag now on, decimal representation\n");
    } else {
        display_decimal = false;
        printf("Decimal display flag now off, hexadecimal representation\n");
    }
}

static void print_one_unit(const unsigned char *p) {
    if (!(unit_size == 1 || unit_size == 2 || unit_size == 4)) {
        printf("No such unit\n");
        return;
    }

    if (display_decimal) {
        int32_t val = 0;
        memcpy(&val, p, (size_t)unit_size);
        printf(dec_formats[unit_size - 1], val);
    } else {
        uint32_t val = 0; // use unsigned for hex display
        memcpy(&val, p, (size_t)unit_size);
        printf(hex_formats[unit_size - 1], val);
    }
}

void memory_display(void) {

    char line[256];
    unsigned int addr_hex = 0; /* address in hex */
    int u = 0;                 /* number of units in decimal */

    printf("Enter address and length\n");
    if (fgets(line, sizeof(line), stdin) == NULL) {
        printf("EOF detected, exiting.\n");
        exit(0);
    }

    /* addr in hex, u in decimal */
    if (sscanf(line, "%x %d", &addr_hex, &u) != 2) {
        printf("Error: invalid input (expected: <hex_address> <decimal_length>)\n");
        return;
    }

    if (u <= 0) {
        printf("Error: length must be positive\n");
        return;
    }

    if (!(unit_size == 1 || unit_size == 2 || unit_size == 4)) {
        printf("Error: invalid unit size (must be 1, 2, or 4)\n");
        return;
    }

    const unsigned char *start = NULL;

    if (addr_hex == 0) {
        /* special case: start from beginning of mem_buf */
        start = mem_buf;

        /* mem_count is stored in BYTES in this file */
        size_t available_units = mem_count / (size_t)unit_size;
        if ((size_t)u > available_units) {
            printf("Warning: requested %d units but only %zu units available in mem_buf, showing %zu\n",
                   u, available_units, available_units);
            u = (int)available_units;
        }
    } else {
        /* treat input as an actual memory address */
        start = (const unsigned char *)(uintptr_t)addr_hex; 
    }

    if (display_decimal) {
        printf("Decimal\n");
        printf("=======\n");
    } else {
        printf("Hexadecimal\n");
        printf("===========\n");
    }

    if (debug_mode) {
        fprintf(stderr, "Debug: unit_size=%d\n", unit_size);
        fprintf(stderr, "Debug: address=0x%x\n", addr_hex);
        fprintf(stderr, "Debug: length=%d\n", u);
    }

    for (int i = 0; i < u; i++) {
        const unsigned char *p = start + ((size_t)i * (size_t)unit_size);
        print_one_unit(p);
    }
}

void save_into_file(void) {

    if (file_name[0] == '\0') {
        printf("Error: file name is not set\n");
        return;
    }

    if (!(unit_size == 1 || unit_size == 2 || unit_size == 4)) {
        printf("Error: invalid unit size (must be 1, 2, or 4)\n");
        return;
    }

    char line[256];
    unsigned int src_addr_hex = 0;
    unsigned int target_loc = 0;
    int length_units = 0;

    printf("Please enter <source-address> <target-location> <length>\n");

    if (fgets(line, sizeof(line), stdin) == NULL) { // EOF
        printf("EOF detected, exiting.\n");
        exit(0);
    }

    if (sscanf(line, "%x %x %d", &src_addr_hex, &target_loc, &length_units) != 3) { // parese input
        printf("Error: invalid input (expected: <hex_source> <hex_target> <decimal_length>)\n");
        return;
    }

    if (length_units <= 0) {
        printf("Error: length must be positive\n");
        return;
    }

    size_t bytes_to_write = (size_t)length_units * (size_t)unit_size; // total bytes to write

    const unsigned char *src_ptr = NULL; // pointer to source data
    if (src_addr_hex == 0) { // special case source is mem_buf
        src_ptr = mem_buf;

        // If source is mem_buf, make sure we don't read past what we loaded 
        if (bytes_to_write > mem_count) { // not enough data in mem_buf
            printf("Error: requested %zu bytes, but only %zu bytes are loaded into memory\n",
                   bytes_to_write, mem_count);
            return;
        }
    } else {
        // treat input as an actual memory address 
        src_ptr = (const unsigned char *)(uintptr_t)src_addr_hex; // cast to pointer
    }

    FILE *fp = fopen(file_name, "r+b"); /* read/write, binary, not truncate */
    if (fp == NULL) {
        printf("Error: failed to open file '%s' for writing\n", file_name);
        return;
    }

    /* Determine file size (for bounds checking) */
    if (fseek(fp, 0, SEEK_END) != 0) { 
        printf("Error: failed to seek in file '%s'\n", file_name);
        fclose(fp);
        return;
    }

    long file_size_long = ftell(fp); // get file size
    if (file_size_long < 0) {
        printf("Error: failed to determine file size of '%s'\n", file_name);
        fclose(fp);
        return;
    }
    size_t file_size = (size_t)file_size_long; // safe cast

    // if target-location is greater than file size - do nothing 
    if ((size_t)target_loc > file_size) { // target location beyond EOF
        printf("Error: target location 0x%x is beyond end of file (size: %zu bytes)\n",
               target_loc, file_size);
        fclose(fp);
        return;
    }

    // prevent extending the file - ensures only <length> units are overwritten 
    if ((size_t)target_loc + bytes_to_write > file_size) {
        printf("Error: write range [0x%x, 0x%zx) exceeds file size (%zu bytes)\n",
               target_loc, (size_t)target_loc + bytes_to_write, file_size);
        fclose(fp);
        return;
    }

    if (debug_mode) {
        fprintf(stderr, "Debug: file_name='%s'\n", file_name);
        fprintf(stderr, "Debug: source-address=0x%x\n", src_addr_hex);
        fprintf(stderr, "Debug: target-location=0x%x\n", target_loc);
        fprintf(stderr, "Debug: length=%d\n", length_units);
        fprintf(stderr, "Debug: writing %zu bytes\n", bytes_to_write);
    }

    // Seek to the target location and overwrite data
    if (fseek(fp, (long)target_loc, SEEK_SET) != 0) { 
        printf("Error: failed to seek to 0x%x in file '%s'\n", target_loc, file_name);
        fclose(fp);
        return;
    }

    size_t wrote = fwrite(src_ptr, 1, bytes_to_write, fp); // write data to file
    if (wrote != bytes_to_write) { // check if all bytes were written
        printf("Error: failed to write to file '%s' (wrote %zu/%zu bytes)\n",
               file_name, wrote, bytes_to_write);
        fclose(fp);
        return;
    }

    fclose(fp);
    printf("Wrote %d units into file\n", length_units);
}

void memory_modify(void) {

    if (!(unit_size == 1 || unit_size == 2 || unit_size == 4)) {
        printf("Error: invalid unit size (must be 1, 2, or 4)\n");
        return;
    }

    if (mem_count == 0) {
        printf("Error: memory buffer is empty (load into memory first)\n");
        return;
    }

    char line[256];
    unsigned int location = 0;   /* hex offset into mem_buf */
    unsigned int val = 0;        /* hex value */

    printf("Please enter <location> <val>\n");
    if (fgets(line, sizeof(line), stdin) == NULL) {
        printf("EOF detected, exiting.\n");
        exit(0);
    }

    if (sscanf(line, "%x %x", &location, &val) != 2) {
        printf("Error: invalid input (expected: <hex_location> <hex_val>)\n");
        return;
    }

    // bounds check: we can only modify what we actually loaded 
    if ((size_t)location + (size_t)unit_size > mem_count) {
        printf("Error: location 0x%x out of bounds (mem_count=%zu bytes, unit_size=%d)\n",
               location, mem_count, unit_size);
        return;
    }

    if (debug_mode) {
        fprintf(stderr, "Debug: location=0x%x\n", location);
        fprintf(stderr, "Debug: val=0x%x\n", val);
    }

    // Write little-endian bytes into mem_buf at the given offset 
    uint32_t tmp = (uint32_t)val;
    memcpy(mem_buf + location, &tmp, (size_t)unit_size);
}


struct fun_desc menu[] = {
    { "Toggle Debug Mode", toggle_debug_mode },
    { "Set File Name", set_file_name },
    { "Set Unit Size", set_unit_size },
    { "Load Into Memory", load_into_memory },
    { "Toggle Display Mode", toggle_display_mode },
    { "Memory Display", memory_display },
    { "Save Into File", save_into_file },
    { "Memory Modify", memory_modify },
    { "Quit", quit_program },
    { NULL, NULL }
};

int main(void) {
    int menu_len = 0;
    while (menu[menu_len].name != NULL) {
        menu_len++;
    }

    char line[128];

    while (1) {
        /* print debug state before menu */
        if (debug_mode) {
            fprintf(stderr, "Debug: unit_size=%d\n", unit_size);
            fprintf(stderr, "Debug: file_name='%s'\n", file_name);
            fprintf(stderr, "Debug: mem_count=%zu\n", mem_count);
        }

        printf("Select operation number from the following menu:\n");
        for (int i = 0; i < menu_len; i++) {
            printf("%d) %s\n", i, menu[i].name);
        }
        printf("Option (number): ");

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("EOF detected, exiting.\n");
            break;
        }

        int choice;
        if (sscanf(line, "%d", &choice) != 1) {
            printf("Not within bounds\n");
            continue;
        }

        if (choice < 0 || choice >= menu_len) {
            printf("Not within bounds\n");
            continue;
        }

        printf("Within bounds\n");
        menu[choice].fun();
    }

    return 0;
}
