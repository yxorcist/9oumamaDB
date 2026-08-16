#include <stdio.h>
#include <string.h>

#include "db.h"

int main() {

  DB *db = db_create();

  int key, value;
  int a, b;
  char cmd[32];

  while (1) {
    printf("🗑️ ");
    scanf("%31s", cmd);

    if (strcmp(cmd, "INSERT") == 0) {

      scanf("%d %d", &key, &value);
      db_insert(db, key, value);
    }

    else if (strcmp(cmd, "GET") == 0) {

      scanf("%d", &key);
      int found;
      int v = db_get(db, key, &found);

      if (found)
        printf("%d\n", v);
      else
        printf("NOT_FOUND\n");
    }

    else if (strcmp(cmd, "DELETE") == 0) {
      scanf("%d", &key);
      db_delete(db, key);
    }

    else if (strcmp(cmd, "RANGE") == 0) {
      scanf("%d %d", &a, &b);
      db_range(db, a, b);
    }

    else if (strcmp(cmd, "EXIT") == 0) {
      break;
    }
  }

  db_free(db);

  return 0;
}
