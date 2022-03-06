#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <sstream>
#include <string>

#include <compiler/token.h>

extern std::string glob_src;
extern const std::string KEYWORDS[];

class VarStack
{
private:
	struct Variable
	{
		std::string name;
		size_t stackOffset;
	};

	std::vector<Variable> m_vars;

public:
	void push(const std::string& name);
	void pop();
	const size_t getOffset(const std::string& name) const;
	const size_t getSize() const;
};

std::stringstream compile(std::vector<Token> tokens, bool isFunc = false, const VarStack& funcArgs = VarStack());

#endif // PARSER_H
