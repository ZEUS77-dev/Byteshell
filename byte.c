#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/wait.h>    // accesible only in linux env

#define BUFFER_SIZE 1024
#define TOKEN_DELIMITERS " \t\r\n\a"

// Core Function declarations
void shell_loop();
char* read_line();
char** split_line(char* line);
int execute_command(char** args);
int launch_process(char** args);

// Built-in command functions
int shell_cd(char** args);
int shell_pwd(char** args);
int shell_exit(char** args);
int shell_help(char** args);
int shell_history(char** args);
int shell_echo(char** args);
int shell_ls(char** args);
int shell_touch(char** args);
int shell_date(char** args);

// Array of built-in commands
char* built_in_commands[] = {
    "cd",
    "pwd",
    "exit",
    "help",
    "history",
    "echo",
    "ls",
    "touch",
    "date",
    "clear"
};

// Array of built-in command functions
int (*built_in_functions[])(char**) = {
    &shell_cd,
    &shell_pwd,
    &shell_exit,
    &shell_help,
    &shell_history,
    &shell_echo,
    &shell_ls,
    &shell_touch,
    &shell_date,
    &shell_clear
};

// Get the number of built-in commands
int num_built_in_commands() {
    return sizeof(built_in_commands) / sizeof(char*);
}

// Main shell loop
void shell_loop() {
    char* line;
    char** args;
    int status;

    do {
        printf(">$~ ");
        line = read_line();
        args = split_line(line);
        status = execute_command(args);

        free(line);
        free(args);
    } while (status);
}

// Read a line of input : used getline instead to getchar while loop ,this seemed more intuitive
char* read_line() {
    char* line = NULL;
    ssize_t buffer_size = 0;
    getline(&line, &buffer_size, stdin);
    return line;
}

// Split the line into tokens
char** split_line(char* line) {
    int buffer_size = BUFFER_SIZE;
    int position = 0;
    char** tokens = malloc(buffer_size * sizeof(char*));
    char* token;

    if (!tokens) {
        fprintf(stderr, "Byteshell : allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TOKEN_DELIMITERS);  //strtok()
    while (token != NULL) {
        tokens[position] = token;
        position++;
        if (position >= buffer_size) {
            buffer_size += BUFFER_SIZE;
            tokens = realloc(tokens, buffer_size * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "Byteshell : allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, TOKEN_DELIMITERS);
    }
    tokens[position] = NULL;
    return tokens;
}

char* command_history[BUFFER_SIZE];
int history_count = 0;

// Execute
int execute_command(char** args) {
    if (args[0] == NULL) {
        return 1;
    }

      if (strcmp(args[0], "history") != 0) {
        command_history[history_count] = strdup(args[0]);
        history_count++;
    }

    for (int i = 0; i < num_built_in_commands(); i++) {
        if (strcmp(args[0], built_in_commands[i]) == 0) {
            return (*built_in_functions[i])(args);
        }
    }

    return launch_process(args);
}

// Launch 
int launch_process(char** args) {
    pid_t pid, wpid;
    int status;
    pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("byteshell_launch");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    } else {
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

// Built-in command: cd
int shell_cd(char** args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("byteshell_cd");
        }
    }
    return 1;
}

// Built-in command: pwd
int shell_pwd(char** args) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("getcwd");
    }
    return 1;
}

// Built-in command: exit
int shell_exit(char** args) {
    return 0;
}

// Built-in command: help
int shell_help(char** args) {
    printf("--------ByteShell💲--------\n");
    printf("     Supported built-in commands:\n");
    printf("  -> cd <directory> - Change the current directory\n");
    printf("  -> pwd - Print the current working directory\n");
    printf("  -> exit - Exit the shell\n");
    printf("  -> help - Display this help message\n");
    printf("  -> history - Display the command history\n");
    printf("  -> echo - Display a line of string/text passed as the arguments.\n");
    printf("  -> ls - list all files in the dir\n");
    printf("  -> touch - create new empty file \n");
    printf("  -> date - Display date time \n");
    return 1;
}

// Built-in command: history , instead used a simple array and passing args while execute
int shell_history(char** args) {
    printf("Command History:\n");
    for (int i = 0; i < history_count; i++) {
        printf("%d. %s\n", i+1, command_history[i]);
    }
    return 1;
}

// Built-in command: echo
int shell_echo(char** args) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("%s ", args[i]);
    }
    printf("\n");
    return 1;
}

// Built-in command: ls
int shell_ls(char** args) {
    DIR *dir;
    struct dirent *entry;

    if (args[1] == NULL) {
        dir = opendir(".");
    } else {
        dir = opendir(args[1]);
    }

    if (dir == NULL) {
        perror("opendir");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    closedir(dir);
    return 1;
}

// Built-in command: touch   // builtin touch seems to not work had to use fopen
int shell_touch(char** args) {
    int i = 1;
    while (args[i] != NULL) {
        FILE* file = fopen(args[i], "w");
        if (file == NULL) {
            perror("touch");
        } else {
            fclose(file);
        }
        i++;
    }
    return 1;
}

// Built-in command: date
int shell_date(char** args) {
    time_t current_time;
    time(&current_time);
    printf("Current time: %s", ctime(&current_time));
    return 1;
}

// Built-in command: clear
int shell_clear(char** args) {
    printf("\033[2J\033[H");  // ANSI escape sequence to clear screen
    return 1;
}

// Entry point
int main() {
    shell_loop();
    return EXIT_SUCCESS;
}

/* References : 
    ACM sessions
    javatpoint.com
    https://www.dmulholl.com/lets-build/a-command-line-shell.html
    geeksforgeeks
*/
