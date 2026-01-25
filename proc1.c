#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

/* ZMIENNE GLOBALNE  */
int pipe_fd;
volatile sig_atomic_t running = 1;
volatile sig_atomic_t paused  = 0;

/* HANDLERY SYGNAŁÓW  */

/*
 * Obsługuje sygnał SIGUSR1 - odbiera dane z pipe'a od procesu P2
 * i wyświetla je na standardowe wyjście. Każda wartość w osobnym wierszu.
 * Ignoruje sygnał jeśli proces jest wstrzymany (paused).
 */
void sigusr1_handler(int sig) {
    int value;

    if (paused)
        return;

    if (read(pipe_fd, &value, sizeof(int)) == sizeof(int)) {
        /* KAŻDA JEDNOSTKA DANYCH W OSOBNYM WIERSZU */
        printf("%d\n", value);
        fflush(stdout);
    }
}

/*
 * Obsługuje sygnał SIGTSTP - wstrzymuje działanie procesu P1.
 * Ustawia flagę paused na 1, co powoduje ignorowanie sygnałów SIGUSR1.
 */
void sigtstp_handler(int sig) {
    paused = 1;
}

/*
 * Obsługuje sygnał SIGCONT - wznawia działanie procesu P1.
 * Resetuje flagę paused na 0, umożliwiając ponowne przetwarzanie sygnałów.
 */
void sigcont_handler(int sig) {
    paused = 0;
}

/*
 * Obsługuje sygnał SIGTERM - kończy działanie procesu P1.
 * Ustawia flagę running na 0, co powoduje wyjście z pętli głównej.
 */
void sigterm_handler(int sig) {
    running = 0;
}

/*  proces P1  */

/*
 * Główna funkcja procesu P1 - odbiera dane z pipe'a od procesu P2
 * i wyświetla je na standardowe wyjście.
 * 
 * Parametry:
 *   fd - deskryptor pliku pipe'a do odczytu danych od P2
 * 
 * Działanie:
 *   - Konfiguruje handlery sygnałów (SIGUSR1, SIGTSTP, SIGCONT, SIGTERM)
 *   - Czeka w pętli na sygnały (pause())
 *   - Na SIGUSR1 odbiera i wyświetla dane z pipe'a
 *   - Na SIGTERM kończy działanie i zamyka zasoby
 */
void process_p1(int fd) {
    pipe_fd = fd;

    signal(SIGUSR1, sigusr1_handler); // dane od P2
    signal(SIGTSTP, sigtstp_handler); // wstrzymanie
    signal(SIGCONT, sigcont_handler); // wznowienie
    signal(SIGTERM, sigterm_handler); // zakończenie

    printf("[P1] PID: %d — gotowy do odbioru danych\n", getpid());

    while (running) {
        pause();   // reakcja WYŁĄCZNIE na sygnały
    }

    close(pipe_fd);
    printf("[P1] Zakończenie poprawne\n");
}

/*  MAIN  */

/*
 * Funkcja main procesu P1 - inicjalizuje proces i uruchamia główną pętlę.
 * 
 * Argumenty:
 *   argc - liczba argumentów
 *   argv - tablica argumentów (oczekiwany: <FD_PIPE>)
 * 
 * Zwraca:
 *   0 - sukces
 *   1 - błąd (nieprawidłowa liczba argumentów)
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Użycie: %s <FD_PIPE>\n", argv[0]);
        exit(1);
    }

    process_p1(atoi(argv[1]));
    return 0;
}
