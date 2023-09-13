#include "vm.h"

#include <stdio.h>

#include "common.h"

VM vm;  // if needed to run multiple vm, better if this is not global

void initVM() {
}

void freeVM() {
}

static InterpretResult run() {  // dispatching can be made faster with direct threaded code, jump table, computed goto
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

  for (;;) {
    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        printValue(constant);
        printf("\n");
        break;
      }
      case OP_RETURN:
        return INTERPRET_OK;
    }
  }
#undef READ_CONSTANT
#undef READ_BYTE
}

InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;  // ideally kept in a local variable to improve speed
  return run();
}