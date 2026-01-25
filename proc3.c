/* proc3.c */
#include "ipc.h"
#include "signals.h"

FILE *input_source = NULL;

int pipe_fm;
pid_t p2_pid;

// --- Prototypy funkcji ---
void parse_args(int argc, char* argv[]);
void init_ipc_resources(int* sem_id, struct shared** shm);
void setup_signal_handling();
int  get_input_data(char* buffer);
int  write_to_shm(int sem_id, struct shared* shm, const char* buffer);
void send_termination_marker(int sem_id, struct shared* shm);
void loop(int sem_id, struct shared* shm);

// ============================================================================
// MAIN
// ============================================================================
int main(int argc, char* argv[]) {
    // 1. Parsowanie argumentów
    parse_args(argc, argv);

    int sem_id;
    struct shared* shm;
    
    // 2. Inicjalizacja zasobów IPC (Tylko semafory i pamięć!)
    init_ipc_resources(&sem_id, &shm);

    // 3. Konfiguracja sygnałów i łącza sterującego
    setup_signal_handling();

    // 4. Główna pętla przetwarzania
    loop(sem_id, shm);

    if (input_source && input_source != stdin) {
        fclose(input_source);
    }

    printf("[P3] Koniec procesu\n");
    return 0;
}

// --- Obsługa argumentów linii poleceń ---
void parse_args(int argc, char* argv[]) {
    // Sprawdzamy pierwszy argument (argv[1]) jako ścieżkę do pliku
    if (argc > 1 && strcmp(argv[1], "NULL") != 0) {
        input_source = fopen(argv[1], "r");
        if (!input_source) {
            perror("[P3] Błąd otwarcia pliku, przełączam na stdin");
            input_source = stdin;
        }
    } else {
        input_source = stdin; // Domyślnie stdin
    }
}

// --- Inicjalizacja IPC ---
void init_ipc_resources(int* sem_id, struct shared** shm) {
    *sem_id = get_semaphore();
    if (*sem_id == -1) { perror("[P3] semget"); exit(1); }
    
    *shm = get_shared_memory();
    if (!*shm) { perror("[P3] shmget"); exit(2); }

}

// --- Konfiguracja sygnałów ---
void setup_signal_handling() {
    pipes[P1][READ] = pipe_fm;
    pid[P2] = p2_pid;
    
    struct sigaction sa;
    sa.sa_handler = p3_notify_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; 
    sigaction(SIGUSR1, &sa, NULL);
}

// --- Główna pętla przetwarzania ---
void loop(int sem_id, struct shared* shm) {
    char buffer[BUFFER_SIZE];

    while (1) {
        wait_if_paused(&status[P3][PAUSE], &status[P3][TERM], "P3");
        
        if (status[P3][TERM]) break;

        if (!get_input_data(buffer)) {
            break; 
        }

        int write_status = write_to_shm(sem_id, shm, buffer);
        
        if (write_status == 1) continue; 
        if (write_status == -1) break;

        sleep(1);
    }

    if (!status[P3][TERM]) {
        send_termination_marker(sem_id, shm);
    }
}

// --- Wysłanie znacznika końca danych ---
void send_termination_marker(int sem_id, struct shared* shm) {
    P_EMPTY;
    P_MUTEX;
    shm->buf[0] = 0; // Pusty string jako znacznik końca
    shm->len = -1;
    V_MUTEX;
    V_FULL;
    printf("[P3] KONIEC DANYCH\n");
}

// --- Pobieranie danych (opakowanie safe_fgets) ---
int get_input_data(char* buffer) {
    while (1) {
        wait_if_paused(&status[P3][PAUSE], &status[P3][TERM], "P3");
        if (status[P3][TERM]) return 0;

        errno = 0;
        // Czytamy z wybranego źródła (plik lub stdin)
        if (fgets(buffer, BUFFER_SIZE, input_source) == NULL) {
            if (errno == EINTR) continue; 
            return 0; 
        }
        
        remove_newline(buffer);

        if (strcmp(buffer, ".") == 0) {
            return 0;
        }
        return 1;
    }
}

// --- Sekcja krytyczna: Semafor P -> Zapis -> Semafor V ---
int write_to_shm(int sem_id, struct shared* shm, const char* buffer) {
    
    // A. Opuszczenie semafora EMPTY (czekamy na miejsce)
    if (P_EMPTY == -1) { 
         if (errno == EINTR) return 1; // Przerwano sygnałem -> Retry
         return -1; // Prawdziwy błąd
    }
    
    // B. Opuszczenie semafora MUTEX (dostęp wyłączny)
    if (P_MUTEX == -1) { 
         V_EMPTY; // ROLLBACK
         if (errno == EINTR) return 1; // Retry
         return -1; // Error
    }

    // C. Zapis danych
    strncpy(shm->buf, buffer, BUFFER_SIZE - 1);
    shm->buf[BUFFER_SIZE - 1] = '\0';

    shm->len = strlen(shm->buf);

    // D. Podniesienie semaforów
    V_MUTEX;
    V_FULL; 
    
    return 0; // Sukces
}