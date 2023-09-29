#include "vm.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"

VM vm;  // if needed to run multiple vm, better if this is not global

static void resetStack() {
  vm.stackCount = 0;
  vm.frameCount = 0;
  vm.openUpvalues = NULL;
}

static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
    ObjFunction* function = frame->closure->function;
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  resetStack();
}

static Value clockNative(int argCount, __attribute__((unused)) Value* args) {
  if (argCount > 0) {
    runtimeError("Expected 0 arguments");
    return NIL_VAL;
  }

  return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value printNative(int argCount, Value* args) {
  if (argCount != 1) {
    runtimeError("Expect 1 argument");
    return NIL_VAL;
  }

  printValue(args[0]);
  printf("\n");
  return args[0];
}

static Value firstNative(int argCount, Value* args) {
  if (argCount != 1 && !IS_TUPLE(args[0])) {
    runtimeError("Expect 1 argument of type Tuple");
    return NIL_VAL;
  }

  return AS_TUPLE(args[0])->first;
}

static Value secondNative(int argCount, Value* args) {
  if (argCount != 1 && !IS_TUPLE(args[0])) {
    runtimeError("Expect 1 argument of type Tuple");
    return NIL_VAL;
  }

  return AS_TUPLE(args[0])->second;
}

static void defineNative(const char* name, NativeFn function) {
  push(OBJ_VAL(copyString(name, (int)strlen(name))));
  push(OBJ_VAL(newNative(function)));
  tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop();
  pop();
}

void initVM() {
  vm.stack = ALLOCATE(Value, STACK_MIN);
  vm.stackCapacity = STACK_MIN;
  vm.frames = ALLOCATE(CallFrame, FRAMES_MIN);
  vm.frameCapacity = FRAMES_MIN;

  resetStack();
  initTable(&vm.globals);
  initTable(&vm.strings);

  defineNative("clock", clockNative);
  defineNative("print", printNative);
  defineNative("first", firstNative);
  defineNative("second", secondNative);
  vm.objects = NULL;
  vm.bytesAllocated = 0;
  vm.nextGC = 1024 * 1024;

  vm.grayCount = 0;
  vm.grayCapacity = 0;
  vm.grayStack = NULL;
}

void freeVM() {
  FREE_ARRAY(Value*, vm.stack, vm.stackCapacity);
  FREE_ARRAY(CallFrame*, vm.frames, vm.frameCapacity);
  freeTable(&vm.globals);
  freeTable(&vm.strings);
  freeObjects();
}

void push(Value value) {
  if (vm.stackCapacity < vm.stackCount + 1) {
    int oldCapacity = vm.stackCapacity;
    vm.stackCapacity = GROW_CAPACITY(oldCapacity);
    vm.stack = GROW_ARRAY(Value, vm.stack, oldCapacity, vm.stackCapacity);
  }

  vm.stack[vm.stackCount] = value;
  vm.stackCount++;
}

CallFrame* newFrame() {
  if (vm.frameCapacity < vm.frameCount + 1) {
    int oldCapacity = vm.frameCapacity;
    vm.frameCapacity = GROW_CAPACITY(oldCapacity);
    vm.frames = GROW_ARRAY(CallFrame, vm.frames, oldCapacity, vm.frameCapacity);
  }

  return &vm.frames[vm.frameCount++];
}

Value pop() {
  vm.stackCount--;
  return vm.stack[vm.stackCount];
}

static Value peek(int distance) {
  return vm.stack[vm.stackCount - 1 - distance];
}

static bool call(ObjClosure* closure, int argCount) {
  if (argCount != closure->function->arity) {
    runtimeError("Expected %d arguments but got %d.", closure->function->arity, argCount);
    return false;
  }


  CallFrame* frame = newFrame();
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = vm.stack + vm.stackCount - argCount - 1;
  return true;
}

static bool tailCall(ObjClosure* closure, int argCount) {
  if (argCount != closure->function->arity) {
    runtimeError("Expected %d arguments but got %d.", closure->function->arity, argCount);
    return false;
  }


  CallFrame* frame = vm.frames + vm.frameCount - 1;
  Value* dst;
  for (int i = 0; i < argCount + 1; i++) {
    dst = frame->slots + i;
    *dst = vm.stack[vm.stackCount - argCount - 1 + i];
  }
  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  return true;
}

static bool callValue(Value callee, int argCount, bool shouldTailCall) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_CLOSURE:
        if (shouldTailCall) {
          return tailCall(AS_CLOSURE(callee), argCount);
        } else {
          return call(AS_CLOSURE(callee), argCount);
        }
      case OBJ_NATIVE: {
        NativeFn native = AS_NATIVE(callee);
        Value result = native(argCount, vm.stack + vm.stackCount - argCount);
        vm.stackCount -= argCount + 1;
        push(result);
        return true;
      }
      default:
        break;
    }
  }
  runtimeError("Can only call functions and classes.");
  return false;
}

static ObjUpvalue* captureUpValue(Value* local) {
  ObjUpvalue* prevUpValue = NULL;
  ObjUpvalue* upvalue = vm.openUpvalues;
  while (upvalue != NULL && upvalue->location > local) {
    prevUpValue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }

  ObjUpvalue* createdUpvalue = newUpvalue(local);
  createdUpvalue->next = upvalue;
  if (prevUpValue == NULL) {
    vm.openUpvalues = createdUpvalue;
  } else {
    prevUpValue->next = createdUpvalue;
  }
  return createdUpvalue;
}

static void closeUpvalues(Value* last) {
  while (vm.openUpvalues != NULL && vm.openUpvalues->location >= last) {
    ObjUpvalue* upvalue = vm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.openUpvalues = upvalue->next;
  }
}

static bool isFalsey(Value value) {
  return (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate(char* a, int aLength, char* b, int bLength) {
  int length = aLength + bLength;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a, aLength);
  memcpy(chars + aLength, b, bLength);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
  pop();
  pop();
  push(OBJ_VAL(result));
}

static InterpretResult runOptimized() {  // dispatching can be made faster with direct threaded code, jump table, computed goto
  register CallFrame* frame;
  register uint8_t* ip;

#define LOAD_FRAME()                     \
  frame = &vm.frames[vm.frameCount - 1]; \
  ip = frame->ip;

#define STORE_FRAME() frame->ip = ip

#ifdef DEBUG_TRACE_EXECUTION
#define TRACE_EXECUTION()                                                 \
  printf("          ");                                                   \
  for (Value* slot = vm.stack; slot < vm.stack + vm.stackCount; slot++) { \
    printf("[");                                                          \
    printValue(*slot);                                                    \
    printf("]");                                                          \
  }                                                                       \
  printf("\n");                                                           \
  disassembleInstruction(&frame->closure->function->chunk, (int)(ip - frame->closure->function->chunk.code));

#else

#define TRACE_EXECUTION() \
  do {                    \
  } while (false)
#endif

#if COMPUTED_GOTO
  static void* dispatchTable[] = {
#define OPCODE(op) &&code_##op,
#include "opcodes.h"
#undef OPCODE
  };

#define INTERPRET_LOOP DISPATCH();
#define CASE_CODE(name) code_##name
#define DISPATCH()                                          \
  do {                                                      \
    TRACE_EXECUTION();                                      \
    goto* dispatchTable[instruction = (OpCode)READ_BYTE()]; \
  } while (false)

#else

#define INTERPRET_LOOP \
  loop:                \
  TRACE_EXECUTION();   \
  switch (instruction = (OpCode)READ_BYTE())

#define CASE_CODE(name) case OP_##name
#define DISPATCH() goto loop

#endif

#define READ_BYTE() (*ip++)
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_SHORT() \
  (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op)                      \
  do {                                                \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      frame->ip = ip;                                 \
      runtimeError("Operands must be numbers.");      \
      return INTERPRET_RUNTIME_ERROR;                 \
    }                                                 \
    int b = AS_NUMBER(pop());                         \
    int a = AS_NUMBER(pop());                         \
    push(valueType(a op b));                          \
  } while (false)

  LOAD_FRAME();
  OpCode instruction;
  INTERPRET_LOOP {
    CASE_CODE(CONSTANT) : {
      Value constant = READ_CONSTANT();
      push(constant);
      DISPATCH();
    }
    CASE_CODE(NIL) : push(NIL_VAL);
    DISPATCH();
    CASE_CODE(TRUE) : push(BOOL_VAL(true));
    DISPATCH();
    CASE_CODE(FALSE) : push(BOOL_VAL(false));
    DISPATCH();
    CASE_CODE(POP) : pop();
    DISPATCH();
    CASE_CODE(GET_LOCAL) : {
      uint8_t slot = READ_BYTE();
      push(frame->slots[slot]);
      DISPATCH();
    }
    CASE_CODE(SET_LOCAL) : {
      uint8_t slot = READ_BYTE();
      frame->slots[slot] = peek(0);
      DISPATCH();
    }
    CASE_CODE(GET_GLOBAL) : {  // PERF: looking up in hash tables is slow, how to improve?
      ObjString* name = READ_STRING();
      Value value;
      if (!tableGet(&vm.globals, name, &value)) {
        frame->ip = ip;
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      push(value);
      DISPATCH();
    }
    CASE_CODE(DEFINE_GLOBAL) : {  // PERF: global vars are lazy eval, change to compile time to improve perf
      ObjString* name = READ_STRING();
      tableSet(&vm.globals, name, peek(0));
      pop();
      DISPATCH();
    }
    CASE_CODE(SET_GLOBAL) : {
      ObjString* name = READ_STRING();
      if (tableSet(&vm.globals, name, peek(0))) {
        tableDelete(&vm.globals, name);
        frame->ip = ip;
        runtimeError("Undefined variable '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      DISPATCH();
    }
    CASE_CODE(GET_UPVALUE) : {
      uint8_t slot = READ_BYTE();
      push(*frame->closure->upvalues[slot]->location);
      DISPATCH();
    }
    CASE_CODE(SET_UPVALUE) : {
      uint8_t slot = READ_BYTE();
      *frame->closure->upvalues[slot]->location = peek(0);
      DISPATCH();
    }
    CASE_CODE(DEFINE_TUPLE) : {
      Value second = pop();
      Value first = pop();
      push(OBJ_VAL(newTuple(&first, &second)));
      DISPATCH();
    }
    CASE_CODE(BANG_EQUAL) : {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(!valuesEqual(a, b)));
      DISPATCH();
    }
    CASE_CODE(EQUAL) : {
      Value b = pop();
      Value a = pop();
      push(BOOL_VAL(valuesEqual(a, b)));
      DISPATCH();
    }
    CASE_CODE(GREATER) : BINARY_OP(BOOL_VAL, >);
    DISPATCH();
    CASE_CODE(LESS) : BINARY_OP(BOOL_VAL, <);
    DISPATCH();
    CASE_CODE(GREATER_EQUAL) : BINARY_OP(BOOL_VAL, >=);
    DISPATCH();
    CASE_CODE(LESS_EQUAL) : BINARY_OP(BOOL_VAL, <=);
    DISPATCH();
    CASE_CODE(ADD) : {
      Value p0 = peek(0);
      Value p1 = peek(1);
      if (IS_STRING(p0) && IS_STRING(p1)) {
        ObjString* b = AS_STRING(peek(0));
        ObjString* a = AS_STRING(peek(1));

        concatenate(a->chars, a->length, b->chars, b->length);
      } else if (IS_NUMBER(p0) && IS_NUMBER(p1)) {
        int b = AS_NUMBER(pop());
        int a = AS_NUMBER(pop());

        push(NUMBER_VAL(a + b));
      } else if (IS_NUMBER(p0) && IS_STRING(p1)) {
        ObjString* b = convertToString(peek(0));
        ObjString* a = AS_STRING(peek(1));

        concatenate(a->chars, a->length, b->chars, b->length);
      } else if (IS_STRING(p0) && IS_NUMBER(p1)) {
        ObjString* b = AS_STRING(peek(0));
        ObjString* a = convertToString(peek(1));

        concatenate(a->chars, a->length, b->chars, b->length);

      } else {
        frame->ip = ip;
        runtimeError("Operands must be two numbers or two strings.");
        return INTERPRET_RUNTIME_ERROR;
      }

      DISPATCH();
    }
    CASE_CODE(SUBTRACT) : BINARY_OP(NUMBER_VAL, -);
    DISPATCH();
    CASE_CODE(MULTIPLY) : BINARY_OP(NUMBER_VAL, *);
    DISPATCH();
    CASE_CODE(DIVIDE) : BINARY_OP(NUMBER_VAL, /);
    DISPATCH();
    CASE_CODE(MODULO) : BINARY_OP(NUMBER_VAL, %);
    DISPATCH();
    CASE_CODE(NOT) : push(BOOL_VAL(isFalsey(pop())));
    DISPATCH();
    CASE_CODE(NEGATE) : {
      if (!IS_NUMBER(peek(0))) {
        frame->ip = ip;
        runtimeError("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      push(NUMBER_VAL(-AS_NUMBER(pop())));
      DISPATCH();
    }
    CASE_CODE(PRINT) : {
      printValue(peek(0));
      printf("\n");
      DISPATCH();
    }
    CASE_CODE(JUMP) : {
      uint16_t offset = READ_SHORT();
      ip += offset;
      DISPATCH();
    }
    CASE_CODE(JUMP_IF_TRUE) : {
      uint16_t offset = READ_SHORT();
      if (!isFalsey(peek(0))) ip += offset;
      DISPATCH();
    }
    CASE_CODE(JUMP_IF_FALSE) : {
      uint16_t offset = READ_SHORT();
      if (isFalsey(peek(0))) ip += offset;
      DISPATCH();
    }
    CASE_CODE(LOOP) : {
      uint16_t offset = READ_SHORT();
      ip -= offset;
      DISPATCH();
    }
    CASE_CODE(CALL) : {
      int argCount = READ_BYTE();
      frame->ip = ip;
      if (!callValue(peek(argCount), argCount, false)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];
      ip = frame->ip;
      DISPATCH();
    }
    CASE_CODE(TCALL) : {
      int argCount = READ_BYTE();
      frame->ip = ip;
      if (!callValue(peek(argCount), argCount, true)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frameCount - 1];
      vm.stackCount = frame->slots + argCount + 1 - vm.stack;
      ip = frame->ip;
      DISPATCH();
    }
    CASE_CODE(CLOSURE) : {
      ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
      ObjClosure* closure = newClosure(function);
      push(OBJ_VAL(closure));
      for (int i = 0; i < closure->upvalueCount; i++) {
        uint8_t isLocal = READ_BYTE();
        uint8_t index = READ_BYTE();
        if (isLocal) {
          closure->upvalues[i] = captureUpValue(frame->slots + index);
        } else {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }
      DISPATCH();
    }
    CASE_CODE(CLOSE_UPVALUE) : {
      closeUpvalues(vm.stack + vm.stackCount - 1);
      pop();
      DISPATCH();
    }
    CASE_CODE(RETURN) : {
      Value result = pop();
      closeUpvalues(frame->slots);
      vm.frameCount--;
      if (vm.frameCount == 0) {
        pop();
        return INTERPRET_OK;
      }

      vm.stackCount = frame->slots - vm.stack;
      push(result);
      frame = &vm.frames[vm.frameCount - 1];
      ip = frame->ip;
      DISPATCH();
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_BINARY_OP
}

InterpretResult interpret(const char* source) {
  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));
  ObjClosure* closure = newClosure(function);
  pop();
  push(OBJ_VAL(closure));
  call(closure, 0);

  InterpretResult result = runOptimized();

  return result;
}
