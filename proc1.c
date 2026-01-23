#include "ipc.h"

void loop(int msg_id);

void handle_signal(int sig) {
    if (sig == SIGUSR1) {
        // obsluz sygnal
    } else {
        printf("[P1] Niepoprawny sygnal\n");
    }
}

int main(int argc, char* argv[]) {
    set_signals(handle_signal);

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