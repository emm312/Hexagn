#include <compiler/ast/ast.h>
#include <compiler/ast/nodes.h>

#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <functional>
#include <ranges>
#include <math.h>

#include <util.h>
#include <compiler/linker.h>
#include <compiler/string.h>
#include <importer/importHelper.h>

/// Literally everything thats not AST

Program::~Program()
{
	for (auto& statement: statements)
		delete statement;
}

// pls save me this was pain

BiOpNode::BiOpNode(Node* lhs, const Operation& op, Node* rhs)
{
	this->nodeType = NodeType::NT_BiOpNode;
	this->lhs = lhs;
	this->op = op;
	this->rhs = rhs;
}
BiOpNode::~BiOpNode()
{
	delete lhs;
	delete rhs;
}

NumberNode::NumberNode(const size_t& val)
{
	this->nodeType = NodeType::NT_NumberNode;
	this->val = val;
}

TypeNode::TypeNode(const std::string& val, const bool& ptr)
{
	this->nodeType = NodeType::NT_TypeNode;
	this->val = val;
	this->isPointer = ptr;
}

IdentifierNode::IdentifierNode(const std::string& name)
{
	this->nodeType = NodeType::NT_IdentifierNode;
	this->name = name;
}

StringNode::StringNode(const std::string& val)
{
	this->nodeType = NodeType::NT_StringNode;
	this->val = val;
}

IfNode::IfNode(Node* cond, const Program& body)
{
	this->nodeType = NodeType::NT_IfNode;
	this->condition = cond;
	this->body = body;
}
IfNode::~IfNode()
{
	delete condition;
}

WhileNode::WhileNode(Node* cond, const Program& body)
{
	this->nodeType = NodeType::NT_WhileNode;
	this->condition = cond;
	this->body = body;
}
WhileNode::~WhileNode()
{
	delete condition;
}

VarDefineNode::VarDefineNode(TypeNode* type, IdentifierNode* ident, Node* expr)
{
	this->nodeType = NodeType::NT_VarDefineNode;
	this->type = type;
	this->ident = ident;
	this->expr = expr;
}
VarDefineNode::~VarDefineNode()
{
	delete type;
	delete ident;
	delete expr;
}

VarAssignNode::VarAssignNode(IdentifierNode* ident, Node* expr)
{
	this->nodeType = NodeType::NT_VarAssignNode;
	this->ident = ident;
	this->expr = expr;
}
VarAssignNode::~VarAssignNode()
{
	delete ident;
	delete expr;
}

FunctionNode::FunctionNode(TypeNode* retType, IdentifierNode* ident)
{
	this->nodeType = NodeType::NT_FunctionNode;
	this->retType = retType;
	this->name = ident;
}
FunctionNode::~FunctionNode()
{
	delete retType;
	delete name;

	for (const auto& arg: args)
	{
		delete arg.type;
		delete arg.argName;
	}

	delete body;
}

FuncCallNode::FuncCallNode(IdentifierNode* name, const std::vector<Node*>& args)
{
	this->nodeType = NodeType::NT_FuncCallNode;
	this->name = name;
	this->args = args;
}
FuncCallNode::~FuncCallNode()
{
	for (const auto& arg: args)
		delete arg;
}

ImportNode::ImportNode(std::string& library) {
	this->library = library;
	this->nodeType = NodeType::NT_ImportNode;
}

ImportNode::~ImportNode() {
	// empty for now
}

UrclCodeblockNode::UrclCodeblockNode(std::string& code) {
	this->code = code;
	this->nodeType = NodeType::NT_UrclCodeblockNode;
}

UrclCodeblockNode::~UrclCodeblockNode() {
	// empty for now
}
/// ACTUAL AST

TokenBuffer::TokenBuffer(const std::vector<Token>& tokens)
	: m_tokens(tokens)
{}

bool TokenBuffer::hasNext() const
{
	return m_index < m_tokens.size();
}

void TokenBuffer::advance()
{
	m_index++;
}

const Token& TokenBuffer::next()
{
	return m_tokens[++m_index];
}

const Token& TokenBuffer::current() const
{
	return m_tokens[m_index];
}

const size_t& TokenBuffer::pos() const
{
	return m_index;
}

void TokenBuffer::consume(const TokenType& type, const std::string& errorMsg)
{
	if (!hasNext() || current().m_type != type)
	{
		const Token& tok = hasNext() ? m_tokens[m_index] : m_tokens[m_index - 1];
		std::cerr << "Error: " << errorMsg << " at line " << tok.m_lineno << '\n';
		std::cerr << tok.m_lineno << ": " << getSourceLine(glob_src, tok.m_lineno);
		drawArrows(tok.m_start, tok.m_end, tok.m_lineno);
		exit(-1);
	}

	advance();
}

// Function declaration because make needs it
Node* expressionParser(TokenBuffer& buf);
std::vector<Node*> makeFunctionCall(TokenBuffer& buf)
{
	const Token& lparen = buf.current();

	buf.advance();
	if (!buf.hasNext())
	{
		std::cerr << "Error: Expected function argument or ')' after '(' at line " << lparen.m_lineno << '\n';
		std::cerr << lparen.m_lineno << ": " << getSourceLine(glob_src, lparen.m_lineno);
		drawArrows(lparen.m_start, lparen.m_end, lparen.m_lineno);
		exit(-1);
	}

	std::vector<Node*> args;
	while (buf.hasNext() && buf.current().m_type != TokenType::TT_CLOSE_PAREN)
	{
		args.push_back(
			expressionParser(buf)
		);

		if (buf.current().m_type == TokenType::TT_CLOSE_PAREN)
			break;

		if (!buf.hasNext() || buf.current().m_type != TokenType::TT_COMMA)
		{
			std::cerr << "Error: Expected ',' or ')' at line " << buf.current().m_lineno << '\n';
			std::cerr << buf.current().m_lineno << ": " << getSourceLine(glob_src, buf.current().m_lineno);
			drawArrows(buf.current().m_start, buf.current().m_end, buf.current().m_lineno);
			exit(-1);
		}

		buf.advance();
	}
	buf.advance();

	return args;
}

Node* expressionParser(TokenBuffer& buf)
{
	/*
		Layout of grammar:
		
		factor = num | ident | funcCall | str | (expr)
		term = factor ([*\/] factor)*
		expr = term ([+-] term)*
		comp = expr ([><(>=)(<=)(==)] expr)*
	*/

	// std::function go brr
	std::function<Node*(TokenBuffer&)> expr;
	const std::function<Node*(TokenBuffer&)> factor = [&expr](TokenBuffer& buf) -> Node*
	{
		const Token tok = buf.current();
		if (tok.m_type == TokenType::TT_NUM)
		{
			buf.advance();
			return new NumberNode { std::stoull(tok.m_val) };
		}
		else if (tok.m_type == TokenType::TT_IDENTIFIER)
		{
			buf.advance();
			if (buf.current().m_type ==  TokenType::TT_OPEN_PAREN)
			{
				const auto& args = makeFunctionCall(buf);
				return new FuncCallNode { new IdentifierNode { tok.m_val }, args };
			}
			return new IdentifierNode { tok.m_val };
		}
		else if (tok.m_type == TokenType::TT_STR)
		{
			buf.advance();
			return new StringNode { tok.m_val };
			
		}
		else if (tok.m_type == TokenType::TT_OPEN_PAREN)
		{
			buf.consume(TokenType::TT_OPEN_PAREN, "");
			Node* node = expr(buf);
			buf.consume(TokenType::TT_CLOSE_PAREN,
				"Expected closing ')' for expression"
			);
			return node;
		}
		return nullptr; // to make warning go away
	};

	const std::function<Operation(const Token& tok)> tokToOperation = [](const Token& tok) -> Operation
	{
		switch (tok.m_type)
		{
			case TokenType::TT_PLUS:
				return Operation::ADD;

			case TokenType::TT_MINUS:
				return Operation::SUB;

			case TokenType::TT_MULT:
				return Operation::MULT;

			case TokenType::TT_DIV:
				return Operation::DIV;

			
			// No MOD yet, will add later

			default:
				return (Operation) -1;
		}
	};

	const std::function<Node*(TokenBuffer&)> term = [&factor, &tokToOperation](TokenBuffer& buf) -> Node*
	{
		Node* node = factor(buf);

		while (buf.current().m_type == TokenType::TT_MULT || buf.current().m_type == TokenType::TT_DIV)
		{
			const Token op = buf.current();
			if (op.m_type == TokenType::TT_MULT)
				buf.consume(TokenType::TT_MULT, "Expected * or /");
			else if (op.m_type == TokenType::TT_DIV)
				buf.consume(TokenType::TT_DIV, "Expected * or /");
			
			node = new BiOpNode { node, tokToOperation(op), factor(buf) };
		}

		return node;
	};

	expr = [&term, &tokToOperation](TokenBuffer& buf) -> Node*
	{
		Node* node = term(buf);
		while (buf.current().m_type == TokenType::TT_PLUS || buf.current().m_type == TokenType::TT_MINUS)
		{
			const Token op = buf.current();
			if (op.m_type == TokenType::TT_PLUS)
				buf.consume(TokenType::TT_PLUS, "Expected + or -");
			if (op.m_type == TokenType::TT_MINUS)
				buf.consume(TokenType::TT_MINUS, "Expected + or -");
			
			node = new BiOpNode { node, tokToOperation(op), term(buf) };
		}

		return node;
	};
	
	const std::function<Node*(TokenBuffer&)> comp = [&expr](TokenBuffer& buf) -> Node*
	{
		Node* node = expr(buf);
	};

	return expr(buf);
}

const Type makeType(TokenBuffer& buf)
{
	Token baseType = buf.current();
	buf.advance();
	if (!buf.hasNext() || (buf.current().m_type != TokenType::TT_IDENTIFIER && buf.current().m_type != TokenType::TT_MULT))
	{
		std::cerr << "Error: Expected identifier or `*` after keyword at line " << baseType.m_lineno << '\n';
		std::cerr << baseType.m_lineno << ": " << getSourceLine(glob_src, baseType.m_lineno);
		drawArrows(baseType.m_start, baseType.m_end, baseType.m_lineno);
		exit(-1);
	}

	if (buf.current().m_type == TokenType::TT_MULT)
	{
		buf.advance();
		return { baseType, true };
	}
	else
		return { baseType, false };
}

inline const bool isDataType(const Token& tok)
{
	return tok.m_type == TokenType::TT_VOID
		|| tok.m_type == TokenType::TT_INT
		|| tok.m_type == TokenType::TT_UINT
		|| tok.m_type == TokenType::TT_FLOAT
		|| tok.m_type == TokenType::TT_STRING
		|| tok.m_type == TokenType::TT_CHARACTER;
}

const Program makeAst(const std::vector<Token>& tokens)
{
	Program prog;
	TokenBuffer buf(tokens);

	while (buf.hasNext())
	{
		const Token& current = buf.current();
		switch (current.m_type)
		{
			// Variable or function definition
			case TokenType::TT_VOID:
			case TokenType::TT_INT:
			case TokenType::TT_UINT:
			case TokenType::TT_FLOAT:
			case TokenType::TT_STRING:
			case TokenType::TT_CHARACTER:
			{
				const Type type = makeType(buf);
				TypeNode* typeNode = new TypeNode{ type.baseType.m_val, type.isPointer };
				Token next = buf.current();
				const Token identifier = buf.current();
				buf.consume(TokenType::TT_IDENTIFIER, "Expected identifier after keyword");
				IdentifierNode* identNode = new IdentifierNode{ identifier.m_val };

				if (!buf.hasNext() || (buf.current().m_type != TokenType::TT_ASSIGN && buf.current().m_type != TokenType::TT_OPEN_PAREN && buf.current().m_type != TokenType::TT_SEMICOLON))
				{
					std::cerr << "Error: Expected '=' or ';' or '(' at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}
				next = buf.current();

				// Variable definition
				if (next.m_type == TokenType::TT_ASSIGN)
				{
					if (current.m_type == TokenType::TT_VOID)
					{
						std::cerr << "Error: Cannot have void type for variable at line " + current.m_lineno << '\n';
						std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
						drawArrows(current.m_start, current.m_end, current.m_lineno);
						exit(-1);
					}

					buf.advance();
					if (!buf.hasNext())
					{
						std::cerr << "Error: Expected expression after '=' at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}

					Node* expression = expressionParser(buf);

					buf.consume(TokenType::TT_SEMICOLON, "Expected `;` after expression");
					
					prog.statements.push_back(
						new VarDefineNode {
							typeNode,
							identNode,
							expression
						}
					);
				}

				else if (next.m_type == TokenType::TT_SEMICOLON)
					prog.statements.push_back(
						new VarDefineNode {
							typeNode,
							identNode,
							nullptr
						}
					);

				// Function definition
				else if (next.m_type == TokenType::TT_OPEN_PAREN)
				{
					FunctionNode* func = new FunctionNode{ typeNode, identNode };

					buf.advance();
					if (!buf.hasNext() || !(isDataType(buf.current()) || buf.current().m_type == TokenType::TT_CLOSE_PAREN))
					{
						std::cerr << "Error: Expected keyword or ')' at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}
					next = buf.current();

					// Append parameters in loop
					while (buf.hasNext() && buf.current().m_type != TokenType::TT_CLOSE_PAREN)
					{
						const Token& _type = buf.current();
						if (!buf.hasNext() || !isDataType(_type))
						{
							std::cerr << "Error: Expected keyword or ')' at line " << _type.m_lineno << '\n';
							std::cerr << _type.m_lineno << ": " << getSourceLine(glob_src, _type.m_lineno);
							drawArrows(_type.m_start, _type.m_end, _type.m_lineno);
							exit(-1);
						}
						buf.advance();
						const Token& identifier = buf.current();
						buf.consume(TokenType::TT_IDENTIFIER, "Expected identifier after keyword");

						next = buf.current();

						const FunctionNode::Argument arg { new TypeNode { _type.m_val, false }, new IdentifierNode { identifier.m_val } };

						func->args.push_back(arg);

						if (!buf.hasNext())
						{
							std::cerr << "Error: Expected ',' or ')' at line " << next.m_lineno << '\n';
							std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
							drawArrows(next.m_start, next.m_end, next.m_lineno);
							exit(-1);
						}
						next = buf.current();

						if (next.m_type == TokenType::TT_CLOSE_PAREN)
							break;
						else if (next.m_type != TokenType::TT_COMMA)
						{
							std::cerr << "Error: Expected ',' or ')' at line " << next.m_lineno << '\n';
							std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
							drawArrows(next.m_start, next.m_end, next.m_lineno);
							exit(-1);
						}

						buf.advance();
					}

					buf.advance();
					if (!buf.hasNext() || buf.current().m_type != TokenType::TT_OPEN_BRACE)
					{
						std::cerr << "Error: Expected '{' at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}
					buf.consume(TokenType::TT_OPEN_BRACE, "Expected '{'");
					next = buf.current();
					if (!buf.hasNext())
					{
						std::cerr << "Error: Expected function body or '}' at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}

					// Get function body in loop
					std::vector<Token> body;
					size_t scope = 0;
					while (buf.hasNext())
					{
						if (buf.current().m_type == TokenType::TT_OPEN_BRACE)
							++scope;
						else if (buf.current().m_type == TokenType::TT_CLOSE_BRACE)
						{
							if (scope == 0)
								break;
							--scope;
						}

						body.push_back(buf.current());
						buf.advance();
					}

					buf.consume(TokenType::TT_CLOSE_BRACE, "Expected closing '}' after function body");

					func->body = new Program(makeAst(body));

					prog.statements.push_back(func);
				}

				break;
			}

			// Variable reassignment or function call
			case TokenType::TT_IDENTIFIER:
			{
				IdentifierNode* identNode = new IdentifierNode(current.m_val);

				buf.advance();
				if (!buf.hasNext() && !(buf.current().m_type == TokenType::TT_ASSIGN || buf.current().m_type == TokenType::TT_OPEN_PAREN))
				{
					std::cerr << "Error: Expected '=' or function call after identifier at line " << current.m_lineno << '\n';
					std::cerr << current.m_lineno << ": " << getSourceLine(glob_src,current.m_lineno);
					drawArrows(current.m_start, current.m_end, current.m_lineno);
					exit(-1);
				}
				const Token& next = buf.current();

				// Variable reassignment
				if (next.m_type == TokenType::TT_ASSIGN)
				{
					buf.advance();

					if (!buf.hasNext())
					{
						std::cerr << "Error: Expected expression after '=' at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}

					Node* expression = expressionParser(buf);

					buf.consume(TokenType::TT_SEMICOLON, "Expected `;` after expression");

					prog.statements.push_back(
						new VarAssignNode {
							identNode,
							expression
						}
					);
				}

				// Function call
				else if (next.m_type == TokenType::TT_OPEN_PAREN)
				{
					const std::vector<Node*> args = makeFunctionCall(buf);

					buf.consume(TokenType::TT_SEMICOLON, "Expected ';' after function call");

					prog.statements.push_back(
						new FuncCallNode {
							identNode,
							args
						}
					);
				}
				break;
			}

			case TokenType::TT_IF:
			{
				/*
					reference if
					if (expr) { body }

					curr-token = "if"
				*/

				buf.advance();
				buf.consume(TokenType::TT_OPEN_PAREN, "Expected '(' after if");
				Node* expr = expressionParser(buf);
				buf.consume(TokenType::TT_CLOSE_PAREN, "Expected ')' after expression");
				buf.consume(TokenType::TT_OPEN_BRACE, "Expected '{' after ')'");

				std::vector<Token> body;
				size_t scope = 0;
				while (buf.hasNext())
				{
					if (buf.current().m_type == TokenType::TT_OPEN_BRACE)
						++scope;
					else if (buf.current().m_type == TokenType::TT_CLOSE_BRACE)
					{
						if (scope == 0)
							break;
						--scope;
					}

					body.push_back(buf.current());
					buf.advance();
				}
				const Program ast = makeAst(body);

				buf.consume(TokenType::TT_CLOSE_PAREN, "Expected '}' after body");

				prog.statements.push_back(
					new IfNode {
						expr,
						ast
					}
				);

				break;
			}

			case TokenType::TT_WHILE:
			{
				/*
					Reference while
					while (expr) { body }
					curr-token = "while"
				*/
				buf.advance();
				buf.consume(TokenType::TT_OPEN_PAREN, "Expected ( afer while");
				Node* expr = expressionParser(buf);
				buf.consume(TokenType::TT_CLOSE_PAREN, "Expected ')' after expression");
				buf.consume(TokenType::TT_OPEN_BRACE, "Expected '{' after ')'");

				std::vector<Token> body;
				size_t scope = 0;
				while (buf.hasNext())
				{
					if (buf.current().m_type == TokenType::TT_OPEN_BRACE)
						++scope;
					else if (buf.current().m_type == TokenType::TT_CLOSE_BRACE)
					{
						if (scope == 0)
							break;
						--scope;
					}

					body.push_back(buf.current());
					buf.advance();
				}
				const Program ast = makeAst(body);

				buf.consume(TokenType::TT_CLOSE_BRACE, "Expected '}' after body");

				prog.statements.push_back(
					new WhileNode {
						expr,
						ast
					}
				);

				break;
			}

			case TokenType::TT_IMPORT: {
				std::string libName;

				buf.advance();
				while (buf.hasNext() && buf.current().m_type != TokenType::TT_SEMICOLON)
				{
					Token curr = buf.current();
					if (curr.m_type != TokenType::TT_IDENTIFIER && curr.m_type != TokenType::TT_URCL_BLOCK /* Since urcl literal becomes a urcl token */)
					{
						std::cerr << "Error: Expected module name after import at line " << curr.m_lineno << '\n';
						std::cerr << curr.m_lineno << ": " << getSourceLine(glob_src, curr.m_lineno);
						drawArrows(curr.m_start, curr.m_end, curr.m_lineno);
						exit(-1);
					}

					libName += curr.m_val;

					buf.advance();
					if (!buf.hasNext())
					{
						std::cerr << "Error: Expected '.' after module/submodule name at line " << current.m_lineno << '\n';
						std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
						drawArrows(current.m_start, current.m_end, current.m_lineno);
						exit(-1);
					}
					curr = buf.current();

					if (curr.m_type == TokenType::TT_SEMICOLON) break;
					else if (curr.m_type == TokenType::TT_DOT || curr.m_type == TokenType::TT_COLON)
						libName += curr.m_val;
					else
					{
						std::cerr << "Error: Expected '.' or ':' or ';' after module/submodule name at line " << curr.m_lineno << '\n';
						std::cerr << curr.m_lineno << ": " << getSourceLine(glob_src, curr.m_lineno);
						drawArrows(curr.m_start, curr.m_end, curr.m_lineno);
						exit(-1);
					}

					buf.advance();
				}
				buf.consume(TokenType::TT_SEMICOLON, "Expected ; after import.");
				prog.statements.push_back(new ImportNode { libName });
				break;
			}
			case TokenType::TT_URCL_BLOCK:
			{
				buf.advance();
				Token tok = buf.current();
				buf.consume(TokenType::TT_STR, "Expected string after URCL codeblock.");
				prog.statements.push_back(
					new UrclCodeblockNode {
						tok.m_val
					}
				);
				buf.consume(TokenType::TT_SEMICOLON, "Expected ; after URCL codeblock.");
			}

			case TokenType::TT_SEMICOLON: break;

			default:
			{
				// std::cerr << current.toString() << '\n';
				std::cerr << "Unexpected token at line " << current.m_lineno << '\n';
				std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
				drawArrows(current.m_start, current.m_end, current.m_lineno);
				exit(-1);
			}
		}
	}

	return prog;
}
