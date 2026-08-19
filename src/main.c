#include <stdio.h>

#include "db.h"
#include "parser.h"

#define MAX_LINE 256

int main(int argc, char **argv) {

  DB *db = db_create();
  db_load(db, "database.db");

  FILE *input = stdin;

  if (argc > 2) {
    fprintf(stderr, "Usage: %s [script]\n", argv[0]);
    db_free(db);
    return 1;
  }

  if (argc == 2) {
    input = fopen(argv[1], "r");

    if (!input) {
      perror("fopen");
      db_free(db);
      return 1;
    }
  }

  char line[MAX_LINE];
  Command command;

  while (fgets(line, sizeof(line), input)) {

    if (!parse_command(line, &command)) {
      printf("ERROR: invalid command\n");
      continue;
    }

    if (command.type == CMD_EXIT)
      break;

    switch (command.type) {

    case CMD_INSERT:
      db_insert(db, command.key, command.value);
      break;

    case CMD_GET: {
      int found;
      int value = db_get(db, command.key, &found);

      if (found)
        printf("%d\n", value);
      else
        printf("NOT_FOUND\n");

      break;
    }

    case CMD_DELETE:
      db_delete(db, command.key);
      break;

    case CMD_RANGE:
      db_range(db, command.a, command.b);
      break;

    default:
      break;
    }
  }

  if (input != stdin)
    fclose(input);

  db_save(db, "database.db");
  db_free(db);

  return 0;
}
