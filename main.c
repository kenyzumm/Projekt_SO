#include "ipc.h"
#include <unistd.h>
#include <signal.h>

pid_t pid[3] = {-1};
int pipes[3][2];


void init(int* sem_id, int* shm_id, int* msg_id, int pipes[][2]);
int init_semaphores(int number_sems, int* sem_id);
int init_shared_memory(int* shm_id);
int init_msg(int* msg_id);
int init_pipes(int pipes[][2]);
void clear_all(int* sem_id, int* shm_id, int* msg_id, int pipes[][2]);
void handle_signal(int sig) {
    char buf[4];
    memcpy(buf, &sig, 4);

    for (int i=PM_P1; i<=PM_P3; i++) {
        write(pipes[i][WRITE], buf, 4);
    }
    kill(pid[2], SIGUSR1);
    
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    int sem_id=-1, shm_id=-1, msg_id=-1;

    init(&sem_id, &shm_id, &msg_id, pipes);
    
    int id = -1;
    for (int i=0; i<3; i++) {
        pid[i] = fork();
        if (pid[i] == -1) handle_error("[PM] fork", 1);
        if (pid[i] == 0) {
            id = i;
            break;
        }
        
    }
    char buf[16];
    char buf2[16];
    switch(id) {
        case 0:
            close(pipes[PM_P1][WRITE]);
            close(pipes[PM_P2][READ]);
            close(pipes[PM_P2][WRITE]);
            close(pipes[PM_P3][READ]);
            close(pipes[PM_P3][WRITE]);
            snprintf(buf, sizeof(buf), "%d", pipes[PM_P1][READ]);
            printf("PODMIENIAM P1\n");
            execlp("./proc1", "./proc1", buf, NULL);
        case 1:
            close(pipes[PM_P2][WRITE]);
            close(pipes[PM_P1][READ]);
            close(pipes[PM_P1][WRITE]);
            close(pipes[PM_P3][READ]);
            close(pipes[PM_P3][WRITE]);
            snprintf(buf, sizeof(buf), "%d", pipes[PM_P2][READ]);
            snprintf(buf2, sizeof(buf2), "%d", pid[0]);
            printf("PODMIENIAM P2\n");
            execlp("./proc2", "./proc2", buf, buf2, NULL);
        case 2:
            close(pipes[PM_P3][WRITE]);
            close(pipes[PM_P1][READ]);
            close(pipes[PM_P1][WRITE]);
            close(pipes[PM_P2][READ]);
            close(pipes[PM_P2][WRITE]);
            snprintf(buf, sizeof(buf), "%d", pipes[PM_P3][READ]);
            snprintf(buf2, sizeof(buf2), "%d", pid[1]);
            printf("PODMIENIAM P3\n");
            execlp("./proc3", "./proc3", buf, buf2, NULL);
    }

    set_signals(handle_signal);
    for(int i=PM_P1; i<=PM_P3; i++) {
        close(pipes[i][READ]);
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
        if (fcntl(pipes[i][READ], F_SETFL, O_NONBLOCK) == -1) {
            perror("fcntl READ");
        }

        // Dla WRITE end - nieblokujące  
        if (fcntl(pipes[i][WRITE], F_SETFL, O_NONBLOCK) == -1) {
            perror("fcntl WRITE");
        }
    }
    return 0;
}

void clear_all(int* sem_id, int* shm_id, int* msg_id, int pipes[][2]) {
    if (*sem_id != -1) semctl(*sem_id, 0, IPC_RMID);
    if (*shm_id != -1) shmctl(*shm_id, IPC_RMID, NULL);
    if (*msg_id != -1) msgctl(*msg_id, IPC_RMID, NULL);
}