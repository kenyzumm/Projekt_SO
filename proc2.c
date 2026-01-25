#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

void process_p2(int fd_pipe, pid_t p1) {

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);




    

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
