#ifndef LINKER_H
#define LINKER_H

#include <string>
#include <vector>

#include <compiler/token.h>
#include <compiler/parser.h>

extern std::vector<Function> linkerFunctions;

void linkerAddFunction(const Function& function);
const Function linkerGetFunction(const Token& name, const std::vector<Token>& argTypes);

#endif // LINKER_H
