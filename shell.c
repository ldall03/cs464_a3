#include <unistd.h>
#include <wait.h>
#include <poll.h>

#include "utils.c"

void run_extern(char** args)
{
    // Create default search paths
    char path1[256] = "/bin/";
    char path2[256] = "/usr/bin/";
    strcat(path1, args[0]);
    strcat(path2, args[0]);

    // Try every path, including the absolute path if default does not work
    if (execve(path1, args, NULL) == 0) {
    } else if (execve(path2, args, NULL) == 0) {
    } else if (execve(args[0], args, NULL) == 0) {
    } else {
        printf("Command not recognized or could not be executed\n");
        exit(11);
    }
}

int run_cmd(char* cmd, char* buffer, int buffsize, int* status) {
    // Separate the string
    char** args = split(cmd);

    int p[2];
    if (pipe(p) < 0) {
        c_log(ERR, "Could not create pipe");
        return -1;
    }

    int pid;
    if ((pid = fork()) < 0) {
        c_log(ERR, "Could not fork");
        close(p[0]);
        close(p[1]);
        return -1;
    }

    if (pid == 0) {
        close(p[0]); // Close reading end
        
        dup2(p[1], 1); // redirect stdout to pipe
        dup2(p[1], 2); // redirect stderr to pipe

        run_extern(args);
    }

    wait(status);
    *status = WEXITSTATUS(*status);

        struct pollfd pollfd;
        pollfd.fd = p[0];
        pollfd.events = POLLIN;
        int ret = poll(&pollfd, 1, 100);
        int valread;
        switch (ret) {
            case -1: break;
            case 0: break;
            default:
                valread = read(p[0], buffer, buffsize);
        }
    close(p[1]); // Close writing end
    close(p[0]); // Close reading end

    return valread;
}
