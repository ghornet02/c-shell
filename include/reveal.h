/**
 * @file reveal.h
 * @brief The `reveal` built-in: list directory contents (supports -a and -l).
 */

#ifndef REVEAL_H
#define REVEAL_H

#include "token.h"

int directory_exists(const char *path);
void execute_reveal_command(Command *cmd);

#endif
