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
        handle_atomics(got_signal, &status[P1][PAUSE], &status[P1][TERM]);
    }
}
void p2_notify_handler(int sig) {
    if (sig == SIGUSR1) {
        int got_signal;
        // P2 czyta ze SWOJEJ rury (P2), a nie P1
        read(pipes[P2][READ], &got_signal, sizeof(int)); 
        notify(pid[P1]); // Przekazuje dalej do P1 (jeśli tak ma być w łańcuchu)
        handle_atomics(got_signal, &status[P2][PAUSE], &status[P2][TERM]); // status[P2]!
    }
}
void p3_notify_handler(int sig) {
    if (sig == SIGUSR1) {
        int got_signal;
        // P3 czyta ze SWOJEJ rury (P3)
        read(pipes[P3][READ], &got_signal, sizeof(int)); 
        notify(pid[P2]); // Przekazuje do P2
        handle_atomics(got_signal, &status[P3][PAUSE], &status[P3][TERM]); // status[P3]!
    }
}

void p2_out_signal_handler(int sig) { // Kiedy ta funkcja się uruchomi (np. gdy wciśniesz Ctrl+Z w P2), Proces 2 wysyła do MAIN-a sygnał SIGUSR2
    (void)sig; // Unikamy ostrzeżenia o nieużywanej zmiennej
    pid_t pp = getppid();
    if (pp <= 1) return;

    if (sig == SIGTSTP) {
        // Jeśli Ctrl+Z -> wyślij prośbę o pauzę (umowny SIGUSR2)
        kill(pp, SIGUSR2);
    } 
    else if (sig == SIGCONT) {
        // Jeśli fg -> wyślij prośbę o wznowienie (SIGCONT)
        kill(pp, SIGUSR1);
    }
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

void wait_if_paused(volatile sig_atomic_t* paused, volatile sig_atomic_t* term) {
    while (*paused && !*term) {
        pause();
    }
}

//Problem "Podwójnego Echa" Gdy wciskasz Ctrl+Z w terminalu, system operacyjny (Linux) wysyła sygnał SIGTSTP do wszystkich procesów naraz (do Maina, P1, P2 i P3).

void parent_send_control(int sig) { //Odbiera sygnał od P2, tłumaczy go na rozkaz dla wszystkich procesów i wysyła go przez rury
    int command;

    if (sig == SIGUSR2) {
        command = SIGTSTP;  // Tłumaczymy: "USR2" znaczy "Zrób Pauzę"
    } else if (sig == SIGUSR1) {
        command = SIGCONT;  // Tłumaczymy: "USR1" znaczy "Wznów"
    } else {
        command = sig;      // Inny sygnał przesyłamy bez zmian
    }

    for (int i=0; i<3; i++) {
        write(pipes[i][WRITE], &command, sizeof(int));
    }
    notify(pid[P3]);
    if (command == SIGTSTP) {
        printf("\n[Main] P2 zlecił pauzę. Wykonuję.\n");
        
        // Techniczny trik, żebyś odzyskał terminal (znak zachęty $)
        // To nie łamie zasady - to tylko pozwala systemowi "zawiesić" okno,
        // bo cała logika pauzy w procesach już została uruchomiona przez P2.
        signal(SIGTSTP, SIG_DFL);// Przywracamy domyślną obsługę Ctrl+Z
        raise(SIGTSTP); // Wyślij ten sygnał sam do siebie w tym momencie.
        
        // --- TU PROGRAM CZEKA NA 'fg' ---
        
        signal(SIGTSTP, SIG_IGN); // Po powrocie znowu ignorujemy system
    }
    else {
        printf("\n[Main] P2 zlecił wznowienie. Pracujemy dalej.\n");
    }
}