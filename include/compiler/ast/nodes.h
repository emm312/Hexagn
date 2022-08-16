#ifndef NODES_H
#define NODES_H

#include <string>
#include <vector>

#include <compiler/token.h>

struct Node
{
	// Needed for a dumb reason
	virtual bool isNumberNode() const { return false; }
};

struct FunctionNode;
struct Program
{
	std::vector<Node> statements;
	std::vector<FunctionNode> functions;
};

enum class Operation
{
	ADD,
	SUB,
	MULT,
	DIV,
	MOD,
};

struct BiOpNode: Node
{
	Node lhs;
	Operation op;
	Node rhs;

	BiOpNode(const Node& lhs, const Operation& op, const Node& rhs);
};

struct NumberNode: Node
{
	size_t val;

	NumberNode(const size_t& val);
	bool isNumberNode() const { return true; }
};

struct TypeNode: Node
{
	std::string val;
	bool isPointer;

	// Why the default constructor needed?
	TypeNode() = default;
	TypeNode(const std::string& val, const bool& ptr);
};

struct IdentifierNode: Node
{
	std::string name;

	IdentifierNode() = default;
	IdentifierNode(const std::string& name);
};

struct StringNode: Node
{
	std::string val;

	StringNode(const std::string& val);
};

struct WhileNode: Node
{
	// May be NumberNode or BiOpNode
	Node condition;
	Program body;

	WhileNode(const Node& cond, const Program& body);
};

struct IfNode: Node
{
	Node condition;
	Program body;

	IfNode(const Node& cond, const Program& body);
};

struct VarDefineNode: Node
{
	TypeNode type;
	IdentifierNode ident;

	// May be NumberNode or BiOpNode
	Node expr;

	VarDefineNode(const TypeNode& type, const IdentifierNode& ident, const Node& expr);
};

struct VarAssignNode: Node
{
	IdentifierNode ident;
	Node expr;

	VarAssignNode(const IdentifierNode& ident, const Node& expr);
};

struct FunctionNode: Node
{
	struct Argument
	{
		TypeNode type;
		IdentifierNode argName;
	};

	TypeNode retType;
	IdentifierNode name;

	std::vector<Argument> args;
	Program body;

	FunctionNode(const TypeNode& retType, const IdentifierNode& ident);
};

#endif // NODES_H
