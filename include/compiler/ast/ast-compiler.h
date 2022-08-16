#ifndef AST_COMPILER_H
#define AST_COMPILER_H

#include <string>

#include <compiler/ast/nodes.h>

std::string compileAst(const Program& program);

#endif // AST_COMPILER_H
