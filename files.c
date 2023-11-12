#ifndef FILES
#define FILES

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <libgen.h>

#include "utils.c"

struct FIO_handler
{
    pthread_mutex_t* lock;
    pthread_cond_t* cond;
    int* files;
    int* reads;
    int* writes;
};

struct FIO_handler* get_FIO_handler(pthread_mutex_t* mutex, pthread_cond_t* cond)
{
    struct FIO_handler* h = malloc(sizeof(struct FIO_handler));
    if (!h) return NULL;

    if (pthread_mutex_init(mutex, NULL) < 0) {
        return NULL;
    }

    if (pthread_cond_init(cond, NULL) < 0) {
        return NULL;
    }

    h->lock = mutex;
    h->cond = cond;

    // Each index in the array represents a fd
    // for example h->files[5] = 1 this means fd 5 is opened 
    // same for reads and writes, reads says how man threads are reading
    // writes is 1 if it is being written to by a thread
    h->files = malloc(sizeof(int) * 128);
    h->reads = malloc(sizeof(int) * 128);
    h->writes = malloc(sizeof(int) * 128);

    return h;
}

void free_FIO_handler(struct FIO_handler* h)
{
    pthread_mutex_destroy(h->lock);
    pthread_cond_destroy(h->cond);
    free(h->files);
    free(h->reads);
    free(h->writes);
    free(h);
}

void lock_handler(struct FIO_handler* h)
{
    pthread_mutex_lock(h->lock);
}

void unlock_handler(struct FIO_handler* h)
{
    pthread_mutex_unlock(h->lock);
}

void wait_handler(struct FIO_handler* h)
{
    pthread_cond_wait(h->cond, h->lock);
}

void signal_handler(struct FIO_handler* h)
{
    pthread_cond_signal(h->cond);
}

// Get the filename of an open file descriptor
int cmp_fd_to_name(int fd, char* fname)
{
    char number[8];
    char path[128] = "/proc/self/fd/";
    char buffer[256];
    sprintf(number, "%d", fd);
    strcat(path, number);
    int len = readlink(path, buffer, 128);
    buffer[len] = 0;
    char* name = basename(buffer);
    if (str_eq(name, fname))
        return 1;
    return 0;
}

// return 1 if a given file id is opened/in use
int fd_opened_true(struct FIO_handler* handler, int fd)
{
    int ret = 0;
    lock_handler(handler);
    ret = handler->files[fd];
    unlock_handler(handler);
    return ret;
}

// If file is open set fname to 0 and return its id
int open_file(struct FIO_handler* handler, char* fname)
{
    for (int i = 0; i < 128; i++) {
        if (!handler->files[i]) continue;
        if (cmp_fd_to_name(i, fname)) {
            *fname = 0;
            return i;
        }
    }

    int file = open(fname, O_RDWR|O_CREAT|O_APPEND, 0666);
    if (file < 0) {
        return -1;
    }

    lock_handler(handler);
    handler->files[file] = 1;
    unlock_handler(handler);

    return file;
}

int seek_file(struct FIO_handler* handler, int id, int bytes)
{
    if (!fd_opened_true(handler, id))
        return -2;

    lock_handler(handler);
    while (handler->reads[id] || handler->writes[id]) {
        wait_handler(handler);
    }
    handler->writes[id] = 1;
    unlock_handler(handler);
    signal_handler(handler);

    int val = lseek(id, bytes, SEEK_CUR);

    lock_handler(handler);
    handler->writes[id] = 0;
    unlock_handler(handler);
    signal_handler(handler);

    if (val < 0) {
        return -1;
    }

    return 0; // err ret only
}

int read_file(struct FIO_handler* handler, int id, int length, char* buffer)
{
    if (!fd_opened_true(handler, id))
        return -2;

    lock_handler(handler);
    while (handler->writes[id] > 0) {
        wait_handler(handler);
    }
    handler->reads[id]++;
    unlock_handler(handler);
    signal_handler(handler);

    c_log(DEBUG, "READ START");
    int count = read(id, buffer, length);
    c_log(VERBOSE, buffer);
    DEBUG_SLEEP(3);
    c_log(DEBUG, "READ END");

    lock_handler(handler);
    handler->reads[id]--;
    unlock_handler(handler);
    signal_handler(handler);

    if (count < 0) {
        return -1;
    }

    return count;
}

int write_file(struct FIO_handler* handler, int id, char** buffer)
{
    if (!fd_opened_true(handler, id))
        return -2;

    lock_handler(handler);
    while (handler->reads[id] || handler->writes[id]) {
        wait_handler(handler);
    }
    handler->writes[id] = 1;
    unlock_handler(handler);
    signal_handler(handler);

    c_log(DEBUG, "WRITE START");
    int val = 0;
    val = write(id, "\n", 1);
    while (*buffer != NULL) {
        val = write(id, *buffer, strlen(*buffer));
        val = write(id, " ", 1);
        buffer++;
    }
    DEBUG_SLEEP(6);
    c_log(DEBUG, "WRITE END");

    lock_handler(handler);
    handler->writes[id] = 0;
    unlock_handler(handler);
    signal_handler(handler);

    if (val < 0) {
        return -1;
    }

    return 0; // err ret only 
}

int close_file(struct FIO_handler* handler, int id)
{
    if (!fd_opened_true(handler, id))
        return -2;

    lock_handler(handler);
    while (handler->reads[id] || handler->writes[id]) {
        wait_handler(handler);
    }
    handler->files[id] = 0;
    unlock_handler(handler);
    close(id);
    return 0;
}

#endif // FILES
