#ifndef NODES_H
#define NODES_H

#include <string>
#include <vector>

#include <compiler/token.h>

enum class NodeType: size_t
{
	NT_BiOpNode,
	NT_NumberNode,
	NT_TypeNode,
	NT_IdentifierNode,
	NT_StringNode,
	NT_WhileNode,
	NT_IfNode,
	NT_VarDefineNode,
	NT_VarAssignNode,
	NT_FunctionNode,
	NT_FuncCallNode,
	NT_ImportNode,
};

struct Node
{
	NodeType nodeType;
	virtual ~Node() = default;
};

struct FunctionNode;
struct Program
{
	std::vector<Node*> statements;
	~Program();
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
	Node* lhs;
	Operation op;
	Node* rhs;

	BiOpNode(Node* lhs, const Operation& op, Node* rhs);
	virtual ~BiOpNode();
};

struct NumberNode: Node
{
	size_t val;

	NumberNode(const size_t& val);
	virtual ~NumberNode() = default;
};

struct TypeNode: Node
{
	std::string val;
	bool isPointer;

	TypeNode(const std::string& val, const bool& ptr);
	virtual ~TypeNode() = default;
};

struct IdentifierNode: Node
{
	std::string name;

	IdentifierNode(const std::string& name);
	virtual ~IdentifierNode() = default;
};

struct StringNode: Node
{
	std::string val;

	StringNode(const std::string& val);
	virtual ~StringNode() = default;
};

struct IfNode: Node
{
	Node* condition;
	Program body;

	IfNode(Node* cond, const Program& body);
	virtual ~IfNode();
};

struct WhileNode: Node
{
	// May be NumberNode or BiOpNode
	Node* condition;
	Program body;

	WhileNode(Node* cond, const Program& body);
	virtual ~WhileNode();
};

struct VarDefineNode: Node
{
	TypeNode* type;
	IdentifierNode* ident;

	Node* expr;

	VarDefineNode(TypeNode* type, IdentifierNode* ident, Node* expr);
	virtual ~VarDefineNode();
};

struct VarAssignNode: Node
{
	IdentifierNode* ident;
	Node* expr;

	VarAssignNode(IdentifierNode* ident, Node* expr);
	virtual ~VarAssignNode();
};

struct FunctionNode: Node
{
	struct Argument
	{
		TypeNode* type;
		IdentifierNode* argName;
	};

	TypeNode* retType;
	IdentifierNode* name;

	std::vector<Argument> args;
	Program* body;

	FunctionNode(TypeNode* retType, IdentifierNode* ident);
	virtual ~FunctionNode();
};

struct FuncCallNode: Node
{
	IdentifierNode* name;
	std::vector<Node*> args;

	FuncCallNode(IdentifierNode* name, const std::vector<Node*>& args);
	virtual ~FuncCallNode();
};

struct ImportNode: Node
{
	std::string library;

	ImportNode(std::string& library);
	virtual ~ImportNode();
};

#endif // NODES_H
