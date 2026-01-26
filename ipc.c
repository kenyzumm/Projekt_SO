#include "ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/sem.h>

// IPC resources
int sem;
int shm_id;
int msg;
struct shared* shm;

void get_semaphores() {
    key_t key = KEY;
    if (key == -1) {
        perror("ftok");
        exit(1);
    }
    sem = semget(key, 2, 0666 | IPC_CREAT); // 2 semafory (EMPTY, FULL)
    if (sem == -1) {
        perror("semget");
        exit(1);
    }
}

void get_shm() {
    key_t key = KEY;
    if (key == -1) {
        perror("ftok");
        exit(1);
    }
    shm_id = shmget(key, sizeof(struct shared), 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }
    shm = (struct shared *)shmat(shm_id, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat");
        exit(1);
    }
}

void get_msg() {
    key_t key = KEY;
    if (key == -1) {
        perror("ftok");
        exit(1);
    }
    msg = msgget(key, 0666 | IPC_CREAT);
    if (msg == -1) {
        perror("msgget");
        exit(1);
    }
}

// Semaphore P operation (Wait)
void P(int sem_num) {
    struct sembuf sb = {sem_num, -1, 0};
    while (semop(sem, &sb, 1) == -1) {
        if (errno == EINTR) continue;
        perror("semop P");
        exit(1);
    }
}

// Semaphore V operation (Signal)
void V(int sem_num) {
    struct sembuf sb = {sem_num, 1, 0};
    while (semop(sem, &sb, 1) == -1) {
        if (errno == EINTR) continue;
        perror("semop V");
        exit(1);
    }
}
