#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>

#include "shell.h"

#define MAX_INPUT 1024
#define MAX_ARG 100
#define MAX_PATH 100

void change_directory(char *path)
{
    if (path == NULL || strcmp(path, "~") == 0) // do "cd ~" to go back to home directory
    {
        path = getenv("HOME");
    }
    if (chdir(path) != 0)
    {
        perror("cd");
    }
}

void print_working_directory()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s\n", cwd);
    }
    else
    {
        perror("pwd");
    }
}

void set_env_var(char *name, char *value)
{
    if (setenv(name, value, 1) != 0)
    {
        perror("setenv");
    }
}

void unset_env_var(char *name)
{
    if (unsetenv(name) != 0)
    {
        perror("unsetenv");
    }
}

void get_env_var(char *input)
{
    char var_name[strlen(input)];
    strncpy(var_name, input + 1, strlen(input) - 1);
    var_name[strlen(input) - 1] = '\0';

    char *value = getenv(var_name);
    if (value != NULL)
    {
        strcpy(input, value);
    }

    else
    {
        input[0] = '\0';
    }
}

void execute_unimplemented_command(char *command, char **args, char **path_dirs, int path_count)
{
    // go search for the command in PATH directories
    int found = 0;
    for (int i = 0; i < path_count; i++)
    {
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path_dirs[i], command);
        if (access(full_path, X_OK) == 0)
        {
            found = 1;
            pid_t pid = fork();
            if (pid == 0)
            {
                execv(full_path, args);
                perror("execl");
                exit(EXIT_FAILURE);
            }
            else if (pid > 0)
            {
                wait(NULL);
            }
            else
            {
                perror("fork");
            }
            break;
        }
    }
    if (!found)
    {
        printf("Unknown command: %s\n", command);
    }
}

void split_command_into_args(char *input, char *args[])
{
    int arg_count = 0;
    char *arg_token = strtok(input, " ");
    while (arg_token != NULL)
    {
        args[arg_count++] = arg_token;
        arg_token = strtok(NULL, " ");
    }
    args[arg_count] = NULL;
}

void execute_piped_commands(char *input, char **path_dirs, int path_count)
{
    char *commands[MAX_ARG];
    int command_count = 0;

    // split the input by the pipe operator
    char *token = strtok(input, "|");
    while (token != NULL)
    {
        commands[command_count++] = token;
        token = strtok(NULL, "|");
    }

    int pipe_fds[2];
    int in_fd = 0;

    for (int i = 0; i < command_count; i++)
    {
        pipe(pipe_fds);

        pid_t pid = fork();
        if (pid == 0)
        {
            dup2(in_fd, 0); // redirect input
            if (i < command_count - 1)
            {
                dup2(pipe_fds[1], 1); // redirect output
            }
            close(pipe_fds[0]);

            // split the command into arguments
            char *args[MAX_ARG];
            split_command_into_args(commands[i], args);

            // execute the command
            execute_unimplemented_command(args[0], args, path_dirs, path_count);
            exit(EXIT_FAILURE);
        }
        else
        {
            wait(NULL);
            close(pipe_fds[1]);
            in_fd = pipe_fds[0];
        }
    }
}

void execute_command_with_redirection(char *input, char **path_dirs, int path_count)
{
    char *input_file = NULL;
    char *output_file = NULL;

    char input_copy[strlen(input) + 1];
    strcpy(input_copy, input);

    char *input_redir = strchr(input_copy, '<');
    if (input_redir != NULL)
    {
        *input_redir = '\0';
        input_file = strtok(input_redir + 1, " ");
    }

    char *output_redir = strchr(input, '>');
    if (output_redir != NULL)
    {
        *output_redir = '\0';
        output_file = strtok(output_redir + 1, " ");
    }

    if (input_file != NULL)
    {
        while (isspace((unsigned char)*input_file))
            input_file++;
        char *end = input_file + strlen(input_file) - 1;
        while (end > input_file && isspace((unsigned char)*end))
            end--;
        end[1] = '\0';
    }

    if (output_file != NULL)
    {
        while (isspace((unsigned char)*output_file))
            output_file++;
        char *end = output_file + strlen(output_file) - 1;
        while (end > output_file && isspace((unsigned char)*end))
            end--;
        end[1] = '\0';
    }

    char *args[MAX_ARG];
    split_command_into_args(input_copy, args);

    pid_t pid = fork();
    if (pid == 0)
    {
        if (input_file != NULL)
        {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in == -1)
            {
                perror("open input file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }

        if (output_file != NULL)
        {
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out == -1)
            {
                perror("open output file");
                exit(EXIT_FAILURE);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }

        execute_unimplemented_command(args[0], args, path_dirs, path_count);
        exit(EXIT_FAILURE);
    }
    else if (pid > 0)
    {
        wait(NULL);
    }
    else
    {
        perror("fork");
    }
}
