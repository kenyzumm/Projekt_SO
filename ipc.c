#include "ipc.h"

#include <stdio.h>

void init_ipc(void) {}

void cleanup_ipc(void) {}

void proc1() {}
void proc2() {}
void proc3() {}

void handle_error(const char* msg, int exit_code) {
    printf("Error: %s\n", msg);
    exit(exit_code);
}
void debug(const char* msg) {
    if (debug_mode) {
        printf("[DEBUG]: %s\n", msg);
    }
}

//test