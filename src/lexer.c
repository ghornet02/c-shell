/**
 * @file lexer.c
 * @brief Lexer that splits an input line into words and shell operators.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "token.h"
#include "shellPrompt.h"

/**
 * @brief Map an operator character to its token type.
 * @param c Operator character.
 * @return Matching token type, or TOKEN_unknown.
 */
tokenType get_token_type(char c)
{
    switch (c)
    {
    case '&':
        return TOKEN_ampersand;
    case ';':
        return TOKEN_semicolon;
    case '<':
        return TOKEN_input;
    case '>':
        return TOKEN_output;
    case '|':
        return TOKEN_pipeline;
    default:
        return TOKEN_unknown;
    }
}

/**
 * @brief Check whether a character is one of & ; < > |.
 * @param c Character to test.
 * @return Non-zero if c is an operator.
 */
int is_shell_operator(char c)
{
    return (c == '&' || c == ';' || c == '<' || c == '>' || c == '|');
}

/**
 * @brief Append a token to the token array if there is room.
 * @param tokens Token array.
 * @param token_count In/out count of tokens.
 * @param word Token text.
 * @param type Token type.
 */
void add_token(Token *tokens, int *token_count, const char *word, tokenType type)
{
    if (*token_count < 1023)
    {
        strcpy(tokens[*token_count].Cfg_word, word);
        tokens[*token_count].type = type;
        (*token_count)++;
    }
}

/**
 * @brief Tokenise an input line.
 *
 * Handles single and double quotes, the >> operator and the standalone
 * shortcuts ., .., ~ and -.
 * @param input Null-terminated input line.
 * @param tokens Output array, terminated with a TOKEN_eof entry.
 * @return Number of tokens produced.
 */
int lexer(char *input, Token *tokens)
{
    int i = 0, length = strlen(input);
    int token_count = 0;
    char current_token[1024];
    int token_pos = 0;

    while (i <= length)
    {

        char c = (i < length) ? input[i] : '\0';

        if (c == '"' || c == '\'')
        {
            char quote_char = c;
            i++;

            if (token_pos > 0)
            {
                current_token[token_pos] = '\0';
                add_token(tokens, &token_count, current_token, TOKEN_name);
                token_pos = 0;
            }

            while (i < length && input[i] != quote_char)
            {
                current_token[token_pos++] = input[i++];
            }

            if (i < length && input[i] == quote_char)
            {
                i++;
            }

            current_token[token_pos] = '\0';
            add_token(tokens, &token_count, current_token, TOKEN_name);
            token_pos = 0;
            continue;
        }

        if (isspace(c) || c == '\0')
        {
            if (token_pos > 0)
            {
                current_token[token_pos] = '\0';
                add_token(tokens, &token_count, current_token, TOKEN_name);
                token_pos = 0;
            }
            if (c == '\0')
                break;
            i++;
            continue;
        }

        if (c == '>' && i + 1 < length && input[i + 1] == '>')
        {
            if (token_pos > 0)
            {
                current_token[token_pos] = '\0';
                add_token(tokens, &token_count, current_token, TOKEN_name);
                token_pos = 0;
            }
            add_token(tokens, &token_count, ">>", TOKEN_output_append);
            i += 2;
            continue;
        }

        if (is_shell_operator(c))
        {
            if (token_pos > 0)
            {
                current_token[token_pos] = '\0';
                add_token(tokens, &token_count, current_token, TOKEN_name);
                token_pos = 0;
            }
            char op[2] = {c, '\0'};
            add_token(tokens, &token_count, op, get_token_type(c));
            i++;
            continue;
        }

        if (c == '.' && i + 1 < length && input[i + 1] == '.' && token_pos == 0 &&
            (i == 0 || isspace(input[i - 1])) &&
            (i + 2 >= length || isspace(input[i + 2]) || is_shell_operator(input[i + 2])))
        {
            add_token(tokens, &token_count, "..", TOKEN_name);
            i += 2;
            continue;
        }

        if (c == '.' && token_pos == 0 &&
            (i == 0 || isspace(input[i - 1])) &&
            (i + 1 >= length || isspace(input[i + 1]) || is_shell_operator(input[i + 1])))
        {
            add_token(tokens, &token_count, ".", TOKEN_name);
            i++;
            continue;
        }

        if (c == '~' && token_pos == 0 &&
            (i == 0 || isspace(input[i - 1])) &&
            (i + 1 >= length || isspace(input[i + 1]) || is_shell_operator(input[i + 1])))
        {
            add_token(tokens, &token_count, "~", TOKEN_name);
            i++;
            continue;
        }

        if (c == '-' && token_pos == 0 &&
            (i == 0 || isspace(input[i - 1])) &&
            (i + 1 >= length || isspace(input[i + 1]) || is_shell_operator(input[i + 1])))
        {
            add_token(tokens, &token_count, "-", TOKEN_name);
            i++;
            continue;
        }

        current_token[token_pos++] = c;
        i++;
    }

    tokens[token_count].type = TOKEN_eof;
    tokens[token_count].Cfg_word[0] = '\0';
    return token_count;
}
