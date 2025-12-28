CC      = gcc
CFLAGS  = -m32 -Wall -Wextra -O0 -fno-pie -no-pie -fno-stack-protector
TARGETS = hexeditplus count_digit

all: $(TARGETS)

# Rule to compile hexeditplus
hexeditplus: hexeditplus.c
	$(CC) $(CFLAGS) hexeditplus.c -o hexeditplus

# Rule to compile count_digit (used for patching ntsc)
count_digit: count_digit.c
	$(CC) $(CFLAGS) count_digit.c -o count_digit

clean:
	rm -f $(TARGETS) *.o
