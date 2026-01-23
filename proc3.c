#include "ipc.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h> // tylko debug

int pipe_fm; //id pipe'a from main to proc3
pid_t p2_pid; //pid procesu proc2

void loop(int sem_id, struct shared* shm);

void handle_signal(int sig);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("[P3] Nieprawidlowa liczba argumentow! Oczekiwano 2 (pipe, pid), otrzymano: %d\n", argc - 1);
        return 10;
    }
    pipe_fm = atoi(argv[1]); 
    p2_pid = atoi(argv[2]);

    set_signals(handle_signal);

    int sem_id = get_semaphore();
    if (sem_id == -1) return 1;
    struct shared* shm = get_shared_memory();
    if (!shm) return 2;

    loop(sem_id, shm);

    shmdt(shm);

    return 0;
}

void loop(int sem_id, struct shared* shm) {
    char buffer[BUFFER_SIZE];

    while(fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;

        P_EMPTY;
        P_MUTEX;

        if (strlen(buffer) < sizeof(shm->buf)) {
            strcpy(shm->buf, buffer);
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

void handle_signal(int sig) {
    if (sig == SIGUSR1) {
        char buf[sizeof(int)];
        read(pipe_fm, buf, sizeof(buf));
        kill(p2_pid, SIGUSR1);
    } else {
        printf("[P3] Niepoprawny sygnal\n");
    }   
}