#ifndef LINKER_H
#define LINKER_H

#include <string>
#include <vector>

#include <compiler/token.h>
#include <compiler/parser.h>

void linkerAddFunction(const Function& function);
const Function linkerGetFunction(const Token& name, const std::vector<Token>& argTypes);
const std::vector<Function>& linkerGetFunctions();

#endif // LINKER_H
