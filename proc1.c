#include "ipc.h"

int p;
int got_signal = 0;
int received_signal = 0;

void loop(int msg_id);
void signal_handler(int sig);
void handle_signal();

int main(int argc, char* argv[]) {
    set_signals(signal_handler);
    
    if (argc != 2 ) {
        printf("Uzycie: %s <PIPE>\n", argv[0]);
        return 1;
    }

    p = atoi(argv[1]);

    int msg_id = get_msg_queue();
    if (msg_id == -1) return 1;

    loop(msg_id);
    printf("Wylaczam proces 1\n");
}

void loop(int msg_id) {
    struct msgbuf msg;
    while(1) {
        if (got_signal) handle_signal();

        msgrcv(msg_id, &msg, sizeof(msg.data), 1, 0);
        if (msg.data == -1) break;
        printf("[P1] Odebrano: %d\n", msg.data);
    }
}

void signal_handler(int sig) {
    received_signal = sig;
    got_signal = 1;
}

void handle_signal() {
    if (received_signal == SIGUSR1) {
        char buf[sizeof(int)];
        int signal_id;
        read(p, buf, sizeof(buf));
        memcpy(&signal_id, buf, 4);
        printf("[P1] Otrzymalem SIGUSR1 oraz ID: %d\n", signal_id);
    }
    got_signal = 0;
}