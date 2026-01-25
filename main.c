#include "signals.h"
#include <fcntl.h>

#define KEY ftok("/tmp", 'x')
int get_semaphores() {
    key_t key = KEY;
    int s = ;
    if (s == -1) exit(1);
    return s;
}
struct shared_data* get_shared_memory() {
    
}


int main(int argc, char* argv[]) {
    const char* file_path = (argc >=2) ? argv[1] : NULL;
    key_t key = ftok("/tmp", 'x');

    // przygotowac komunikacje
    for (int i=0; i<3; i++) {
        pipe(pipes[i]);
    }
    sem = get_semaphores();
    shm = get_shared_memory();
    msg = get_message_queue();


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
        process_p3();
        exit(0);
    }

    for (int i=0; i<3; i++) close(pipes[i][READ]);

    for (int i=0; i<3; i++) {
        wait(NULL);
    }
    return 0;
}