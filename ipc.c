#include "ipc.h"

#include <stdio.h>

void init_ipc(void) {}

void cleanup_ipc(void) {}

void proc1() {}
void proc2() {}
void proc3() {}

void handle_error(const char* msg, int exit_code) {
    perror(msg);
    exit(exit_code);
}
void debug(const char* msg) {
    if (debug_mode) {
        printf("[DEBUG]: %s\n", msg);
    }
}

// Funkcja wykonująca operacje na semaforach
// semid - ID zestawu, sem_num - nr semafora, op - operacja (np. -1 to opuszczenie, 1 to podniesienie)
void sem_op(int semid, int sem_num, int op) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = op;
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) == -1) {
        // Jeśli sem_op zostanie przerwane przez sygnał (np. SIGUSR1), spróbuj ponownie
        if (errno == EINTR) {
            sem_op(semid, sem_num, op);
        } else {
            handle_error("[IPC] Błąd operacji na semaforze (semop)", 1);
        }
    }
}