#ifndef IPC_H
#define IPC_H

#include <stdlib.h>

#define KEY_PATH "ipc.key"
#define KEY ftok(KEY_PATH, 'A')
#define MAX_LENGTH 1024

extern int debug_mode;

void init_ipc(void);
void cleanup_ipc(void);
void proc1();
void proc2();
void proc3();

void handle_error(const char* msg, int exit_code);
void debug(const char* msg);

#endif // IPC_H