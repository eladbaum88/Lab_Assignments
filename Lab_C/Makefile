CC     = gcc
CFLAGS = -m32 -Wall -g

.PHONY: all clean

all: myshell mypipeline Looper

myshell: myshell.c LineParser.c LineParser.h
	$(CC) $(CFLAGS) myshell.c LineParser.c -o myshell

mypipeline: mypipeline.c
	$(CC) $(CFLAGS) mypipeline.c -o mypipeline

looper: Looper.c
	$(CC) $(CFLAGS) Looper.c -o Looper

clean:
	rm -f myshell mypipeline Looper
