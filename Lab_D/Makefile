NASM = nasm
GCC  = gcc

NASMFLAGS = -f elf32
GCCFLAGS  = -m32

all: multi

multi: multi.o
	$(GCC) $(GCCFLAGS) -no-pie multi.o -o multi

multi.o: multi.s
	$(NASM) $(NASMFLAGS) multi.s -o multi.o

clean:
	rm -f multi.o multi
