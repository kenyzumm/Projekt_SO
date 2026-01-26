#include "ipc.h"

pid_t pid[3];
int pipes[3][2];
int msg;
int sem;
struct shared* shm;

int sem_op(int sem_id, int sem_num, int op) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = op;
    sb.sem_flg = 0;
    return semop(sem_id, &sb, 1);
}

void handle_error(const char* msg, int exit_code) {
    perror(msg);
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

int get_semaphore(void) {
    key_t key = KEY;
    if (key == -1) return -1;
    return semget(key, 0, 0);
}

int get_msg_queue(void) {
    key_t key = KEY;
    if (key == -1) return -1;
    return msgget(key, 0);
}

void clean_stdin_buffer(void) {
    tcflush(STDIN_FILENO, TCIFLUSH);
}

const char *sig_name(int s) {
    if (s == SIGTSTP) return "SIGTSTP";
    if (s == SIGCONT) return "SIGCONT";
    if (s == SIGTERM) return "SIGTERM";
    if (s == SIGINT)  return "SIGINT";
    if (s == SIGUSR1) return "SIGUSR1";
    return "INNY_SYGNAL";
}

void remove_newline(char *s) {
    if (!s) return;
    int l = strlen(s);
    if (l > 0 && s[l-1] == '\n') s[l-1] = '\0';
    if (l > 1 && s[l-2] == '\r') s[l-2] = '\0';
}

