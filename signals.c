#include "signals.h"

volatile sig_atomic_t PM_got_signal = 0;
volatile sig_atomic_t PM_last_signal = 0;
volatile sig_atomic_t PM_sender_pid = 0;

volatile sig_atomic_t status[3][2] = {
    {0,0}, {0,0}, {0,0}
};

void p1_notify_handler(int sig) {
    if (sig == SIGUSR1) {
        int got_signal;
        read(pipes[P1][READ], &got_signal, sizeof(int));
        handle_atomics(sig, &status[P1][PAUSE], &status[P1][TERM]);
    }
}
void p2_notify_handler(int sig) {
    if (sig == SIGUSR1) {
        int got_signal;
        read(pipes[P1][READ], &got_signal, sizeof(int));
        notify(pid[P1]);
        handle_atomics(got_signal, &status[P1][PAUSE], &status[P1][TERM]);
    }
}
void p3_notify_handler(int sig) {
    if (sig == SIGUSR1) {
        int got_signal;
        read(pipes[P1][READ], &got_signal, sizeof(int));
        notify(pid[P2]);
        handle_atomics(got_signal, &status[P1][PAUSE], &status[P1][TERM]);
    }
}

void p2_out_signal_handler(int sig) { // nwm
    pid_t pp = getppid();
    if (pp > 1) kill(pp, sig);
}
void pm_signal_handler(int sig, siginfo_t* info) {
    if (info) PM_sender_pid = info->si_pid;
    PM_last_signal = sig;
    PM_got_signal = 1;
}

void handle_atomics(int sig, volatile sig_atomic_t* paused, volatile sig_atomic_t* term) {
    switch (sig) {
        case SIGTSTP:
            *paused = 1;
        break;
        case SIGCONT:
            *paused = 0;
        break;
        case SIGTERM:
        case SIGINT:
            *paused = 0;
            *term = 1;
        break;
    }
}

void notify(pid_t pid) {
    if (pid > 0) {
        kill(pid, SIGUSR1);
    }
}
void wait_if_paused(volatile sig_atomic_t* paused, volatile sig_atomic_t* term, const char* who) {
    while (*paused && !*term) {
        pause();
    }
}
void parend_send_control(int sig) {
    for (int i=0; i<3; i++) {
        write(pipes[P3][WRITE], &sig, sizeof(int));
    }
    notify(pid[P3]);
}