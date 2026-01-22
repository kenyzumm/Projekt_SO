#include "ipc.h"

void loop(int msg_id);

int main() {
    int msg_id = get_msg_queue();
    if (msg_id == -1) return 1;

    loop(msg_id);
    printf("Wylaczam proces 1\n");
}

void loop(int msg_id) {
    struct msgbuf msg;
    while(1) {
        msgrcv(msg_id, &msg, sizeof(msg.data), 1, 0);
        if (msg.data == -1) break;
        printf("[P1] Odebrano: %d\n", msg.data);
    }
}