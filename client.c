#ifndef SHELL
#define SHELL

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#include "utils.c"

struct shell_state {
    int fport;
    int sport;
    int conn;
};

struct shell_state* init_shell_state()
{
    struct shell_state *s = malloc(sizeof(struct shell_state));
    if (!s) {
        c_log(ERR, "malloc error.");
        return NULL;
    }

    s->fport = 9001;
    s->sport = 9002;
    s->conn = -1;
    return s;
}

char* get_user_input()
{
    const int buffsize = 1024;
    int position = 0;
    char* buffer = malloc(sizeof(char) * buffsize);
    if (!buffer) {
        c_log(ERR, "malloc error.");
        return NULL;
    }

    char c;

    while (1) {
        c = getchar();

        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position++] = c;
        }
    }
}

void client_loop(int conn)
{
    int running = 1;
    do {
        printf("> ");

        // Get line from user
        char* command = get_user_input();
        if (str_eq(command, "")) {
            free(command);
            continue;
        }
        if (str_eq(command, "QUIT")) {
            running = 0;
        }

        send(conn, command, strlen(command), 0);

        int buffsize = 1024;
        char buffer[1024] = "";
        read(conn, buffer, buffsize); // subtract 1 for null terminator at the end
        printf("%s\n", buffer);

        // free stuff
        free(command);
    } while (running); 
}

int get_con(int port)
{
    int fd;
    struct hostent* info;
    struct sockaddr_in serv_addr;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        c_log(ERR, "Socket creation error.");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if ((info = gethostbyname("localhost")) == NULL) {
        c_log(ERR, "Could not resolve host.");
        return -1;
    }
    memcpy(&serv_addr.sin_addr, info->h_addr, info->h_length);

    puts("Connecting");
    if (connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        c_log(ERR, "Connection failed.");
        return -1;
    }

    return fd;
}

int main(int argc, char** argv) {
    printf("Choose a port to connect to: ");
    char* p_str = get_user_input();
    int port = atoi(p_str);
    free(p_str);

    int conn = get_con(port);
    if (conn < 0) {
        exit(EXIT_FAILURE);
    }

    client_loop(conn);
    close(conn);

    return EXIT_SUCCESS;
}

#endif // SHELL
