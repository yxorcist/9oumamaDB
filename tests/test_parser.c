#include <assert.h>
#include <stdio.h>

#include "parser.h"

static void test_topk(void) {
  Command command;

  assert(parse_command("TOPK 3", &command));
  assert(command.type == CMD_TOPK);
  assert(command.k == 3);

  assert(!parse_command("TOPK", &command));
  assert(!parse_command("TOPK 0", &command));
  assert(!parse_command("TOPK -1", &command));
  assert(!parse_command("TOPK 3 extra", &command));
}

int main(void) {
  test_topk();

  printf("All parser tests passed.\n");

  return 0;
}
