/* proc2.c */
#include "ipc.h"
#include "signals.h"

void process_p2() {
    printf("[P2] PID: %d — gotowy\n", getpid());
    int sem_id = sem; // Dla makr

    // Sygnały
    struct sigaction sa;
    sa.sa_handler = p2_notify_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    while (1) {
        wait_if_paused(&status[P2][PAUSE], &status[P2][TERM]);
        if (status[P2][TERM]) break;

        // 1. Pobranie danych z SHM
        if (P_FULL == -1) { if (errno==EINTR) continue; break; }
        if (P_MUTEX == -1) { V_FULL; if (errno==EINTR) continue; break; }

        int data_len = shm->len; // Odczyt długości lub znacznika końca

        V_MUTEX;
        V_EMPTY; // Zwolnienie miejsca dla P3

        // 2. Obsługa końca
        if (data_len == -1) {
            // Prześlij znacznik końca do P1
            struct msgbuf m;
            m.type = 1;
            m.data = -1;
            msgsnd(msg, &m, sizeof(m.data), 0);
            break;
        }

        // 3. Wysłanie wyniku do P1 (przez Kolejkę Komunikatów!)
        struct msgbuf m;
        m.type = 1;
        m.data = data_len; // P2 tylko przekazuje długość
        
        if (msgsnd(msg, &m, sizeof(m.data), 0) == -1) {
            if (errno != EINTR) perror("msgsnd P2");
        }
        
        printf("[P2] Przetworzono długość: %d\n", data_len);
    }
    printf("[P2] Koniec\n");
}