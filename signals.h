#ifndef SIGNALS_H
#define SIGNALS_H

#include "ipc.h"
#include <unistd.h>
#include <signal.h>

#define P1 0
#define P2 1
#define P3 2

#define PAUSE 0
#define TERM 1

#define READ 0
#define WRITE 1

extern int pipes[3][2];
extern pid_t pid[3];

extern volatile sig_atomic_t PM_got_signal;
extern volatile sig_atomic_t PM_last_signal;
extern volatile sig_atomic_t PM_sender_pid;

extern volatile sig_atomic_t status[P3 + 1][TERM + 1];


void p1_notify_handler(int sig);
void p2_notify_handler(int sig);
void p3_notify_handler(int sig);
void p2_out_signal_handler(int sig);
void pm_signal_handler(int sig, siginfo_t* info);

void handle_atomics(int sig, volatile sig_atomic_t* paused, volatile sig_atomic_t* term);
void notify(pid_t pid);
void wait_if_paused(volatile sig_atomic_t *paused, volatile sig_atomic_t *term, const char* who);
void parent_send_control(int sig);


#endif // SIGNALS_H