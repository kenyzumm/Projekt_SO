#ifndef IPC_H
#define IPC_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define KEY_PATH "ipc.key"
#define KEY ftok(KEY_PATH, 'A')
#define MAX_LENGTH 1024

#define SEM_KEY 5678L
#define SHM_KEY 1234L

extern int debug_mode;

void init_ipc(void);
void cleanup_ipc(void);
void proc1();
void proc2();
void proc3();

void handle_error(const char* msg, int exit_code);
void debug(const char* msg);
void sem_op(int semid, int sem_num, int op);

struct shared_data {
    char text[256]; // Bufor na tekst przesyłany między procesami
};

#endif
