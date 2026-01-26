#ifndef IPC_H
#define IPC_H

#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024
#define KEY ftok("/tmp", 'a')

// Semaphores
#define SEM_EMPTY 0
#define SEM_FULL 1

struct shared {
    char buf[BUFFER_SIZE];
    int ec; // Exit code/status: 0 = OK, -1 = END
};

struct msgbuf {
    long type;
    int data;
};

#ifdef __linux__
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};
#endif

// IPC functions
void get_semaphores();
void get_shm();
void get_msg();
void P(int sem_num);
void V(int sem_num);

// IPC resources (extern for access from other modules)
extern int sem;
extern int shm_id;
extern int msg;
extern struct shared* shm;

#endif // IPC_H
