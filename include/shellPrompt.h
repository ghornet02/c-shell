/**
 * @file shellPrompt.h
 * @brief Shell-wide state, prompt rendering and the core lexer/parser/executor entry points.
 */

#ifndef SHELL_PROMPT_H
#define SHELL_PROMPT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include "token.h"
#include "background.h"

extern char shell_home[PATH_MAX];
extern char current_dir[PATH_MAX];

void init_shell_home(void);
void update_current_dir(void);
void printShellPrompt(void);
void execute_jobs(JobList* jobs);
int is_builtin_command(const char* cmd);
int execute_builtin(Command* cmd);
void execute_pipeline(Pipeline* pipeline);
int lexer(char *input, Token *tokens);
int parse_tokens(Token *tokens, int token_count, JobList *jobs);

void execute_pipeline_background(Pipeline* pipeline);

int handle_eof_condition(void);

#endif
