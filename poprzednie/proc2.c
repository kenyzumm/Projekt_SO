#include "ipc.h"


void args(int argc, char* argv[]);
void setup_signals();
void print_start_info();
void run_loop(int semid, struct shared_data *shm);
void cleanup();
int get_semaphores();
struct shared_data* attach_memory();


int main(int argc, char* argv[]) {
    // 1. Konfiguracja
    args(argc, argv);
    setup_signals();

    // 2. Pobranie danych
    int semid = get_semaphores();
    struct shared_data *shm = attach_memory();
    
    // 3. Wysłanie danych
    print_start_info();
    run_loop(semid, shm);

    // 4. Wyjście
    cleanup();
    return 0;
}

void args(int argc, char* argv[]) {}
void setup_signals() {}
void print_start_info() {}
void run_loop(int semid, struct shared_data *shm) {
    char buffer[256];
    P_FULL;
    P_MUTEX;
    strcpy(buffer, shm->text);
    printf("[P2] Otrzymano: '%s'\n", buffer);
    V_MUTEX;
    V_EMPTY;
}
void cleanup() {}