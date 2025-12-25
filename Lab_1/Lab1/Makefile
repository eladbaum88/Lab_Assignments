# Makefile for count-words program

TARGET := count_words addresses base manu

# compiler and flags (enable warnings + debug info for gdb)
CC     := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -O0 -g
CFLAGS += -m32


# default target
all: $(TARGET)

# count_words 
count_words: count-words.o
	$(CC) $(CFLAGS) $^ -o $@

count-words.o: count-words.c
	$(CC) $(CFLAGS) -c $< -o $@

# addresses
addresses: addresses.o
	$(CC) $(CFLAGS) $^ -o $@

addresses.o: addresses.c
	$(CC) $(CFLAGS) -c $< -o $@

# base
base: base.o
	$(CC) $(CFLAGS) $^ -o $@
base.o: base.c
	$(CC) $(CFLAGS) -c $< -o $@	

# manu
manu: manu.o
	$(CC) $(CFLAGS) $^ -o $@
manu.o: manu.c
	$(CC) $(CFLAGS) -c $< -o $@

# run targets
run_count:
	./count_words $(ARGS)

run_addresses:
	./addresses $(ARGS)

run_base:
	./base $(ARGS)	
	
run_menu:	
	./menu $(ARGS)	

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean run_count run_addresses run_base run_manu