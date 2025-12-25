CC = gcc
CFLAGS = -m32 -Wall -g -Wextra 

.PHONY: all clean

all: AntiVirus 

AntiVirus: AntiVirus.c
	$(CC) $(CFLAGS) AntiVirus.c -o AntiVirus

clean:
	rm -f AntiVirus *.o
