#ifndef LINKER_H
#define LINKER_H

#include <vector>

#include <compiler/ast/nodes.h>

struct Function
{
	TypeNode* returnType;
	IdentifierNode* name;
	std::vector<TypeNode*> argTypes;
	std::string code;

	std::string getSignature() const;
};

class Linker
{
private:
	std::vector<Function> linkerFunctions;

public:
	void addFunction(const Function& function);
	Function getFunction(const std::string& src, const std::string& name, const std::vector<TypeNode*>& argTypes) const;
	const std::vector<Function>& getFunctions() const;

};

#endif // LINKER_H
