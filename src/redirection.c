/**
 * @file redirection.c
 * @brief I/O redirection support.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "redirection.h"

/**
 * @brief Extract <, > and >> operators from a command's argv.
 *
 * The operators and their file names are removed from argv. For output, the
 * first redirection wins.
 * @param cmd Command to rewrite in place.
 * @param redir Output redirection info.
 * @return 0 on success, -1 on a missing file name.
 */
int parse_redirection(Command *cmd, RedirectionInfo *redir)
{

    redir->input_file = NULL;
    redir->output_file = NULL;
    redir->output_append = 0;
    redir->has_input_redirect = 0;
    redir->has_output_redirect = 0;
    redir->saved_stdin = -1;
    redir->saved_stdout = -1;

    char *new_argv[MAX_ARG];
    int new_argc = 0;

    for (int i = 0; i < cmd->argc; i++)
    {
        if (strcmp(cmd->argv[i], "<") == 0)
        {

            if (i + 1 >= cmd->argc)
            {
                fprintf(stderr, "Expected filename after <\n");
                return -1;
            }

            if (redir->input_file)
            {
                free(redir->input_file);
            }
            redir->input_file = strdup(cmd->argv[i + 1]);
            redir->has_input_redirect = 1;
            i++;
        }
        else if (strcmp(cmd->argv[i], ">") == 0)
        {

            if (i + 1 >= cmd->argc)
            {
                fprintf(stderr, "Expected filename after >\n");
                return -1;
            }

            if (!redir->has_output_redirect)
            {
                redir->output_file = strdup(cmd->argv[i + 1]);
                redir->output_append = 0;
                redir->has_output_redirect = 1;
            }
            i++;
        }
        else if (strcmp(cmd->argv[i], ">>") == 0)
        {

            if (i + 1 >= cmd->argc)
            {
                fprintf(stderr, "Expected filename after >>\n");
                return -1;
            }

            if (!redir->has_output_redirect)
            {
                redir->output_file = strdup(cmd->argv[i + 1]);
                redir->output_append = 1;
                redir->has_output_redirect = 1;
            }
            i++;
        }
        else
        {

            new_argv[new_argc++] = cmd->argv[i];
        }
    }

    for (int i = 0; i < new_argc; i++)
    {
        if (i < cmd->argc)
        {

            if (cmd->argv[i] != new_argv[i])
            {
                free(cmd->argv[i]);
                cmd->argv[i] = strdup(new_argv[i]);
            }
        }
    }

    for (int i = new_argc; i < cmd->argc; i++)
    {
        free(cmd->argv[i]);
        cmd->argv[i] = NULL;
    }

    cmd->argc = new_argc;

    return 0;
}

/**
 * @brief Apply redirection in the shell process, saving stdin/stdout for later restore.
 * @param redir Redirection info.
 * @return 0 on success, -1 on failure (descriptors are restored).
 */
int setup_redirection(RedirectionInfo *redir)
{

    if (redir->has_input_redirect)
    {

        int test_input_fd = open(redir->input_file, O_RDONLY);

        if (test_input_fd < 0)
        {
            printf("No such file or directory\n");
            return -1;
        }
        close(test_input_fd);

        redir->saved_stdin = dup(STDIN_FILENO);
        if (redir->saved_stdin == -1)
        {
            fprintf(stderr,"dup stdin");
            return -1;
        }

        int input_fd = open(redir->input_file, O_RDONLY);
        if (input_fd == -1)
        {
            printf("No such file or directory\n");
            close(redir->saved_stdin);
            return -1;
        }

        if (dup2(input_fd, STDIN_FILENO) == -1)
        {
            fprintf(stderr,"dup2 stdin");
            close(input_fd);
            close(redir->saved_stdin);
            return -1;
        }

        close(input_fd);
    }

    if (redir->has_output_redirect)
    {

        int flags = O_WRONLY | O_CREAT;
        if (redir->output_append)
        {
            flags |= O_APPEND;
        }
        else
        {
            flags |= O_TRUNC;
        }

        int test_output_fd = open(redir->output_file, flags, 0644);
        if (test_output_fd == -1)
        {
            printf("Unable to create file for writing\n");

            if (redir->saved_stdin != -1)
            {
                dup2(redir->saved_stdin, STDIN_FILENO);
                close(redir->saved_stdin);
                redir->saved_stdin = -1;
            }
            return -1;
        }
        close(test_output_fd);

        redir->saved_stdout = dup(STDOUT_FILENO);
        if (redir->saved_stdout == -1)
        {
            fprintf(stderr,"dup stdout");
            if (redir->saved_stdin != -1)
            {
                dup2(redir->saved_stdin, STDIN_FILENO);
                close(redir->saved_stdin);
            }
            return -1;
        }

        int output_fd = open(redir->output_file, flags, 0644);
        if (output_fd == -1)
        {
            printf("Unable to create file for writing\n");
            close(redir->saved_stdout);
            if (redir->saved_stdin != -1)
            {
                dup2(redir->saved_stdin, STDIN_FILENO);
                close(redir->saved_stdin);
            }
            return -1;
        }

        if (dup2(output_fd, STDOUT_FILENO) == -1)
        {
            fprintf(stderr,"dup2 stdout");
            close(output_fd);
            close(redir->saved_stdout);
            if (redir->saved_stdin != -1)
            {
                dup2(redir->saved_stdin, STDIN_FILENO);
                close(redir->saved_stdin);
            }
            return -1;
        }

        close(output_fd);
    }

    return 0;
}

/**
 * @brief Restore stdin/stdout saved by setup_redirection().
 * @param redir Redirection info.
 */
void cleanup_redirection(const RedirectionInfo *redir)
{
    fflush(stdout);

    if (redir->has_input_redirect && redir->saved_stdin != -1)
    {
        dup2(redir->saved_stdin, STDIN_FILENO);
        close(redir->saved_stdin);
    }

    if (redir->has_output_redirect && redir->saved_stdout != -1)
    {
        dup2(redir->saved_stdout, STDOUT_FILENO);
        close(redir->saved_stdout);
    }
}

/**
 * @brief Free the file names held by a RedirectionInfo.
 * @param redir Redirection info.
 */
void free_redirection_info(RedirectionInfo *redir)
{
    if (redir->input_file)
    {
        free(redir->input_file);
        redir->input_file = NULL;
    }
    if (redir->output_file)
    {
        free(redir->output_file);
        redir->output_file = NULL;
    }
}
