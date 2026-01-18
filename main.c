#include "ipc.h"

#include <stdio.h>
#include <unistd.h>

int debug_mode = 1;

int main(int argc, char* argv[]) {
    pid_t pids[3] = {-2, -2, -2};

    // tworzenie procesow
    pids[0] = fork();
    switch (pids[0]) {
        case -1:
            handle_error("[PM] fork failed", 1);
        break;

        case 0:
            debug("[PM] Child 1 created successfully");
        break;

        default:
            pids[1] = fork();
            switch (pids[1]) {
                case -1:
                    handle_error("[PM] fork failed", 1);
                break;
                case 0:
                    debug("[PM] Child 2 created successfully");
                break;
                default:
                    pids[2] = fork();
                    switch (pids[2]) {
                        case -1:
                            handle_error("[PM] fork failed", 1);
                        break;
                        case 0:
                            debug("[PM] Child 3 created successfully");
                        break;
                    }
            }
            break;
    }

    // zastapienie procesow potomnych odpowiednimi funkcjami
    if (pids[0] == 0) {
        proc1();
        _exit(0);
    } else if (pids[1] == 0) {
        proc2();
        _exit(0);
    } else if (pids[2] == 0) {
        proc3();
        _exit(0);
    }

    // proces rodzica czeka na zakonczenie procesow potomnych
    if (pids[0] > 0 && pids[1] > 0 && pids[2] > 0) {
        debug("[PM] Parent process waiting for children to finish");
        for (int i = 0; i < 3; i++) {
            waitpid(pids[i], NULL, 0);
        }
        debug("[PM] All child processes have finished");
    }
    return 0;
}