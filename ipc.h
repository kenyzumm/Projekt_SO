#ifndef IPC_H
#define IPC_H
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

#define BUFFER_SIZE 1024

#define PM_P1 0
#define PM_P2 1
#define PM_P3 2
#define READ 0
#define WRITE 1

#define MUTEX 0
#define EMPTY 1
#define FULL 2

#define P_MUTEX     sem_op(sem_id, MUTEX, -1)
#define V_MUTEX     sem_op(sem_id, MUTEX, 1)
#define P_EMPTY     sem_op(sem_id, EMPTY, -1)
#define V_EMPTY     sem_op(sem_id, EMPTY, 1)
#define P_FULL      sem_op(sem_id, FULL, -1)
#define V_FULL      sem_op(sem_id, FULL, 1)

#define KEY ftok("/tmp", 'A')

#ifdef __linux__
union semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
    struct seminfo  *__buf;
};
#endif

struct shared {
    char buf[BUFFER_SIZE];
    int len;
};

struct msgbuf {
    long type;
    int data;
};

int sem_op(int sem_id, int sem_num, int op);

void handle_error(const char* msg, int exit_code);

struct shared* get_shared_memory(void);
int get_semaphore(void);
int get_msg_queue(void);

const char *sig_name(int s);

void clean_stdin_buffer(void);

void remove_newline(char *s);

#endif