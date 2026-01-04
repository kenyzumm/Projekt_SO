# Makefile dla Producer-Consumer IPC
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = -lrt  # System V IPC (msgq, shm, sem)

# Pliki obiektowe
OBJS = main.o ipc.o proc1.o proc2.o proc3.o

# Główny target
ipc_proj: $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Kompilacja plików .c do .o (automatycznie używa ipc.h)
%.o: %.c ipc.h
	$(CC) $(CFLAGS) -c $< -o $@

# Czyszczenie
clean:
	rm -f $(OBJS) ipc_proj ipc.key  # + pliki IPC

.PHONY: clean debug test install-deps
