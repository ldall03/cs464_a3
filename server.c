#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.c"

int main()
{
    int fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = "";
    char* hello = "Hell from server";

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        c_log(ERR, "Socket failed.");
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        c_log(ERR, "Could not set socket options.");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        c_log(ERR, "Socket bind failed.");
        return -1;
    }

    if(listen(fd, 3) < 0) {
        c_log(ERR, "Failed to listen.");
        return -1;
    }

    if ((new_socket = accept(fd, (struct sockaddr*)&address, &addrlen)) < 0) {
        c_log(ERR, "Failed to accept.");
        return -1;
    } 
    valread = read(new_socket, buffer, 1024 - 1); // Subtract 1 for the null terminator at the end
    c_log(INFO, buffer);
    send(new_socket, hello, strlen(hello), 0);

    close(new_socket);
    close(fd);

    return 0;
}
