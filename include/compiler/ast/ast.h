#ifndef AST_H
#define AST_H

#include <vector>
#include <sstream>
#include <string>

#include <compiler/ast/nodes.h>
#include <compiler/token.h>

extern std::string glob_src;
extern const std::string SignedIntTypes[];
extern const std::string UnsignedIntTypes[];
extern const std::string FloatTypes[];

struct Type
{
	const Token baseType;
	const bool isPointer = false;
};

bool operator ==(const Token& lhs, const Token& rhs);

const Program makeAst(const std::vector<Token>& tokens);

#endif // AST_H
