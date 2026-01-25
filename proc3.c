/* proc3.c */
#include "ipc.h"
#include "signals.h"


int pipe_fm;   // Deskryptor potoku od Main
pid_t p2_pid;  // PID procesu P2 do propagacji

void trans_args(int argc, char* argv[]);
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
    trans_args(argc, argv);

    int sem_id;
    struct shared* shm;
    
    // 1. Inicjalizacja zasobów IPC
    init_ipc_resources(&sem_id, &shm);

    // 2. Konfiguracja sygnałów
    setup_signal_handling();

    // 3. Główna pętla przetwarzania
    loop(sem_id, shm);

    // 4. Sprzątanie
    close(pipe_fm);
    printf("[P3] Koniec procesu\n");
    return 0;
}

// Obsługa argumentów linii poleceń
void trans_args(int argc, char* argv[]) {
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

    // WAŻNE: Inicjalizacja kolejki komunikatów dla handlera p3_notify_handler (z signals.c)
    // Zmienna 'msq' jest zdefiniowana jako extern w ipc.h/signals.h
    msq = get_msg_queue();
    if (msq == -1) { perror("[P3] msgget"); exit(3); }
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


void loop(int sem_id, struct shared* shm) {
    char buffer[BUFFER_SIZE];

    while (1) {
        // 1. Sprawdzenie pauzy (Busy Wait na zmiennych atomowych)
        wait_if_paused(&p3_paused, &p3_term, "P3");
        
        // Sprawdzenie czy nie ma końca programu (flaga od handlera)
        if (p3_term) break;

        // 2. Pobranie danych z wejścia
        // Funkcja zwraca 0 w przypadku błędu/EOF/Termination, 1 w przypadku sukcesu
        if (!get_input_data(buffer)) {
            break; 
        }

        // 3. Zapis do pamięci współdzielonej (Sekcja Krytyczna)
        // Funkcja zwraca status: 0=OK, 1=RETRY (przerwane sygnałem), -1=ERROR
        int write_status = write_to_shm(sem_id, shm, buffer);
        
        if (write_status == 1) continue; // Przerwano sygnałem -> restart pętli
        if (write_status == -1) break;   // Błąd krytyczny semaforów

        // 4. Symulacja pracy
        sleep(1);
    }

    // 5. Wysłanie znacznika końca (jeśli nie zostaliśmy zabici sygnałem SIGTERM)
    if (!p3_term) {
        send_termination_marker(sem_id, shm);
    }
}

// --- Pobieranie danych (opakowanie safe_fgets) ---
int get_input_data(char* buffer) {
    // Ta funkcja sama sprawdzi pauzę, jeśli fgets zostanie przerwane sygnałem (EINTR)
    if (!safe_fgets_interruptible(stdin, buffer, BUFFER_SIZE, &p3_paused, &p3_term)) {
        return 0; // EOF lub SIGTERM
    }
    
    // Usunięcie znaku nowej linii
    buffer[strcspn(buffer, "\n")] = 0;
    return 1;
}

// --- Sekcja krytyczna: Semafor P -> Zapis -> Semafor V ---
// Zwraca: 0 (Sukces), 1 (Przerwano sygnałem - spróbuj ponownie), -1 (Błąd)
int write_to_shm(int sem_id, struct shared* shm, const char* buffer) {
    
    // A. Opuszczenie semafora EMPTY (czekamy na miejsce)
    if (P_EMPTY == -1) { 
         if (errno == EINTR) return 1; // Przerwano sygnałem -> Retry
         return -1; // Prawdziwy błąd semafora
    }
    
    // B. Opuszczenie semafora MUTEX (dostęp wyłączny)
    if (P_MUTEX == -1) { 
         // ROLLBACK: Jeśli nie udało się zająć MUTEX (np. przez sygnał),
         // musimy oddać zabrane wcześniej miejsce (V_EMPTY), żeby nie zgubić zasobu.
         V_EMPTY; 
         
         if (errno == EINTR) return 1; // Retry
         return -1; // Error
    }

    // C. Zapis danych (Kopiowanie do pamięci współdzielonej)
    if (strlen(buffer) < sizeof(shm->buf)) {
        strcpy(shm->buf, buffer);
    } else {
        printf("[P3] Wiersz przycięty!\n");
        strncpy(shm->buf, buffer, sizeof(shm->buf)-1);
        shm->buf[sizeof(shm->buf)-1] = 0;
    }

    // D. Podniesienie semaforów (Sygnalizacja)
    V_MUTEX; // Zwolnienie sekcji krytycznej
    V_FULL;  // Zasygnalizowanie P2, że są nowe dane
    
    return 0; // Sukces
}








// --- Konfiguracja sygnałów ---
void setup_signal_handling() {
    // Zapisujemy PIDy globalne (dla funkcji notify w signals.c)
    // Zmienna 'pid_p2' jest zdefiniowana jako extern
    pid_p2 = p2_pid;

    // Instalacja handlerów z signals.c
    install_sa_handler(SIGUSR1, p3_notify_handler, 0);
}