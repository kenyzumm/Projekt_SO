#include "ipc.h" //rzeczy wspolne dla wszystkich procesow (klucze, kształ danych itp)


int debug_mode=1; // tryb debugowania
int p_read_par; // deskryptor do czytania od procesu rodzica (cos w stylu nr. identyfikacyjnego)
int p_write_p2; // deskryptor do pisania do procesu 2
pid_t pid_p2; // PID procesu 2

// handler sygnału od procesu rodzica
void handler_p3(int sig){
    int val; // zmienna do przechowywania odczytanej wartości
    int bytes_read=read(p_read_par, &val, sizeof(val)); // czytanie wartości od procesu rodzica
    if(bytes_read>0){
        debug("[P3] Otrzymano od Rodzica. Przesyłam do P2.");
        write(p_write_p2, &val, sizeof(val)); // przesyłanie wartości do procesu 2
        kill(pid_p2, SIGUSR2);  // sygnalizowanie procesu 2 o nowej wartości (jest to sygnał programowalny, który tutaj oznacza: "Sprawdź łącze od P3".)
    } else if(bytes_read==0){
        debug("[P3] Koniec danych od Rodzica. Zamykanie P3.");
        _exit(0); // koniec danych, natychmistowe zamknięcie procesu bez "sprzątania", system operacyjny sobie poradzi z posprzątaniem
    } else
        perror("[P3] Błąd funkcji read w handlerze");
}

// funkcja walidująca i parsująca argumenty wejściowe
void validate_and_parse_args(int argc, char* argv[]){
    if(argc<4){
        perror("[P3] Za mało argumentów. Oczekiwano: 4");
        _exit(1); // natychmiastowe zakończenie procesu z kodem błędu
    }
    p_read_par=atoi(argv[1]); // deskryptor do czytania od procesu rodzica
    p_write_p2=atoi(argv[2]); // deskryptor do pisania do procesu 2
    pid_p2=(pid_t)atoi(argv[3]); // PID procesu 2
    char buf[128];
    sprintf(buf, "[P3] Argumenty wczytane: ReadPipe=%d, WritePipe=%d, PID_P2=%d",p_read_par, p_write_p2, pid_p2);
    debug(buf);
}

void setup_signal(){
    signal(SIGSUSR1, handler_p3); // ustawienie handlera dla sygnału od procesu rodzica (Jeśli przyjdzie do mnie sygnał SIGUSR1, to natychmiast przerwać i uruchomić handler_p3)
    debug("[P3] Handler dla sygnału SIGUSR1 został pomyślnie ustawiony.");
}

int get_semaphores(){
    int sem_id=semget(SEM_KEY,3,0666); // semget - (semaphore get) szuka zestawu semaforów, SEM_KEY - klucz do semafora
    // 3 - liczba semaforów w zestawie (EMPTY, FULL, MUTEX), 0666 - uprawnienia (odczyt i zapis dla wszystkich)
    if(sem_id==-1){
        perror("[P3] Błąd funkcji semget");
        _exit(1);
    }
    char buf[64];
    sprintf(buf, "[P3] Uzyskano dostęp do semaforów. ID: %d", sem_id);
    debug(buf);   
    return sem_id;
}

void print_start_info() { // Funkcja informacyjna - dostosowuje komunikaty do trybu pracy
    if (isatty(STDIN_FILENO))
        printf("[P3] Tryb KLAWIATURY. Wpisz tekst i wciśnij ENTER:\n"); // Program uruchomiono ręcznie w konsoli. Użytkownik musi wiedzieć, że program czeka na jego działanie.
    else
        debug("[P3] Tryb PLIKU/POTOKU - działam w tle."); // Program czyta dane z pliku (./proc3 < plik.txt) lub z rury (|)..
}

struct shared_data* attach_memory() {
    int shmid=shmget(SHM_KEY, sizeof(struct shared_data), 0666); //Pobieramy ID istniejącej pamięci
    if (shmid==-1) {
        handle_error("[P3] shmget failed", 1);
    }
    char buf[128];
    sprintf(buf, "[P3] Znaleziono pamięć dzieloną (SHM). ID: %d", shmid);
    debug(buf);
    struct shared_data *shm=(struct shared_data *)shmat(shmid, NULL, 0); // Rzutujemy wynik (void*) na wskaźnik do naszej struktury (struct shared_data*)
    if (shm==(void *)-1) {
        handle_error("[P3] shmget failed", 1);
    }
    debug("[P3] Pamięć została pomyślnie dołączona.");
    return shm;
}

void run_producer_loop(int semid, struct shared_data *shm) {
    char buffer[256];
    char log_buf[300]; // Bufor pomocniczy na komunikaty debugowe
    debug("[P3] Rozpoczynam pętlę producenta. Czekam na dane z STDIN...");
    while(fgets(buffer, 256, stdin)!=NULL) { // Główna pętla czytania
        buffer[strcspn(buffer, "\n")]=0; //Opcjonalne: Usuwamy "Enter" (\n) z końca napisu
        sprintf(log_buf, "[P3] Wczytano z klawiatury: '%s'. Czekam na miejsce w buforze...", buffer);
        debug(log_buf);
        // Jeśli sem_op zwróci -1, to znaczy że wystąpił błąd systemowy
        sem_op(semid, 0, -1); // SEM_EMPTY - czekamy na wolne miejsce w buforze
        sem_op(semid, 2, -1); // Wejdź do sekcji krytycznej (MUTEX)
        if (strlen(buffer)<sizeof(shm->text)){ // Kopiujemy tekst. Sprawdzamy, czy nie jest za długi
            strcpy(shm->text, buffer);
            sprintf(log_buf, "[P3] Zapisano do pamięci dzielonej: '%s'", shm->text);
            debug(log_buf);
        } else
            fprintf(stderr, "[P3] BŁĄD: Tekst zbyt długi do bufora pamięci!\n");
        sem_op(semid, 2, 1); //Wyjdź z sekcji krytycznej
        sem_op(semid, 1, 1); //Powiadom P2, że są dane (SEM_FULL)
        debug("[P3] Powiadomiono P2 (SEM_FULL podniesiony).");
    }
    if(ferror(stdin)) // Po wyjściu z pętli sprawdzamy, dlaczego się skończyła
        perror("[P3] Błąd funkcji fgets (odczyt wejścia)");
    else
        debug("[P3] Koniec danych wejściowych (EOF)."); // Jeśli pętla zakończyła się przez koniec pliku
}

void cleanup(struct shared_data *shm) {
    debug("[P3] Sprzątanie: Próba odłączenia pamięci dzielonej...");
    if (shmdt(shm) == -1)  // shmdt zwraca -1 w przypadku błędu, a 0 gdy jest OK
        perror("[P3] Ostrzeżenie: Błąd funkcji shmdt");
    else 
        debug("[P3] Pamięć dzielona została poprawnie odłączona.");
}

int main(int argc, char *argv[]) {
    // 1. Konfiguracja
    validate_and_parse_args(argc, argv);
    setup_signals();

    // 2. Pobranie danych
    int semid = get_semaphores();
    struct shared_data *shm = attach_memory();

    // 3. Wysłanie danych
    print_start_info();
    run_producer_loop(semid, shm);

    // 4. Wyjście
    cleanup(shm);
    return 0;
}