CC = gcc
CFLAGS = -m32 -Wall -g

.PHONY: all clean

all: myshell Looper mypipe

myshell: myshell.c LineParser.c
	$(CC) $(CFLAGS) myshell.c LineParser.c -o myshell

Looper: Looper.c
	$(CC) $(CFLAGS) Looper.c -o Looper

mypipe: mypipe.c
	$(CC) $(CFLAGS) mypipe.c -o mypipe

clean:
	rm -f myshell Looper mypipe *.o
