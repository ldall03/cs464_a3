#include "utils.c"

struct shell_state {
    int fport;
    int sport;
    int max_thread;
    int delay;
    int verbose;
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
    s->max_thread = 5;
    s->delay = 0;
    s->verbose = 0;
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

int main(int argc, char** argv) {
    
}
