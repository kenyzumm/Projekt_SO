#include "ipc.h"
#include <unistd.h>


void init(int* sem_id, int* shm_id, int* msg_id, int pipes[][2]);
int init_semaphores(int number_sems, int* sem_id);
int init_shared_memory(int* shm_id);
int init_msg(int* msg_id);
int init_pipes(int pipes[][2]);
void clear_all(int* sem_id, int* shm_id, int* msg_id, int pipes[][2]);
void handle_signal(int sig) {
    switch (sig) {
        case SIGINT: // wznowienie ctrl c
        break;
        case SIGTSTP: // zatrzymanie ctrl z
        break;
        case SIGQUIT: // wyjscie ctrl /* \ */
        break;
    }
}

int main() {
    pid_t pid[3] = {-1};
    int sem_id=-1, shm_id=-1, msg_id=-1;
    int pipes[3][2];

    init(&sem_id, &shm_id, &msg_id, pipes);
    set_signals(handle_signal);

    int id = -1;
    for (int i=0; i<3; i++) {
        pid[i] = fork();
        if (pid[i] == -1) handle_error("[PM] fork", 1);
        if (pid[i] == 0) {
            id = i;
            break;
        }
        
    }
    switch(id) {
        case 0:
            execlp("./proc1", pipes[PM_P1]);
        case 1:
            execlp("./proc2", pipes[PM_P2]);
        case 2:
            execlp("./proc3", pipes[PM_P3]);
    }

    for (int i=0; i<3; i++) {
        wait(NULL);
    }

    clear_all(&sem_id, &shm_id, &msg_id, pipes);
}

void init(int* sem_id, int* shm_id, int* msg_id, int pipes[][2]) {
    int ec;
    ec = init_semaphores(3, sem_id);
    if (ec) {
        clear_all(sem_id, shm_id, msg_id, pipes);
        handle_error("[PM] semaphores", ec);
    }
    ec = init_shared_memory(shm_id);
    if (ec) {
        clear_all(sem_id, shm_id, msg_id, pipes);
        handle_error("[PM] shared memory", ec);
    }
    ec = init_msg(msg_id);
    if (ec) {
        clear_all(sem_id, shm_id, msg_id, pipes);
        handle_error("[PM] message queue", ec);
    }
    ec = init_pipes(pipes);
    if (ec) {
        clear_all(sem_id, shm_id, msg_id, pipes);
        handle_error("[PM] pipes", ec);
    }
}

int init_semaphores(int number_sems, int* sem_id) {
    key_t key = KEY;
    if (key == -1) return 5;

    *sem_id = semget(key, number_sems, IPC_CREAT | 0666);
    if (*sem_id == -1) return 6;

    union semun arg;
    arg.val = 1;
    if (semctl(*sem_id, MUTEX, SETVAL, arg) == -1) return 7;
    if (semctl(*sem_id, EMPTY, SETVAL, arg) == -1) return 7;
    arg.val = 0;
    if (semctl(*sem_id, FULL, SETVAL, arg) == -1) return 7;

    return 0;
}

int init_shared_memory(int* shm_id) {
    key_t key = KEY;
    if (key == -1) return 5;

    *shm_id = shmget(key, sizeof(struct shared), IPC_CREAT | 0666);
    if (*shm_id == -1) return 8;

    return 0;
}

int init_msg(int* msg_id) {
    key_t key = KEY;
    if (key == -1) return 5;
    *msg_id = msgget(key, IPC_CREAT | 0666);
    if (*msg_id == -1) return 9;

    return 0;
}

int init_pipes(int pipes[][2]) {
    for (int i=0; i<3; i++) {
        if (pipe(pipes[i]) == -1) {
            return 10;
        }
        close(pipes[i][READ]);
    }
}

void clear_all(int* sem_id, int* shm_id, int* msg_id, int pipes[][2]) {
    if (*sem_id != -1) semctl(*sem_id, 0, IPC_RMID);
    if (*shm_id != -1) shmctl(*shm_id, IPC_RMID, NULL);
    if (*msg_id != -1) msgctl(*msg_id, IPC_RMID, NULL);
}