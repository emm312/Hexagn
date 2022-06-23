#ifndef LINKER_H
#define LINKER_H

#include <vector>

#include <compiler/token.h>
#include <compiler/parser.h>

class Linker
{
private:
	std::vector<Function> linkerFunctions;

public:
	void addFunction(const Function& function);
	const Function getFunction(const Token& name, const std::vector<Token>& argTypes) const;
	const std::vector<Function>& getFunctions() const;

};

#endif // LINKER_H
