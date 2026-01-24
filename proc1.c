#include "ipc.h"
#include <errno.h>

int p;
int got_signal = 0;
int received_signal = 0;
int is_paused = 0;

void loop(int msg_id);
void signal_handler(int sig);
void handle_signal();

int main(int argc, char* argv[]) {
    set_signals_ignore(signal_handler);
    
    if (argc != 2 ) {
        printf("Uzycie: %s <PIPE>\n", argv[0]);
        return 1;
    }

    p = atoi(argv[1]);

    int msg_id = get_msg_queue();
    if (msg_id == -1) return 1;

    loop(msg_id);
    close(p);
    printf("[P1] Koniec procesu\n");
}

void loop(int msg_id) {
    struct msgbuf msg;
    while(1) {
        if (got_signal) handle_signal();
        
        if (is_paused) {
            pause();
            continue;
        }

        if (msgrcv(msg_id, &msg, sizeof(msg.data), 1, 0) == -1) {
            if (errno == EINTR) continue;
            break; 
        }
        
        if (msg.data == -1) break;
        printf("[P1] Odebrano: %d\n", msg.data);
    }
}

void signal_handler(int sig) {
    received_signal = sig;
    got_signal = 1;
}

void handle_signal() {
    got_signal = 0;
    if (received_signal == SIGUSR1) {
        int signal_id;
        if (read(p, &signal_id, sizeof(int)) == sizeof(int)) {
            printf("[P1] Przetwarzam ID sygnalu: %d\n", signal_id);
            if (signal_id == SIGTSTP) {
                printf("[P1] Pauza\n");
                is_paused = 1;
            } else if (signal_id == SIGINT || signal_id == SIGCONT) {
                printf("[P1] Wznowienie\n");
                is_paused = 0;
            }
        }
    }
}