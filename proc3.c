/* proc3.c */
#include "ipc.h"
#include "signals.h"

FILE *input_source = NULL;

// --- Prototypy funkcji ---
void parse_args(char* path);
void init_ipc_resources(int* sem_id, struct shared** shm);
void setup_signal_handling();
int  get_input_data(char* buffer);
int  write_to_shm(int sem_id, struct shared* shm, const char* buffer);
void send_termination_marker(int sem_id, struct shared* shm);
void loop(int sem_id, struct shared* shm);

// ============================================================================
// MAIN
// ============================================================================
void process_p3(char* file_path) {
    printf("[P3] Uruchomiono (PID: %d)\n", getpid());

    // 1. Ustawienie źródła danych (odpowiednik dawnego parsowania argumentów)
    parse_args(file_path);

    int sem_id;
    struct shared* shm_ptr;
    
    // 2. Pobranie zasobów IPC (zmiennych globalnych z main)
    init_ipc_resources(&sem_id, &shm_ptr);

    // 3. Konfiguracja sygnałów
    setup_signal_handling();

    // 4. Główna pętla przetwarzania
    loop(sem_id, shm_ptr);

    // 5. Sprzątanie
    if (input_source && input_source != stdin) {
        fclose(input_source);
    }

    printf("[P3] Koniec procesu\n");
}

// --- Obsługa argumentów linii poleceń ---
void parse_args(char* path) {
    // Sprawdzamy pierwszy argument (argv[1]) jako ścieżkę do pliku
    if (path != NULL && strcmp(path, "NULL") != 0) {
        input_source = fopen(path, "r");
        if (!input_source) {
            perror("[P3] Błąd otwarcia pliku, przełączam na stdin");
            input_source = stdin;
        } else {
            printf("[P3] Czytanie z pliku: %s\n", path);
        }
    } else {
        input_source = stdin; // Domyślnie stdin
        printf("[P3] Czytanie z STDIN\n");
    }
}

// --- Inicjalizacja IPC ---
void init_ipc_resources(int* sem_id, struct shared** shm_ptr) {
    // Korzystamy z globalnych zmiennych 'sem' i 'shm' stworzonych w main.c
    *sem_id = sem;
    *shm_ptr = shm;

    if (*sem_id == -1 || *shm_ptr == NULL) {
        fprintf(stderr, "[P3] Błąd: Zasoby IPC nie zostały poprawnie przekazane z main!\n");
        exit(1);
    }
}

// --- Konfiguracja sygnałów ---
void setup_signal_handling() {
    struct sigaction sa; // Ustawienie handlera dla SIGUSR1
    sa.sa_handler = p3_notify_handler; 
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; 
    sigaction(SIGUSR1, &sa, NULL);

    signal(SIGTSTP, SIG_IGN);
    signal(SIGCONT, SIG_IGN); 
    signal(SIGTERM, SIG_IGN); 
    signal(SIGINT,  SIG_IGN);
}

// --- Główna pętla przetwarzania ---
void loop(int sem_id, struct shared* shm) {
    char buffer[BUFFER_SIZE];

    while (1) {
        wait_if_paused(&status[P3][PAUSE], &status[P3][TERM]);
        
        if (status[P3][TERM]) break;

        if (!get_input_data(buffer)) {
            break; 
        }

        printf("[P3] Odczytano linię: \"%s\" (długość: %ld)\n", buffer, strlen(buffer));

        int write_status = write_to_shm(sem_id, shm, buffer);
        
        if (write_status == 1) continue; 
        if (write_status == -1) break;

        //sleep(1);
    }

    if (!status[P3][TERM]) {
        send_termination_marker(sem_id, shm);
    }
}

// --- Wysłanie znacznika końca danych ---
void send_termination_marker(int sem_id, struct shared* shm) {
    if (P_EMPTY == -1) return;
    if (P_MUTEX == -1) {
        V_EMPTY;
        return;
    }
    shm->buf[0] = '\0'; // Pusty string jako znacznik końca
    shm->len = -1;
    V_MUTEX;
    V_FULL;
    printf("[P3] KONIEC DANYCH\n");
}

// --- Pobieranie danych (opakowanie safe_fgets) ---
int get_input_data(char* buffer) {
    while (1) {
        wait_if_paused(&status[P3][PAUSE], &status[P3][TERM]);
        if (status[P3][TERM]) return 0;

        errno = 0;
        // Czytamy ze zmiennej input_source ustawionej w parse_args
        if (fgets(buffer, BUFFER_SIZE, input_source) == NULL) {
            if (errno == EINTR) continue; // Przerwano sygnałem -> Retry
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
         if (errno == EINTR) return 1; // Retry
         return -1; // Error
    }
    
    // B. Opuszczenie semafora MUTEX (dostęp wyłączny)
    if (P_MUTEX == -1) { 
         V_EMPTY; // Rollback
         if (errno == EINTR) return 1;
         return -1;
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