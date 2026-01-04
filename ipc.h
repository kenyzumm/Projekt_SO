#ifndef IPC_H
#define IPC_H

extern int debug_mode = 1;

void init_ipc(void);
void cleanup_ipc(void);
void proc1();
void proc2();
void proc3();

void handle_error(const char* msg, int exit_code);
void debug(const char* msg);

#endif // IPC_H