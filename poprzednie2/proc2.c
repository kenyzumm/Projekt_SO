#include "ipc.h"
#include <string.h>
#include <errno.h>

int p;
pid_t p1_id;
int got_signal = 0;
int s;
int is_paused = 0;

void loop(int sem_id, struct shared* shm, int msg_id);
void signal_handler(int sig);
void handle_signal();


int main(int argc, char* argv[]) {
    set_signals_p2(signal_handler);

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

    close(p);
    printf("[P2] Koniec procesu\n");
}

void loop(int sem_id, struct shared* shm, int msg_id) {
    struct msgbuf msg;

    // Zestaw sygnałów do blokowania w celu uniknięcia wyścigu
    sigset_t new_mask, old_mask;
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGUSR1);
    sigaddset(&new_mask, SIGINT); 

    while (1) {
        if (got_signal) handle_signal();

        // Blokujemy sygnały przed sprawdzeniem stanu pauzy
        sigprocmask(SIG_BLOCK, &new_mask, &old_mask);
        if (is_paused) {
            // sigsuspend atomowo odblokowuje sygnały i czeka
            // Po powrocie przywraca maskę (sygnały zablokowane)
            sigsuspend(&old_mask);

            // Odblokowujemy sygnały, aby obsłużyć flagę w następnej iteracji
            sigprocmask(SIG_SETMASK, &old_mask, NULL);
            continue;
        }
        // Odblokowujemy sygnały dla normalnej pracy
        sigprocmask(SIG_SETMASK, &old_mask, NULL);

        if (P_FULL == -1) {
            if (errno == EINTR) continue;
            break;
        }
        if (P_MUTEX == -1) {
            if (errno == EINTR) continue;
            break;
        }

        char buffer[BUFFER_SIZE];
        strcpy(buffer, shm->buf);

        V_MUTEX;
        V_EMPTY;

        msg.type = 1;
        msg.data = strlen(buffer);
        if (msgsnd(msg_id, &msg, sizeof(msg.data), 0) == -1) {
            if (errno == EINTR) continue;
            break;
        }
    }

    msg.type = 1;
    msg.data = -1;
    msgsnd(msg_id, &msg, sizeof(msg.data), 0);
}


void signal_handler(int sig) {
    s = sig;
    got_signal = 1;
}

void handle_signal() {
    got_signal = 0;
    if (s == SIGUSR1) {
        int signal_id;
        if (read(p, &signal_id, sizeof(int)) == sizeof(int)) {
            printf("[P2] Przetwarzam ID sygnalu: %d\n", signal_id);
            if (signal_id == SIGTSTP) {
                printf("[P2] Pauza\n");
                is_paused = 1;
            } else if (signal_id == SIGINT || signal_id == SIGCONT) {
                printf("[P2] Wznowienie\n");
                is_paused = 0;
            }
            // Propagacja
            kill(p1_id, SIGUSR1);
        }
    } else {
        // Zewnętrzny - do maina
        printf("[P2] Forwarding signal %d to parent\n", s);
        kill(getppid(), s);
    }
}