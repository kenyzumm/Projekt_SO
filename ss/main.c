#include "ipc.h"
#include "signals.h"
#include <sys/wait.h>

// Deklaracje funkcji procesów (żeby main je widział)
void process_p1();
void process_p2();
void process_p3(char* file_path);

int shm_id=-1;

void cleanup_handler(int sig) {
    printf("\n\n[Main] Przechwycono sygnał %d. Sprzątanie...\n", sig);

    // 1. Zabijamy procesy potomne (żeby nie wisiały)
    for (int i = 0; i < 3; i++) {
        if (pid[i] > 0) kill(pid[i], SIGTERM);
    }

    // 2. Usuwamy zasoby IPC
    if (sem != -1) {
        semctl(sem, 0, IPC_RMID);
        printf("[Main] Usunięto semafory.\n");
    }
    if (msg != -1) {
        msgctl(msg, IPC_RMID, NULL);
        printf("[Main] Usunięto kolejkę komunikatów.\n");
    }
    if (shm_id != -1) {
        shmdt(shm); // Odłączenie
        shmctl(shm_id, IPC_RMID, NULL); // Usunięcie z systemu
        printf("[Main] Usunięto pamięć współdzieloną.\n");
    }

    printf("[Main] Wyjście zakończone sukcesem.\n");
    exit(0);
}


int main(int argc, char* argv[]) {
    // wyłącza buforowanie standardowego wyjścia
    setbuf(stdout, NULL);
    
    // Zeby nie bylo warningow
    (void)argc;
    (void)argv;

    // Ustawienie handlera sprzątającego na sygnały zakończenia
    //signal(SIGINT, cleanup_handler);
    signal(SIGINT, SIG_IGN);

    // Ignoruj bezpośrednie Ctrl+Z
    signal(SIGTSTP, SIG_IGN);
    signal(SIGCONT, SIG_IGN);

    // Reaguj tylko na info od P2
    signal(SIGUSR2, parent_send_control);

    // Wznowienie (od P2) 
    signal(SIGUSR1, parent_send_control);
    signal(SIGTERM, parent_send_control);

    // TWORZENIE ZASOBÓW (Raz, na początku)
    key_t key = KEY; // Używamy klucza z ipc.h!

    // Semafory: Tworzymy 3 sztuki z prawami dostępu
    sem = semget(key, 3, IPC_CREAT | 0666);
    if (sem == -1) { perror("semget create"); exit(1); }
    
    // Inicjalizacja wartości semaforów
    semctl(sem, MUTEX, SETVAL, 1); // Mutex otwarty
    semctl(sem, EMPTY, SETVAL, 1); // Jest 1 miejsce wolne
    semctl(sem, FULL,  SETVAL, 0); // Jest 0 miejsc zajętych

    // Pamięć współdzielona
    shm_id = shmget(key, sizeof(struct shared), IPC_CREAT | 0666);
    if (shm_id == -1) { perror("shmget"); exit(1); }
    shm = (struct shared*)shmat(shm_id, NULL, 0);
    
    // Kolejka komunikatów
    msg = msgget(key, IPC_CREAT | 0666);
    if (msg == -1) { perror("msgget"); exit(1); }

    // 2. MENU I PĘTLA GŁÓWNA
    while (1) {
        char file_path[256];
        int num;
        
        printf("\nMENU\n1. STDIN\n2. Plik\n3. Wyjscie\nPodaj opcje: ");
        if (scanf("%d", &num) != 1) break;

        char* path_arg = NULL;

        if (num == 2) {
            printf("Podaj plik: ");
            scanf("%255s", file_path);
            path_arg = file_path;
        } else if (num == 3) {
            break;
        } else if (num != 1) {
            printf("Niepoprawna opcja\n");
            continue;
        }

        // Tworzenie rur
        for (int i=0; i<3; i++) pipe(pipes[i]);

        // Forkowanie procesów
        for (int i=0; i<3; i++) {
            pid[i] = fork();
            if (pid[i] == 0) {
                if (i == P1) process_p1();
                if (i == P2) process_p2();
                if (i == P3) process_p3(path_arg);
                exit(0);
            } else if (pid[i] == -1) {
                perror("fork"); exit(1);
            }
        }

        // Rodzic czeka
        // Rodzic czeka na 3 procesy (odpornie na sygnały!)
        int active_children = 3;
        while (active_children > 0) {
            pid_t w = wait(NULL);
            
            if (w == -1) {
                // Jeśli wait został przerwany przez sygnał (np. od P2), to nie błąd!
                // Po prostu wracamy do czekania.
                if (errno == EINTR) {
                    continue; 
                } else {
                    // Inny błąd (np. brak dzieci)
                    break;
                }
            }
            // Jeśli wait zwrócił PID (>0), to znaczy, że proces naprawdę się zakończył
            active_children--;
        }
        printf("--- Koniec cyklu ---\n");
        for (int i=0; i<3; i++) {
            close(pipes[i][READ]);
            close(pipes[i][WRITE]);
        }
    }

    cleanup_handler(0);

    return 0;
}