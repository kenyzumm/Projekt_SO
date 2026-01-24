#include "ipc.h"
#include <string.h>

int p;
pid_t p1_id;
int got_signal = 0;
int s;

void loop(int sem_id, struct shared* shm, int msg_id);
void signal_handler(int sig);
void handle_signal();


int main(int argc, char* argv[]) {
    set_signals(signal_handler);

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
        if (got_signal) handle_signal();

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

void signal_handler(int sig) {
    got_signal = 1;
    s = sig;
}

void handle_signal() {
    switch(s) {
        case SIGUSR1:
            kill(p1_id, SIGUSR1);
            char buf[4];
            int s;

            read(p, buf, 4);
            memcpy(&s, buf, 4);
            
            printf("[P2] Otrzymalem SIGUSR1 oraz ID: %d\n", s);

        break;
        default:
            kill(getppid(), s);
        break;
    }
    got_signal = 0;
}