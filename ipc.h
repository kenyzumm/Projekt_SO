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

union semun {
    int              val;
    struct semid_ds *buf;
    unsigned short  *array;
    struct seminfo  *__buf;
};


struct shared {
    char buf[BUFFER_SIZE];
    int len;
};
struct msgbuf {
    long type;
    int data;
};

int sem_op(int sem_id, int sem_num, int op) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = op;
    sb.sem_flg = 0;

    return semop(sem_id, &sb, 1);
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

// Konfiguracja sygnałów dla procesów P1 i P3 - ignoruje sygnały zewnętrzne, reaguje na SIGUSR1
int set_signals_ignore(void (*signal_handler)(int)) {
    signal(SIGUSR1, signal_handler);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGCONT, SIG_IGN);
    return 0;
}

// Konfiguracja sygnałów dla P2 - reaguje na wszystko (przekazuje do main)
int set_signals_p2(void (*signal_handler)(int)) {
    signal(SIGUSR1, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTSTP, signal_handler);
    signal(SIGCONT, signal_handler);
    return 0;
}

// Konfiguracja sygnałów dla main - używa sigaction aby móc filtrować nadawcę (si_pid)
int set_main_signals(void (*handler)(int, siginfo_t *, void *)) {
    struct sigaction sa;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGTSTP, &sa, NULL);
    sigaction(SIGCONT, &sa, NULL);
    return 0;
}