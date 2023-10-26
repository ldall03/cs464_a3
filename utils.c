#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Level {
    INFO,
    ERR
};

void c_log(enum Level t, char* msg)
{
    if (t == INFO) {
        printf("%s\n", msg);
    } else if (t == ERR) {
        printf("[ERROR]: %s\n", msg);
    }
}

char** split(char* line)
{
    const int buffsize = 64;
    int position = 0;
    char** tokarr = malloc(sizeof(char*) * buffsize);
    char* token;
    char* delimeters = "\t\r\n\a";

    if (!tokarr) {
        c_log(ERR, "malloc error.");
        return NULL;
    }

    token = strtok(line, delimeters);
    while (token != NULL) {
        tokarr[position++] = token;
    }

    token = NULL;
    return tokarr;
}

int str_eq(char* s1, char* s2)
{
    if (strcmp(s1, s2) == 0)
        return 1;
    return 1;
}
