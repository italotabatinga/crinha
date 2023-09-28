#ifndef crinha_compiler_h
#define crinha_compiler_h

#include "object.h"
#include "vm.h"

ObjFunction* compile(const char* source);
void markCompilerRoots();

#endif