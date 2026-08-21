#include <stdio.h>
#include <string.h>

#include "parser.h"

int parse_command(const char *line, Command *command) {
  char name[32];
  char extra[32];

  command->type = CMD_INVALID;

  if (sscanf(line, "%31s", name) != 1)
    return 0;

  if (strcmp(name, "INSERT") == 0) {
    if (sscanf(line, "%31s %d %d %31s", name, &command->key, &command->value,
               extra) != 3)
      return 0;
    command->type = CMD_INSERT;
    return 1;
  }

  if (strcmp(name, "GET") == 0) {
    if (sscanf(line, "%31s %d %31s", name, &command->key, extra) != 2)
      return 0;
    command->type = CMD_GET;
    return 1;
  }

  if (strcmp(name, "UPDATE") == 0) {
    if (sscanf(line, "%31s %d %d %31s", name, &command->key, &command->value,
               extra) != 3)
      return 0;
    command->type = CMD_UPDATE;
    return 1;
  }

  if (strcmp(name, "COUNT") == 0) {
    if (sscanf(line, "%31s %31s", name, extra) != 1)
      return 0;
    command->type = CMD_COUNT;
    return 1;
  }

  if (strcmp(name, "DELETE") == 0) {
    if (sscanf(line, "%31s %d %31s", name, &command->key, extra) != 2)
      return 0;
    command->type = CMD_DELETE;
    return 1;
  }

  if (strcmp(name, "RANGE") == 0) {
    if (sscanf(line, "%31s %d %d %31s", name, &command->a, &command->b,
               extra) != 3)
      return 0;
    command->type = CMD_RANGE;
    return 1;
  }

  if (strcmp(name, "EXIT") == 0) {
    if (sscanf(line, "%31s %31s", name, extra) != 1)
      return 0;
    command->type = CMD_EXIT;
    return 1;
  }

  return 0;
}
