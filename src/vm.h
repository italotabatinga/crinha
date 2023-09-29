#ifndef crinha_vm_h
#define crinha_vm_h

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MIN 256
#define FRAMES_MAX 16384
#define STACK_MIN (FRAMES_MIN * UINT8_COUNT) // even if dynamically allocated, should have a artificial limit for security reasons
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT) // even if dynamically allocated, should have a artificial limit for security reasons

typedef struct {
  ObjClosure* closure;
  uint8_t* ip; // PERF: this causes pointer indirection access, ip could be a register variable
  Value* slots;
} CallFrame;

typedef struct {
  CallFrame* frames;
  int frameCount;
  int frameCapacity;

  Value* stack;
  int stackCount;
  int stackCapacity;

  Table globals;
  Table strings;
  ObjUpvalue* openUpvalues;

  size_t bytesAllocated;
  size_t nextGC;
  Obj* objects;
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
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