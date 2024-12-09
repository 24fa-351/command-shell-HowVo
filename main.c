#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell.h"

#define MAX_INPUT 1024
#define MAX_ARG 100
#define MAX_PATH 100

int main()
{
    char input[MAX_INPUT];
    char *command;
    char *args[MAX_ARG];
    char *path_env;
    char *path_dirs[MAX_PATH];
    int path_count = 0;

    // read PATH environment variable and find for any command typed that is not directly implemented
    path_env = getenv("PATH");
    if (path_env != NULL)
    {
        char *path_env_copy = strdup(path_env);
        char *token = strtok(path_env_copy, ":");
        while (token != NULL)
        {
            path_dirs[path_count++] = token;
            token = strtok(NULL, ":");
        }
    }

    // main loop to keep the shell running
    while (1)
    {
        printf("xsh> ");
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
        }

        // remove the newline character from the input
        input[strcspn(input, "\n")] = 0;

        // check if the input contains a pipe operator
        if (strchr(input, '|') != NULL)
        {
            execute_piped_commands(input, path_dirs, path_count);
            continue;
        }

        if (strchr(input, '>') != NULL || strchr(input, '<') != NULL)
        {
            execute_command_with_redirection(input, path_dirs, path_count);
            continue;
        }

        // split the input into command and arguments
        int arg_count = 0;
        command = strtok(input, " ");
        args[arg_count++] = command;
        while ((args[arg_count] = strtok(NULL, " ")) != NULL)
        {
            arg_count++;
        }

        if (command == NULL)
        {
            continue;
        }

        // scan the input to see if there any $<something>, if so, replace it with the value of the environment variable
        for (int i = 0; i < arg_count; i++)
        {
            if (args[i][0] == '$')
            {
                get_env_var(args[i]);
            }
        }

        if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0)
        {
            break;
        }

        else if (strcmp(command, "cd") == 0)
        {
            change_directory(args[1]);
        }

        else if (strcmp(command, "pwd") == 0)
        {
            print_working_directory();
        }

        else if (strcmp(command, "set") == 0)
        {
            if (args[1] != NULL && args[2] != NULL)
            {
                set_env_var(args[1], args[2]);
            }
            else
            {
                fprintf(stderr, "set: missing arguments\n");
            }
        }

        else if (strcmp(command, "unset") == 0)
        {
            if (args[1] != NULL)
            {
                unset_env_var(args[1]);
            }
            else
            {
                fprintf(stderr, "unset: missing argument\n");
            }
        }

        else
        {
            execute_unimplemented_command(command, args, path_dirs, path_count);
        }
    }

    return 0;
}