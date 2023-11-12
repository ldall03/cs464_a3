#ifndef SERVER
#define SERVER

#include <asm-generic/errno-base.h>
#include <netinet/in.h>
#include <signal.h>
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <wait.h>

#include "shell.c"
#include "files.c"

struct server_state {
    int f_port;
    int s_port;
    int max_threads;
    int delay;
    int verbose_on;
};

struct thread_struct {
    int socket;
    int* f_table;
    int id;
    int* active;
    struct FIO_handler* handler;
};

struct server_state* get_server_state()
{
    struct server_state* ss = malloc(sizeof(struct server_state));
    ss->f_port = 9001;
    ss->s_port = 9002;
    ss->max_threads = 5;
    ss->delay = 0;
    ss->verbose_on = 0;
    return ss;
}

int get_con(int port)
{
    int sock, ret_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int opt = 1;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        c_log(ERR, "Socket failed.");
        return -1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        c_log(ERR, "Could not set socket options.");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
        c_log(ERR, "Socket bind failed.");
        return -1;
    }

    if(listen(sock, 3) < 0) {
        c_log(ERR, "Failed to listen.");
        return -1;
    }

    // poll for accepting conn
    struct pollfd pollfd;
    pollfd.fd = sock;
    pollfd.events = POLLIN;
    int ret = poll(&pollfd, 1, 2000);
    switch (ret) {
        case -1: break;
        case 0: 
            ret_socket = -1; 
            break;
        default:
            if ((ret_socket = accept(sock, (struct sockaddr*)&address, &addrlen)) < 0) {
                c_log(ERR, "Failed to accept.");
                return -1;
            } 
    }

    close(sock);
    return ret_socket;
}

void get_ret_str(char* t, int status, char* msg, char* buffer)
{
    char n[8];
    sprintf(n, "%d", status);
    strcat(buffer, t);
    strcat(buffer, " ");
    strcat(buffer, n);
    strcat(buffer, " ");
    strcat(buffer, msg);
}

void* start_shell_thread(void* args)
{
    struct thread_struct* data = (struct thread_struct*) args;
    char last_output[1024] = "";
    int has_output = 0;
    c_log_thread(data->id, "starting", "");
    while (*data->active) {
        int buffsize = 1024;
        char rcv_buffer[1024] = "";
        int valread = -1;

        struct pollfd pollfd;
        pollfd.fd = data->socket;
        pollfd.events = POLLIN;
        int ret = poll(&pollfd, 1, 100);
        switch (ret) {
            case -1: break;
            case 0: break;
            default:
                valread = read(data->socket, rcv_buffer, buffsize);
                c_log_thread(data->id, "recieved: ", rcv_buffer);
        }

        if (!valread) {
            break;
        } else if (valread < 0) {
            continue;
        } else if (str_eq(rcv_buffer, "QUIT")) {
            break;
        } else if (str_eq(rcv_buffer, "CPRINT")) {
            char reply[1024] = "";
            if (!has_output) {
                get_ret_str("ERR", EIO, "Nothing to send back", reply);
                send(data->socket, reply, strlen(reply), 0);
            } else {
                char ret[1024] = "";
                get_ret_str("OK", 0, "Command successful", ret);
                strcat(reply, last_output);
                strcat(reply, "\n");
                strcat(reply, ret);
                send(data->socket, reply, strlen(reply), 0);
            }
            continue;
        }

        int status;
        memset(last_output, 0, 1024);
        run_cmd(rcv_buffer, last_output, 1024, &status);
        c_log_thread(data->id, "command returned: ", last_output);
        has_output = 1;

        char reply[1024] = "";
        if (status) {
            get_ret_str("ERR", status, "Command returned with an error", reply);
        } else {
            get_ret_str("OK", 0, "Command successful", reply);
        }
        c_log_thread(data->id, "return msg: ", reply);
        send(data->socket, reply, strlen(reply), 0);
    }
    c_log_thread(data->id, "terminating", "");

    close(data->socket);
    data->f_table[data->id] = 0;
    free(data);
    
    return 0;
}

void* start_file_thread(void* args)
{
    struct thread_struct* data = (struct thread_struct*) args;
    c_log_thread(data->id, "starting", "");

    while (*data->active) {
        int buffsize = 1024;
        char rcv_buffer[1024] = "";
        int valread = -1;
        int has_output = 0;

        struct pollfd pollfd;
        pollfd.fd = data->socket;
        pollfd.events = POLLIN;
        int ret = poll(&pollfd, 1, 100);
        switch (ret) {
            case -1: continue;
            case 0: continue;
            default:
                valread = read(data->socket, rcv_buffer, buffsize);
                c_log_thread(data->id, "recieved: ", rcv_buffer);
        }

        char** cmd_args = split(rcv_buffer);
        char reply[1024] = "";

        // Handle the command
        if (!valread) {
            break;
        } else if (str_eq(rcv_buffer, "QUIT")) {
            break;
        } else if (str_eq(cmd_args[0], "FOPEN")) {
            int status = open_file(data->handler, cmd_args[1]);
            if (status == -1) {
                get_ret_str("FAIL", status, "Could not open file", reply);
            } else if (cmd_args[1][0] == 0) {
                get_ret_str("ERR", status, "File was already opened", reply);
            } else {
                get_ret_str("OK", status, "File opened successful", reply);
            }
        } else if (str_eq(cmd_args[0], "FSEEK")) {
            int id = atoi(cmd_args[1]);
            int off = atoi(cmd_args[2]);
            int status = seek_file(data->handler, id, off);
            if (status == -1) {
                get_ret_str("FAIL", -1, "Failed to seek the file", reply);
            } else if (status == -2) {
                get_ret_str("ERR", ENOENT, "No such a file or directory", reply);
            } else {
                get_ret_str("OK", 0, "Seek successful", reply);
            }
        } else if (str_eq(cmd_args[0], "FREAD")) {
            int id = atoi(cmd_args[1]);
            int len = atoi(cmd_args[2]);
            char* buff = malloc(sizeof(char) * len);
            memset(buff, 0, len);
            int status = read_file(data->handler, id, len, buff);
            if (status == -1) {
                get_ret_str("FAIL", -1, "Could not read from file", reply);
            } else if (status == -2) {
                get_ret_str("ERR", ENOENT, "No such a file or directory", reply);
            } else {
                get_ret_str("OK", status, buff, reply);
            }
            free(buff);
        } else if (str_eq(cmd_args[0], "FWRITE")) {
            int id = atoi(cmd_args[1]);
            cmd_args += 2;
            int status = write_file(data->handler, id, cmd_args);
            if (status == -1) {
                get_ret_str("FAIL", -1, "Could not write to file", reply);
            } else if (status == -2) {
                get_ret_str("ERR", ENOENT, "No such a file or directory", reply);
            } else {
                get_ret_str("OK", 0, "Write successful", reply);
            }
        } else if (str_eq(cmd_args[0], "FCLOSE")) {
            int id = atoi(cmd_args[1]);
            int status = close_file(data->handler, id);
            if (status == -2) {
                get_ret_str("ERR", ENOENT, "No such a file or directory", reply);
            } else {
                get_ret_str("OK", 0, "File closed successfully", reply);
            }
        } else {
            get_ret_str("FAIL", -1, "Could not run command", reply);
        }
        send(data->socket, reply, strlen(reply), 0);
    }
    c_log_thread(data->id, "terminating", "");

    close(data->socket);
    data->f_table[data->id] = 0;
    free(data);
    
    return 0;
}

int shell_control_thread_loop(int t, int port, int p)
{
    int active = 1;
    int t_table[t];
    memset(t_table, 0, sizeof(int) * t);

    while (1) {
        // Check if server wants to terminate
        struct pollfd pollfd;
        pollfd.fd = p;
        pollfd.events = POLLIN;
        int ret = poll(&pollfd, 1, 100);
        char pipe_rcv[8] = "";
        switch (ret) {
            case -1: break;
            case 0: break;
            default:
                read(p, pipe_rcv, 8);
        }
        // Stop looping
        if (str_eq("q", pipe_rcv)) {
            c_log(VERBOSE, "file process stopping");
            active = 0;
            break;
        }

        int count = 0; // Count active threads
        c_log(VERBOSE_INLINE, "t_table: ");
        for (int i = 0; i < t; i++) {
            if (t_table[i] == 1) count++;
            char n[8];
            sprintf(n, "%d", t_table[i]);
            c_log(VERBOSE_INLINE, n);
        }
        c_log(VERBOSE_INLINE, "\n");

        if (count >= t) { // If threshhold reach do nothing
            c_log(VERBOSE, "thread max exceeded");
            sleep(2);
            continue;
        }

        c_log(VERBOSE, "Getting connection...");
        int sock = get_con(port);
        if (sock < 0) // If we did not get a connection loop back
            continue;
        c_log(VERBOSE, "Connection established");

        int id;
        for (int i = 0; i < t; i++) {
            if (t_table[i] == 0) {
                id = i;
                t_table[id] = 1;
                break;
            }
        }

        pthread_t thread;
        struct thread_struct* ts_p = malloc(sizeof(struct thread_struct));
        ts_p->socket = sock;
        ts_p->f_table = t_table;
        ts_p->id = id;
        ts_p->active = &active;
        pthread_create(&thread, NULL, start_shell_thread, (void*) ts_p);
    }

    return 0;
}

int file_control_thread_loop(int t, int port, int p)
{
    int active = 1;
    int t_table[t];
    memset(t_table, 0, sizeof(int) * t);
    
    pthread_mutex_t lock;
    pthread_cond_t cond;
    struct FIO_handler* file_handler = get_FIO_handler(&lock, &cond);
    if (file_handler < 0) {
        c_log(ERR, "Failed to create a file_handler");
    }

    while (1) {
        // Check if server wants to terminate
        struct pollfd pollfd;
        pollfd.fd = p;
        pollfd.events = POLLIN;
        int ret = poll(&pollfd, 1, 100);
        char pipe_rcv[8] = "";
        switch (ret) {
            case -1: break;
            case 0: break;
            default:
                read(p, pipe_rcv, 8);
        }
        // Stop looping
        if (str_eq("q", pipe_rcv)) {
            c_log(VERBOSE, "file process stopping");
            active = 0;
            break;
        }

        int count = 0; // Count active threads
        c_log(VERBOSE_INLINE, "t_table: ");
        for (int i = 0; i < t; i++) {
            if (t_table[i] == 1) count++;
            char n[8];
            sprintf(n, "%d", t_table[i]);
            c_log(VERBOSE_INLINE, n);
        }
        c_log(VERBOSE_INLINE, "\n");

        if (count >= t) { // If threshhold reach do nothing
            c_log(VERBOSE, "thread max exceeded");
            sleep(2);
            continue;
        }

        c_log(VERBOSE, "Getting connection...");
        int sock = get_con(port);
        if (sock < 0) // If we did not get a connection loop back
            continue;
        c_log(VERBOSE, "Connection established");

        int id;
        for (int i = 0; i < t; i++) {
            if (t_table[i] == 0) {
                id = i;
                t_table[id] = 1;
                break;
            }
        }

        pthread_t thread;
        struct thread_struct* ts_p = malloc(sizeof(struct thread_struct));
        ts_p->socket = sock;
        ts_p->f_table = t_table;
        ts_p->id = id;
        ts_p->active = &active;
        ts_p->handler = file_handler;
        pthread_create(&thread, NULL, start_file_thread, (void*) ts_p);
    }

    free_FIO_handler(file_handler);
    return 0;
}

int file_server(struct server_state* state, int p)
{
    file_control_thread_loop(state->max_threads, state->f_port, p);
    free(state);

    return 0;
}

int shell_server(struct server_state* state, int p)
{
    shell_control_thread_loop(state->max_threads, state->s_port, p);
    free(state);

    return 0;
}

int main(int argc, char** argv)
{
    struct server_state* state = get_server_state();
    for (int i = 0; i < argc; i++) {
        if (str_eq(argv[i], "-f"))
            state->f_port = atoi(argv[i+1]);
        if (str_eq(argv[i], "-s"))
            state->s_port = atoi(argv[i+1]);
        if (str_eq(argv[i], "-t"))
            state->max_threads = atoi(argv[i+1]);
        if (str_eq(argv[i], "-D")) {
            state->delay = 1;
            TOGGLE_DEBUG;
        }
        if (str_eq(argv[i], "-v")) {
            state->verbose_on = 1;
            TOGGLE_VERBOSE;
        }
    }

    int fp[2], sp[2];
    if (pipe(fp) < 0) {
        c_log(ERR, "Pipe failed.");
        exit(EXIT_FAILURE);
    }

    if (pipe(sp) < 0) {
        c_log(ERR, "Pipe failed.");
        close(fp[0]);
        close(fp[1]);
        exit(EXIT_FAILURE);
    }

    pid_t f_pid = fork();
    if (f_pid == 0) {
        close(fp[1]);
        close(sp[0]);
        close(sp[1]);
        file_server(state, fp[0]);
        close(fp[0]);
        c_log(VERBOSE, "file exited properly!");
        exit(EXIT_SUCCESS);
    }

    pid_t s_pid = fork(); 
    if (s_pid == 0) {
        close(sp[1]);
        close(fp[0]);
        close(fp[1]);
        shell_server(state, sp[0]);
        close(sp[0]);
        c_log(VERBOSE, "shell exited properly!");
        exit(EXIT_SUCCESS);
    }

    printf("Type 'q' to stop the server\n");
    char c;
    while (c != 'q') {
        c = getchar();
    }

    write(fp[1], "q", 2);
    write(sp[1], "q", 2); 
    close(fp[0]);
    close(sp[0]);
    close(fp[1]);
    close(sp[1]);

    int f_status;
    int s_status;
    waitpid(f_pid, &f_status, 0);
    if (f_status == 1) {
        c_log(ERR, "file server process exited with an error.");
    }

    waitpid(s_pid, &s_status, 0);
    if (s_status == 1) {
        c_log(ERR, "file server process exited with an error.");
    }

    free(state);
    return 0;
}

#endif // SERVER
