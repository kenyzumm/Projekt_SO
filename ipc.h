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

struct shared {
    char buf[BUFFER_SIZE];
    int len;
};
struct msgbuf {
    long type;
    int data;
};

void sem_op(int sem_id, int sem_num, int op) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = op;
    sb.sem_flg = 0;

    semop(sem_id, &sb, 1);
}

void handle_error(const char* msg, int exit_code) {
    printf("Error: %s\n", msg);
    exit(exit_code);
}

struct shared* get_shared_memory() {
    key_t key = KEY;
    if (key == -1) return NULL;

    int shm_id = shmget(key, 0, 0);
    if (shm_id == -1) return NULL;

    struct shared *shm = (struct shared*)shmat(shm_id, NULL, 0);
    if (shm == (void*) - 1) {
        return NULL;
    }
    return shm;
}

int get_semaphore() {
    key_t key = KEY;
    if (key == -1) return -1;
    return semget(key, 0, 0);
}

int get_msg_queue() {
    key_t key = KEY;
    if (key == -1) return -1;
    return msgget(key, 0);
}

int set_signals(void (*signal_handler)(int)) {
    signal(SIGUSR1, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTSTP, signal_handler);

    return 0;
}