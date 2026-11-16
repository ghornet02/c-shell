/**
 * @file hop.h
 * @brief The `hop` built-in: directory navigation with ~, ., .. and - shortcuts.
 */

#ifndef HOP_H
#define HOP_H

#include "token.h"
#include <limits.h>

extern int has_previous_dir;
extern char previous_dir[PATH_MAX];

int directory_exists(const char *path);
int change_directory(const char *path);
void handle_hop_argument(const char *arg);
void execute_hop_command(Command *cmd);

#endif
