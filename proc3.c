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
    set_signals(signal_handler);
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

    close(p2_pid);

    printf("[P3] Koniec procesu\n");

    return 0;
}

void loop(int sem_id, struct shared* shm) {
    char buffer[BUFFER_SIZE];

    while(fgets(buffer, BUFFER_SIZE, stdin) != NULL) {

        if (got_signal) handle_signal();

        // Czekaj dopóki jesteśmy zatrzymani (nie porzucaj odczytanej linii)
        while (is_paused) {
            if (got_signal) handle_signal();
        }

        buffer[strcspn(buffer, "\n")] = 0;

        P_EMPTY;
        P_MUTEX;

        if (strlen(buffer) < sizeof(shm->buf)) {
            strcpy(shm->buf, buffer);
            printf("%s\n", shm->buf);
        } else {
            printf("[P3] Uwaga: Wiersz za dlugi - zostal przyciety.\n");
        }

        V_MUTEX;
        V_FULL;
        sleep(1); // debug
    }

    if (ferror(stdin)) {
        printf("[P3] Krytyczny blad odczytu stdin\n");
    } else {
        P_EMPTY;
        P_MUTEX;

        shm->buf[0] = 0;

        V_MUTEX;
        V_FULL;
        printf("[P3] KONIEC DANYCH\n");
    }
}

void signal_handler(int sig) {
    s = sig;
    got_signal = 1;
}

void handle_signal() {
    if (s == SIGUSR1) {
        char buf[sizeof(int)];
        int signal_id;
        read(pipe_fm, buf, sizeof(buf));
        memcpy(&signal_id, buf, 4);
        printf("[P3] Otrzymalem SIGUSR1 oraz ID: %d\n", signal_id);
        kill(p2_pid, SIGUSR1);
        switch(signal_id) {
            case SIGTSTP:
                printf("[P3] Zatrzymano proces (SIGTSTP). Oczekiwanie na SIGCONT...\n");
                is_paused = 1;
                break;
            case SIGINT:
                printf("[P3] Wznowiono proces (SIGCONT)\n");
                is_paused = 0;
                break;
            case SIGTERM:
            break;
        }
    }
    got_signal = 0;
}