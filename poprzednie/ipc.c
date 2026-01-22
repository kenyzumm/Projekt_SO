#include "ipc.h"

#include <stdio.h>

void init_ipc(void) {
    // Inicjalizacja IPC, jeśli jest potrzebna
}

void cleanup_ipc(void) {}

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

int get_semaphores(){
    int sem_id=semget(SEM_KEY,3, IPC_CREAT | 0666); // semget - (semaphore get) szuka zestawu semaforów, SEM_KEY - klucz do semafora
    // 3 - liczba semaforów w zestawie (EMPTY, FULL, MUTEX), 0666 - uprawnienia (odczyt i zapis dla wszystkich)
    if(sem_id==-1)
        handle_error("[P3] Błąd funkcji semget", 1);
    char buf[64];
    sprintf(buf, "[P3] Uzyskano dostęp do semaforów. ID: %d", sem_id);
    debug(buf);
    return sem_id;
}

struct shared_data* attach_memory() {
    int shmid=shmget(SHM_KEY, sizeof(struct shared_data), 0666); //Pobieramy ID istniejącej pamięci
    if (shmid==-1) 
        handle_error("[P3] Błąd shmget", 1);
    char buf[128];
    sprintf(buf, "[P3] Znaleziono pamięć dzieloną (SHM). ID: %d", shmid);
    debug(buf);
    struct shared_data *shm=(struct shared_data *)shmat(shmid, NULL, 0); // Rzutujemy wynik (void*) na wskaźnik do naszej struktury (struct shared_data*)
    if (shm==(void *)-1) {
        handle_error("[P3] Błąd shmat", 1);
    }
    debug("[P3] Pamięć została pomyślnie dołączona.");
    return shm;
}