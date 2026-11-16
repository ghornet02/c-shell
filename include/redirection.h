/**
 * @file redirection.h
 * @brief Parsing and applying <, > and >> I/O redirection.
 */

#ifndef REDIRECTION_H
#define REDIRECTION_H

#include "token.h"

/**
 * @brief Redirection targets for a command and the saved descriptors needed to undo them.
 */
typedef struct
{
    char *input_file;
    char *output_file;
    int output_append;
    int has_input_redirect;
    int has_output_redirect;
    int saved_stdin;
    int saved_stdout;
} RedirectionInfo;

int parse_redirection(Command *cmd, RedirectionInfo *redir);
int setup_redirection(RedirectionInfo *redir);
void cleanup_redirection(const RedirectionInfo *redir);
void free_redirection_info(RedirectionInfo *redir);

#endif
