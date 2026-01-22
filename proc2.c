#include "ipc.h"
#include <string.h>

void loop(int sem_id, struct shared* shm, int msg_id);

int main() {
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