#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

/* STAŁE */
#define SHM_NAME "/shm_p3_p2"
#define SEM_FULL "/sem_p3_p2_full"
#define SEM_EMPTY "/sem_p3_p2_empty"
#define BUFFER_SIZE 1024

/* ZMIENNE GLOBALNE  */
int pipe_fd;
pid_t parent_pid;
pid_t p1_pid;

char *shm_ptr;
sem_t *sem_full;
sem_t *sem_empty;

volatile sig_atomic_t running = 1;
volatile sig_atomic_t paused  = 0;

/* HANDLERY SYGNAŁÓW  */

/*
 * Obsługuje sygnał SIGUSR1 - przekazuje sygnał START do procesu rodzica.
 * Proces P2 działa jako pośrednik, przekazując sygnał dalej w górę hierarchii.
 */
void sigusr1_handler(int sig) {
    /* sygnał START – przekazujemy do rodzica */
    kill(parent_pid, sig);
}

/*
 * Obsługuje sygnał SIGTSTP - wstrzymuje działanie procesu P2.
 * Ustawia flagę paused na 1, co powoduje pomijanie przetwarzania danych.
 */
void sigtstp_handler(int sig) {
    paused = 1;
}

/*
 * Obsługuje sygnał SIGCONT - wznawia działanie procesu P2.
 * Resetuje flagę paused na 0, umożliwiając ponowne przetwarzanie danych.
 */
void sigcont_handler(int sig) {
    paused = 0;
}

/*
 * Obsługuje sygnał SIGTERM - kończy działanie procesu P2.
 * Ustawia flagę running na 0 i wysyła SIGTERM do procesu P1,
 * co powoduje zakończenie całego łańcucha procesów.
 */
void sigterm_handler(int sig) {
    running = 0;
    kill(p1_pid, SIGTERM);
}

/* proces P2 */

/*
 * Główna funkcja procesu P2 - odbiera dane z pamięci dzielonej od P3,
 * liczy długość tekstu i przekazuje wynik do procesu P1 przez pipe.
 * 
 * Parametry:
 *   fd_pipe - deskryptor pliku pipe'a do zapisu danych do P1
 *   p1      - PID procesu P1, do którego wysyłane są sygnały i dane
 * 
 * Działanie:
 *   - Konfiguruje handlery sygnałów
 *   - Otwiera pamięć dzieloną (shared memory) i mapuje ją
 *   - Otwiera semafory do synchronizacji z P3
 *   - W pętli: czeka na dane od P3, liczy znaki, wysyła wynik do P1
 *   - Na zakończenie zwalnia wszystkie zasoby (munmap, close, sem_close)
 */
void process_p2(int fd_pipe, pid_t p1) {
    int shm_fd;
    int length;

    pipe_fd   = fd_pipe;
    p1_pid    = p1;
    parent_pid = getppid();

    /* sygnały */
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGCONT, sigcont_handler);
    signal(SIGTERM, sigterm_handler);

    /* pamięć dzielona */
    shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    shm_ptr = mmap(NULL, BUFFER_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);

    /* semafory */
    sem_full  = sem_open(SEM_FULL, 0);
    sem_empty = sem_open(SEM_EMPTY, 0);

    printf("[P2] PID: %d — gotowy\n", getpid());

    while (running) {

        if (paused)
            continue;

        /* czekamy na dane od P3 */
        sem_wait(sem_full);

        /* liczymy znaki */
        length = strlen(shm_ptr);
        if (length > 0 && shm_ptr[length - 1] == '\n')
            length--;

        /* wysyłamy wynik do P1 */
        write(pipe_fd, &length, sizeof(int));
        kill(p1_pid, SIGUSR1);

        /* zwalniamy bufor dla P3 */
        sem_post(sem_empty);
    }

    /* sprzątanie */
    munmap(shm_ptr, BUFFER_SIZE);
    close(shm_fd);
    close(pipe_fd);

    sem_close(sem_full);
    sem_close(sem_empty);

    printf("[P2] Zakończony poprawnie\n");
}

/*  MAIN  */

/*
 * Funkcja main procesu P2 - inicjalizuje proces i uruchamia główną pętlę.
 * 
 * Argumenty:
 *   argc - liczba argumentów
 *   argv - tablica argumentów (oczekiwany: <FD_PIPE> <PID_P1>)
 * 
 * Zwraca:
 *   0 - sukces
 *   1 - błąd (nieprawidłowa liczba argumentów)
 */
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Użycie: %s <FD_PIPE> <PID_P1>\n", argv[0]);
        exit(1);
    }

    process_p2(atoi(argv[1]), atoi(argv[2]));
    return 0;
}
