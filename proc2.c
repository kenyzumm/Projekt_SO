#include "ipc.h"
#include <string.h>

int p;
pid_t p1_id;

void loop(int sem_id, struct shared* shm, int msg_id);

void handle_signal(int sig) {
    switch(sig) {
        case SIGUSR1:
            char buf[4];
            read(p, buf, 4);

            int signal;
            memcpy(&signal, buf, 4);
            kill(p1_id, SIGUSR1);

            switch (signal) {
                default:
                    printf("[P2] Odczytalem sygnal: %d", signal);
                break;
            }
        break;
        default:
            kill(getppid(), sig);
        break;
    }
}

int main(int argc, char* argv[]) {

    if (argc != 3) return 2134;
    p = atoi(argv[1]);
    p1_id = atoi(argv[2]);

    int sem_id = get_semaphore();
    if (sem_id == -1) return 1;
    struct shared* shm = get_shared_memory();
    if (!shm) return 2;
    int msg_id = get_msg_queue();
    if (msg_id == -1) return 3;

    loop(sem_id, shm, msg_id);
}

void loop(int sem_id, struct shared* shm, int msg_id) {
    struct msgbuf msg;
    while (1) {
        char buffer[BUFFER_SIZE];
        P_FULL;
        P_MUTEX;
            strcpy(buffer, shm->buf);
            if(buffer[0] == 0) break;
        V_MUTEX;
        V_EMPTY;

        
        msg.type = 1;
        msg.data = strlen(buffer);
        msgsnd(msg_id, &msg, sizeof(msg.data), 0);
    }
    msg.type = 1;
    msg.data = -1;
    msgsnd(msg_id, &msg, sizeof(msg.data), 0);

}