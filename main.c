#include <stdio.h>
#include <unistd.h>

int debug_mode = 1;

void handle_error(const char* msg, int exit_code);
void debug(const char* msg);

int main(int argc, char* argv[]) {
    pid_t pids[3] = {-2, -2, -2};

    // tworzenie procesow
    pids[0] = fork();
    switch (pids[0]) {
        case -1:
            handle_error("fork failed", 1);
        break;

        case 0:
            debug("Child 1 created successfully");
        break;

        default:
            pids[1] = fork();
            switch (pids[1]) {
                case -1:
                    handle_error("fork failed", 1);
                break;
                case 0:
                    debug("Child 2 created successfully");
                break;
                default:
                    pids[2] = fork();
                    switch (pids[2]) {
                        case -1:
                            handle_error("fork failed", 1);
                        break;
                        case 0:
                            debug("Child 3 created successfully");
                        break;
                    }
            }
            break;
    }    
}