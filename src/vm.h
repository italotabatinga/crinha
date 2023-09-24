#ifndef crinha_vm_h
#define crinha_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT) // even if dynamically allocated, should have a artificial limit for security reasons

typedef struct {
  ObjClosure* closure;
  uint8_t* ip; // PERF: this causes pointer indirection access, ip could be a register variable
  Value* slots;
} CallFrame;

typedef struct {
  CallFrame frames[FRAMES_MAX];
  int frameCount;

  Value stack[STACK_MAX];  // could grow dinamically
  Value* stackTop;
  Table globals;
  Table strings;
  ObjUpvalue* openUpvalues;
  Obj* objects;
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#endif