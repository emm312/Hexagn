#ifndef LINKER_H
#define LINKER_H

#include <vector>

#include <compiler/ast/ast.h>
#include <compiler/token.h>

struct Function
{
	const Token name;
	const Type returnType;
	std::vector<Token> argTypes;
	std::string code;

	const std::string getSignature() const;
};

class Linker
{
private:
	std::vector<Function> linkerFunctions;

public:
	void addFunction(const Function& function);
	const Function getFunction(const std::string& src, const Token& name, const std::vector<Token>& argTypes) const;
	const std::vector<Function>& getFunctions() const;

};

#endif // LINKER_H
