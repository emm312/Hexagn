#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <sstream>
#include <string>

#include <compiler/token.h>

extern std::string glob_src;
extern const std::string SignedIntTypes[];
extern const std::string UnsignedIntTypes[];
extern const std::string FloatTypes[];

class VarStack
{
private:
	struct Variable
	{
		const std::string name;
		size_t stackOffset;
		const Token type;
	};

	std::vector<Variable> m_vars;

public:
	void push(const std::string& name, const Token& type);
	void pop();
	const size_t getOffset(const std::string& name) const;
	const Token  getType  (const std::string& name) const;
	const size_t getSize  ()                        const;
};

struct Function
{
	const Token name;
	const Token returnType;
	std::vector<Token> argTypes;
	std::string code;

	const std::string getSignature() const;
};

bool operator ==(const Token& lhs, const Token& rhs);

std::stringstream compile(const std::vector<Token>& tokens, const bool& debugSymbols, const bool& isFunc = false, const VarStack& funcArgs = VarStack());

#endif // PARSER_H
