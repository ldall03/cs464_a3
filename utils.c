#ifndef UTILS
#define UTILS

#include <bits/types/stack_t.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int GL_VERBOSE = 0;
int GL_DEBUG = 0;

#define TOGGLE_VERBOSE (GL_VERBOSE = 1)
#define TOGGLE_DEBUG (GL_DEBUG = 1)

enum Level {
    VERBOSE,
    VERBOSE_INLINE,
    INLINE,
    DEBUG,
    ERR
};

// void TOGGLE_VERBOSE() 
// {
//     GL_VERBOSE = 1;
// }

// void TOGGLE_DEBUG()
// {
//     GL_DEBUG = 1;
// }

void DEBUG_SLEEP(int n)
{
    if (GL_DEBUG)
        sleep(n);
}

void c_log(enum Level t, char* msg)
{
    if (t == VERBOSE_INLINE && GL_VERBOSE) {
        printf("%s", msg);
    } else if (t == VERBOSE && GL_VERBOSE) {
        printf("%s\n", msg);
    } else if (t == DEBUG && GL_DEBUG) {
        printf("%s\n", msg);
    } else if (t == ERR) {
        printf("[ERROR]: %s\n", msg);
    }
}

void c_log_thread(int id, char* msg, char* value)
{
    if (GL_VERBOSE) {
        printf("    [THREAD %d] %s %s\n", id, msg, value);
    }
}

char** split(char* line)
{
    const int buffsize = 64;
    int position = 0;
    char** tokarr = malloc(sizeof(char*) * buffsize);
    char* token;
    char* delimeters = " \t\r\n\a";

    if (!tokarr) {
        c_log(ERR, "malloc error.");
        return NULL;
    }

    token = strtok(line, delimeters);
    while (token != NULL) {
        tokarr[position++] = token;
        token = strtok(NULL, delimeters);
    }

    tokarr[position] = NULL;
    return tokarr;
}

int str_eq(char* s1, char* s2)
{
    if (strcmp(s1, s2) == 0)
        return 1;
    return 0;
}

struct stack
{
    char** array;
    unsigned int capacity;
    int top;
};

struct stack* get_stack(unsigned int capacity)
{
    struct stack* s = malloc(sizeof(struct stack));
    s->array = malloc(capacity * sizeof(char*));
    s->capacity = capacity;
    s->top = -1;
    return s;
}

void stack_resize(struct stack* s)
{
    char** new_array = malloc(2 * s->capacity * sizeof(char*));
    memcpy(new_array, s->array, s->capacity * sizeof(char*));
    free(s->array);
    s->array = new_array;
    s->capacity *= 2;
}

int stack_full(struct stack* s)
{
    return s->top == s->capacity - 1;
}

int stack_empty(struct stack* s)
{
    return s->top == -1;
}

void stack_push(struct stack* s, char* item) 
{
    char* str = malloc(sizeof(char) * 1024);
    memcpy(str, item, strlen(item));
    if (stack_full(s))
        stack_resize(s);

    if (s->array[++s->top])
        free(s->array[s->top]);
    s->array[s->top] = str;
}

char* stack_pop(struct stack* s)
{
    if (stack_empty(s))
        return NULL;

    return s->array[s->top--];
}

void stack_free(struct stack* s)
{
    for (int i = 0; i < s->capacity; i++) {
        if (s->array[i])
            free(s->array[i]);
    }
    free(s->array);
    free(s);
}

#endif // UTILS
