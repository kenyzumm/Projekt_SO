#ifndef SIGNALS_H
#define SIGNALS_H

#include "ipc.h"
#include <unistd.h>
#include <signal.h>

#define PAUSE 0
#define TERM 1


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
void wait_if_paused(volatile sig_atomic_t *paused, volatile sig_atomic_t *term);
void parent_send_control(int sig);


#endif // SIGNALS_H