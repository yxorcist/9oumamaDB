#ifndef PARSER_H
#define PARSER_H

typedef enum {
  CMD_INSERT,
  CMD_GET,
  CMD_UPDATE,
  CMD_DELETE,
  CMD_RANGE,
  CMD_COUNT,
  CMD_EXIT,
  CMD_INVALID
} CommandType;

typedef struct {
  CommandType type;

  int key;
  int value;

  int a;
  int b;
} Command;

int parse_command(const char *line, Command *command);

#endif
