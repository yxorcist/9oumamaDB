#include <stdio.h>
#include <string.h>

#include "db.h"

#define MAX_LINE 256

static void execute_command(DB *db, char *line) {
  char cmd[32];

  if (sscanf(line, "%31s", cmd) != 1)
    return;

  if (strcmp(cmd, "INSERT") == 0) {
    int key, value;

    if (sscanf(line, "%*s %d %d", &key, &value) != 2) {
      printf("ERROR: invalid INSERT\n");
      return;
    }

    db_insert(db, key, value);
  }

  else if (strcmp(cmd, "GET") == 0) {
    int key, found;

    if (sscanf(line, "%*s %d", &key) != 1) {
      printf("ERROR: invalid GET\n");
      return;
    }

    int value = db_get(db, key, &found);

    if (found)
      printf("%d\n", value);
    else
      printf("NOT_FOUND\n");
  }

  else if (strcmp(cmd, "DELETE") == 0) {
    int key;

    if (sscanf(line, "%*s %d", &key) != 1) {
      printf("ERROR: invalid DELETE\n");
      return;
    }

    db_delete(db, key);
  }

  else if (strcmp(cmd, "RANGE") == 0) {
    int a, b;

    if (sscanf(line, "%*s %d %d", &a, &b) != 2) {
      printf("ERROR: invalid RANGE\n");
      return;
    }

    db_range(db, a, b);
  }

  else if (strcmp(cmd, "EXIT") == 0) {
    return;
  }

  else {
    printf("ERROR: unknown command\n");
  }
}

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

  while (fgets(line, sizeof(line), input)) {

    if (strcmp(line, "\n") == 0)
      continue;

    char cmd[32];

    if (sscanf(line, "%31s", cmd) != 1)
      continue;

    if (strcmp(cmd, "EXIT") == 0)
      break;

    execute_command(db, line);
  }

  if (input != stdin)
    fclose(input);

  db_save(db, "database.db");
  db_free(db);

  return 0;
}
