#include "utils.c"
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

struct shell_state {
    int fport;
    int sport;
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

void shell_loop()
{
    struct shell_state *shell_state = init_shell_state();

    do {
        printf("> ");

        // Get line from user
        char* command = get_user_input();
        if (str_eq(command, ""))
            continue;

        // send command to server

        // free stuff
        free(command);
    } while (1); // TODO break at some point
    free(shell_state);
}

int client()
{
    int fd, status, valread;
    struct sockaddr_in serv_addr;
    char* hello = "Hello from client";
    char buffer[1024] = "";
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        c_log(ERR, "Socket creation error.");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        c_log(ERR, "Invalid address/Address not supported");
        return -1;
    }

    if ((status = connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        c_log(ERR, "Connection failed.");
        return -1;
    }

    send(fd, hello, strlen(hello), 0);
    valread = read(fd, buffer, 1024 - 1); // subtract 1 for null terminator at the end
    c_log(INFO, buffer);

    close(fd);
    return 0;
}

int main(int argc, char** argv) {
    return client();
}
