#include <stdio.h>
#include <stdlib.h>

#include "db.h"
#include "parser.h"

#define MAX_LINE 256

int main(int argc, char **argv) {

  DB *db = db_create();
  if (!db)
    return 1;

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

    case CMD_UPDATE:
      db_update(db, command.key, command.value);
      break;

    case CMD_DELETE:
      db_delete(db, command.key);
      break;

    case CMD_RANGE:
      db_range(db, command.a, command.b);
      break;

    case CMD_TOPK: {
      int count = db_count(db);

      if (count == 0)
        break;

      if (command.k > count)
        command.k = count;

      Entry **results = malloc(command.k * sizeof(Entry *));

      if (!results) {
        printf("ERROR: memory allocation failed\n");
        break;
      }

      int result_count = db_topk(db, command.k, results);

      for (int i = 0; i < result_count; i++)
        printf("%d %d\n", results[i]->key, results[i]->value);

      free(results);

      break;
    }

    case CMD_COUNT:
      printf("%d\n", db_count(db));
      break;

    case CMD_SAVE:
      if (!db_save(db))
        printf("ERROR: could not save database\n");
      break;

    case CMD_LOAD:
      if (!db_load(db))
        printf("ERROR: could not load database\n");
      break;

    case CMD_CLEAR:
      db_clear(db);
      break;

    case CMD_BEGIN:
      if (!db_begin(db))
        printf("ERROR: could not begin transaction\n");
      break;

    case CMD_COMMIT:
      if (!db_commit(db))
        printf("ERROR: could not commit transaction\n");
      break;

    case CMD_ROLLBACK:
      if (!db_rollback(db))
        printf("ERROR: could not rollback transaction\n");
      break;

    default:
      break;
    }
  }

  if (input != stdin)
    fclose(input);

  if (!db_save(db))
    fprintf(stderr, "ERROR: failed to save database\n");

  db_free(db);

  return 0;
}
