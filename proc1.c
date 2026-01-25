#include <stdio.h>
#include <stdlib.h>

#include "signals.h"



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
void process_p1() {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = p1_notify_handler;
    sigaction(SIGUSR1, &sa, NULL);

    signal(SIGTSTP, SIG_IGN);
    signal(SIGCONT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGINT,  SIG_IGN);
    signal(SIGRTMIN, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);

    printf("[P1] PID: %d — gotowy do odbioru danych\n", getpid());

    while (!status[P1][TERM]) {
        wait_if_paused(&status[P1][PAUSE], &status[P1][TERM]);
        struct msgbuf m;
        msgrcv(msg, &m, sizeof(m.data), 0, 0);
        if (m.type == -1) break;
        printf("[P1] Odczytano: %d\n", m.data);
    }

    close(pipes[P1][READ]);
    printf("[P1] Zakończenie poprawne\n");
}