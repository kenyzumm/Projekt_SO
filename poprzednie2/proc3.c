#include "ipc.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int pipe_fm; //id pipe'a from main to proc3
pid_t p2_pid; //pid procesu proc2
int got_signal=0;
int s;
int is_paused = 0;

void loop(int sem_id, struct shared* shm);
void signal_handler(int sig);
void handle_signal();


int main(int argc, char* argv[]) {
    set_signals_ignore(signal_handler);
    if (argc != 3) {
        printf("[P3] Nieprawidlowa liczba argumentow! Oczekiwano 2 (pipe, pid), otrzymano: %d\n", argc - 1);
        return 10;
    }

    pipe_fm = atoi(argv[1]);
    p2_pid = atoi(argv[2]);

    int sem_id = get_semaphore();
    if (sem_id == -1) return 1;
    struct shared* shm = get_shared_memory();
    if (!shm) return 2;

    loop(sem_id, shm);

    close(pipe_fm);

    printf("[P3] Koniec procesu\n");

    return 0;
}

void loop(int sem_id, struct shared* shm) {
    char buffer[BUFFER_SIZE];

    while(1) {
        if (got_signal) handle_signal();

        if (is_paused) {
            continue;
        }

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            if (errno == EINTR) continue;
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0;

        if (P_EMPTY == -1) {
            if (errno == EINTR) continue;
            break;
        }
        if (P_MUTEX == -1) {
            if (errno == EINTR) continue;
            break;
        }

        if (strlen(buffer) < sizeof(shm->buf)) {
            strcpy(shm->buf, buffer);
            printf("%s\n", shm->buf);
        } else {
            printf("[P3] Uwaga: Wiersz za dlugi - zostal przyciety.\n");
        }

        V_MUTEX;
        V_FULL;

        //sleep(1);
    }

    // Koniec danych
    P_EMPTY;
    P_MUTEX;
    shm->buf[0] = 0;
    V_MUTEX;
    V_FULL;
    printf("[P3] KONIEC DANYCH\n");
}

void signal_handler(int sig) {
    s = sig;
    got_signal = 1;
}

void handle_signal() {
    got_signal = 0;
    if (s == SIGUSR1) {
        int signal_id;
        if (read(pipe_fm, &signal_id, sizeof(int)) == sizeof(int)) {
            printf("[P3] Przetwarzam ID sygnalu: %d\n", signal_id);
            if (signal_id == SIGTSTP) {
                printf("[P3] Pauza\n");
                is_paused = 1;
            } else if (signal_id == SIGINT || signal_id == SIGCONT) {
                printf("[P3] Wznowienie\n");
                is_paused = 0;
            }
            // Propagacja
            kill(p2_pid, SIGUSR1);
        }
    } else if (s == SIGTERM) {
        printf("[P3] Konczenie...\n");
        exit(0);
    }
}