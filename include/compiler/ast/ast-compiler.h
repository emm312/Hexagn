#ifndef AST_COMPILER_H
#define AST_COMPILER_H

#include <string>
#include <optional>

#include <compiler/ast/nodes.h>
#include <compiler/linker.h>

struct Arguments
{
	const bool debugSymbols;
	const bool emitEntryPoint;
	const bool emitEnd;
};

class VarStack
{
private:
	struct Variable
	{
		const std::string name;
		size_t stackOffset;
		TypeNode* type;
	};

	std::vector<Variable> m_vars;
	size_t frameCounter;

public:
	void push(const std::string& name, TypeNode* type);
	void pop();
	void pop(size_t num);
	void startFrame();
	size_t popFrame();
	size_t getOffset(const std::string& name) const;
	std::optional<TypeNode*> getType(const std::string& name) const;
	size_t getSize() const;
};

std::string compileAst(const Program& program, const Arguments& args, Linker& linker, const VarStack& funcArgs = VarStack());

#endif // AST_COMPILER_H
