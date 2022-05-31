#ifndef LINKER_H
#define LINKER_H

#include <string>
#include <vector>

#include <compiler/token.h>
#include <compiler/parser.h>

class Linker
{
private:
	Linker();

public:
	static std::vector<Function> functions;

	Linker(const Linker&)         = delete;
	void operator=(const Linker&) = delete;

	static void addFunction(const Function& function);
	static const Function getFunction(const Token& name, const std::vector<Token>& argTypes);

};

#endif // LINKER_H
