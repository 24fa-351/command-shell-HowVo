#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#define MAX_INPUT 1024

void read_path_env(char **path_dirs, int *path_count);
void change_directory(char *path);
void print_working_directory();
void set_env_var(char *name, char *value);
void unset_env_var(char *name);
void get_env_var(char **input);
void execute_unimplemented_command(char *command, char **args, char **path_dirs, int path_count);
void split_command_into_args(char *input, char *args[]);
void execute_piped_commands(char *input, char **path_dirs, int path_count);
void execute_command_with_redirection(char *input, char **path_dirs, int path_count);
void execute_command_in_background(char *input, char **path_dirs, int path_count);