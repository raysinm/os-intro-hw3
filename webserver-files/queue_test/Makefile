OBJS = test_queue.o queue.o
TARGET = test_queue

CC = gcc
CFLAGS = -g -Wall

.SUFFIXES: .c .o

all: $(TARGET)

$(TARGET): test_queue.o queue.o
	$(CC) $(CFLAGS) $^ -o $@

queue: queue.o

.c.o:	
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	-rm -f $(OBJS) test_queue queue