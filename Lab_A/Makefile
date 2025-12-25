
CC      = gcc
CFLAGS  = -g -m32 -Wall
TARGET  = encoder
SRCS    = encoder.c
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<


.PHONY: run
run: $(TARGET)
	./$(TARGET)


.PHONY: all clean
clean:
	rm -f $(OBJS) $(TARGET)