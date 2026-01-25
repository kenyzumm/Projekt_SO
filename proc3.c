/* proc3.c */
#include "ipc.h"
#include "signals.h"

// --- Zmienne globalne lokalne dla P3 ---
int pipe_fm;   // Deskryptor potoku od Main
pid_t p2_pid;  // PID procesu P2 do propagacji

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

    // 5. Sprzątanie
    close(pipe_fm);
    printf("[P3] Koniec procesu\n");
    return 0;
}

// --- Obsługa argumentów linii poleceń ---
void parse_args(int argc, char* argv[]) {
    if (argc != 3) {
        printf("[P3] Błąd arg: %d (oczekiwano pipe_fd, p2_pid)\n", argc);
        exit(1);
    }
    pipe_fm = atoi(argv[1]);
    p2_pid = atoi(argv[2]);
}

// --- Inicjalizacja IPC ---
void init_ipc_resources(int* sem_id, struct shared** shm) {
    *sem_id = get_semaphore();
    if (*sem_id == -1) { perror("[P3] semget"); exit(1); }
    
    *shm = get_shared_memory();
    if (!*shm) { perror("[P3] shmget"); exit(2); }

    // POPRAWKA: P3 NIE używa kolejki komunikatów (msq).
    // Usunięto kod init_msg_queue(), który był tutaj błędny.
}

// --- Konfiguracja sygnałów ---
void setup_signal_handling() {
    // 1. Konfiguracja dla signals.c - PROPAGACJA
    // Mówimy bibliotece signals, jaki jest PID następnego procesu (P2)
    pid_p2 = p2_pid; 
    // Lub jeśli w signals.h używasz zmiennej 'next_pid':
    // next_pid = p2_pid;

    // 2. Konfiguracja dla signals.c - STEROWANIE PRZEZ PIPE
    // Przepisujemy deskryptor rury do zmiennej globalnej z signals.h.
    // Dzięki temu wait_if_paused będzie wiedziało skąd czytać komendę.
    ctrl_pipe_fd = pipe_fm;

    // 3. Instalacja handlera
    // Używamy handlera, który obsłuży SIGUSR1 i odczyt z rury
    install_sa_handler(SIGUSR1, p3_notify_handler, 0);
}

// --- Pętla główna ---
void loop(int sem_id, struct shared* shm) {
    char buffer[BUFFER_SIZE];

    while (1) {
        // 1. Sprawdzenie pauzy (Busy Wait + Odczyt z Pipe w tle)
        wait_if_paused(&p3_paused, &p3_term, "P3");
        
        // Sprawdzenie flagi końca
        if (p3_term) break;

        // 2. Pobranie danych z wejścia
        if (!get_input_data(buffer)) {
            break; 
        }

        // 3. Zapis do pamięci współdzielonej (Sekcja Krytyczna)
        int write_status = write_to_shm(sem_id, shm, buffer);
        
        if (write_status == 1) continue; // Przerwano sygnałem -> restart pętli
        if (write_status == -1) break;   // Błąd krytyczny semaforów

        // 4. Symulacja pracy
        sleep(1);
    }

    // 5. Wysłanie znacznika końca
    if (!p3_term) {
        send_termination_marker(sem_id, shm);
    }
}

// --- Wysłanie znacznika końca danych ---
void send_termination_marker(int sem_id, struct shared* shm) {
    P_EMPTY;
    P_MUTEX;
    shm->buf[0] = 0; // Pusty string jako znacznik końca
    V_MUTEX;
    V_FULL;
    printf("[P3] KONIEC DANYCH\n");
}

// --- Pobieranie danych (opakowanie safe_fgets) ---
int get_input_data(char* buffer) {
    // safe_fgets korzysta z wait_if_paused, więc obsłuży też odczyt z rury
    if (!safe_fgets_interruptible(stdin, buffer, BUFFER_SIZE, &p3_paused, &p3_term)) {
        return 0; // EOF lub SIGTERM
    }
    
    // Usunięcie znaku nowej linii
    buffer[strcspn(buffer, "\n")] = 0;
    return 1;
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
    if (strlen(buffer) < sizeof(shm->buf)) {
        strcpy(shm->buf, buffer);
    } else {
        printf("[P3] Wiersz przycięty!\n");
        strncpy(shm->buf, buffer, sizeof(shm->buf)-1);
        shm->buf[sizeof(shm->buf)-1] = 0;
    }

    // D. Podniesienie semaforów
    V_MUTEX;
    V_FULL; 
    
    return 0; // Sukces
}