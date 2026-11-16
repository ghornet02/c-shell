/**
 * @file token.h
 * @brief Token, command, pipeline and job-list types shared by the lexer, parser and executor.
 */

#ifndef TOKEN_H
#define TOKEN_H

typedef enum
{
    TOKEN_name,
    TOKEN_ampersand,
    TOKEN_semicolon,
    TOKEN_input,
    TOKEN_output,
    TOKEN_output_append,
    TOKEN_pipeline,
    TOKEN_unknown,
    TOKEN_eof
} tokenType;

/**
 * @brief A single lexical token produced by the lexer.
 */
typedef struct
{
    tokenType type;
    char Cfg_word[1024];
} Token;

#define MAX_CMD 16
#define MAX_ARG 64

/**
 * @brief One simple command: its argument vector plus any redirection targets.
 */
typedef struct
{
    char *argv[MAX_ARG];
    int argc;
    char *input_file;
    char *output_file;
    int output_append;
} Command;

/**
 * @brief A sequence of commands connected with '|'.
 */
typedef struct
{
    Command cmds[MAX_CMD];
    int cmd_count;
} Pipeline;

/**
 * @brief All pipelines parsed from one input line, separated by ';' or '&'.
 */
typedef struct
{
    Pipeline pipelines[MAX_CMD];
    int count;
    int background;
} JobList;

#endif
