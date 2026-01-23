#include "ipc.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h> // tylko debug

void loop(int sem_id, struct shared* shm);

void handle_signal(int sig) {
    if (sig == SIGUSR1) {
        // obluz sygnal
    } else {
        printf("[P3] Niepoprawny sygnal\n");
    }
}

int main(int argc, char* argv[]) {
    set_signals(handle_signal);
    int sem_id = get_semaphore();
    if (sem_id == -1) return 1;
    struct shared* shm = get_shared_memory();
    if (!shm) return 2;

    loop(sem_id, shm);
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
            printf("ERROR TO BE HANDLED\n");
        }

        V_MUTEX;
        V_FULL;
        sleep(1); // debug
    }

    if (ferror(stdin)) {
        printf("ERROR TO BE HANDLED\n");
    } else {
        P_EMPTY;
        P_MUTEX;

        shm->buf[0] = 0;

        V_MUTEX;
        V_FULL;
        printf("KONIEC DANYCH\n");
    }
}