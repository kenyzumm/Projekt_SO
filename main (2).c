#include "signals.h"
#include <fcntl.h>

#define KEY ftok("/tmp", 'x');


int main(int argc, char* argv[]) {
    
    key_t key = ftok("/tmp", 'x');

    // przygotowac komunikacje
    

    sem = get_semaphore();
    shm = get_shared_memory();
    msg = get_msg_queue();

    int pm_should_exit = 0;
    while (!pm_should_exit) {
        char* file_path = NULL;
        int num;
        printf("MENU\n");
        printf("1. STDIN\n");
        printf("2. Plik\n");
        printf("3. Wyjscie\n");
        printf("Podaj opcje: ");
        scanf("%d", &num);
        if (num == 2) {
            printf("Podaj plik: ");
            scanf("%s", file_path);
        } else if (num == 3) {
            break;
        } else if (num != 1) {
            printf("Podano niepoprawna opcje\n");
            continue;
        }

        for (int i=0; i<3; i++) pipe(pipes[i]);
        for (int i=0; i<3; i++) {
            pid[i] = fork();
            if (pid[i] == 0) {
                break;
            } else if (pid[i] == -1) {
                exit(1);
            }
        }

        if (pid[P1]==0) {
            process_p1();
            exit(0);
        } else if (pid[P2]==0) {
            process_p2();
            exit(0);
        } else if (pid[P3]==0) {
            process_p3(file_path);
            exit(0);
        }

        for (int i=0; i<3; i++) close(pipes[i][READ]);

        for (int i=0; i<3; i++) wait(NULL);
    }
    return 0;
}