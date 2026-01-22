CC=gcc
CFLAGS=-Wall -g

TARGETS=main proc1 proc2 proc3

all: $(TARGETS)

main: main.c ipc.h
	$(CC) $(CFLAGS) -o main main.c

proc1: proc1.c ipc.h
	$(CC) $(CFLAGS) -o proc1 proc1.c

proc2: proc2.c ipc.h
	$(CC) $(CFLAGS) -o proc2 proc2.c

proc3: proc3.c ipc.h
	$(CC) $(CFLAGS) -o proc3 proc3.c

clean:
	rm -f *.o

.PHONY: all clean
