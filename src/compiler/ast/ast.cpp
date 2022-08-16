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

/// NODES CONSTRUCTOR IMPL

// pls save me this was pain
BiOpNode::BiOpNode(const Node& lhs, const Operation& op, const Node& rhs)
{
	this->lhs = lhs;
	this->op = op;
	this->rhs = rhs;
}

NumberNode::NumberNode(const size_t& val)
{
	this->val = val;
}

TypeNode::TypeNode(const std::string& val, const bool& ptr)
{
	this->val = val;
	this->isPointer = ptr;
}

IdentifierNode::IdentifierNode(const std::string& name)
{
	this->name = name;
}

StringNode::StringNode(const std::string& val)
{
	this->val = val;
}

WhileNode::WhileNode(const Node& cond, const Program& body)
{
	this->condition = cond;
	this->body = body;
}

IfNode::IfNode(const Node& cond, const Program& body)
{
	this->condition = cond;
	this->body = body;
}

VarDefineNode::VarDefineNode(const TypeNode& type, const IdentifierNode& ident, const Node& expr)
{
	this->type = type;
	this->ident = ident;
	this->expr = expr;
}

FunctionNode::FunctionNode(const TypeNode& retType, const IdentifierNode& ident)
{
	this->retType = retType;
	this->name = ident;
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
	const Token& tok = current();
	if (tok.m_type != type)
	{
		std::cout << std::to_string(tok.m_type) << '\n';
		std::cerr << "Error: " << errorMsg << " at line " << tok.m_lineno << '\n';
		std::cerr << tok.m_lineno << ": " << getSourceLine(glob_src, tok.m_lineno);
		drawArrows(tok.m_start, tok.m_end, tok.m_lineno);
		exit(-1);
	}

	advance();
}

bool operator ==(const Token& lhs, const Token& rhs)
{
	return lhs.m_type == rhs.m_type && lhs.m_val == rhs.m_val;
}

Node expressionParser(TokenBuffer& buf)
{
	// std::function go brr
	std::function<Node(TokenBuffer&)> expr;
	const std::function<Node(TokenBuffer&)> factor = [&expr](TokenBuffer& buf) -> Node
	{
		const Token tok = buf.current();
		if (tok.m_type == TokenType::TT_NUM)
		{
			buf.consume(TokenType::TT_NUM, "");
			return NumberNode { std::stoull(tok.m_val) };
		}
		else if (tok.m_type == TokenType::TT_OPEN_PAREN)
		{
			buf.consume(TokenType::TT_OPEN_PAREN, "");
			Node node = expr(buf);
			buf.consume(TokenType::TT_CLOSE_PAREN,
				"Expected closing ')' for expression"
			);
			return node;
		}
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

	const std::function<Node(TokenBuffer&)> term = [&factor, &tokToOperation](TokenBuffer& buf) -> Node
	{
		Node node = factor(buf);

		while (buf.current().m_type == TokenType::TT_MULT || buf.current().m_type == TokenType::TT_DIV)
		{
			const Token op = buf.current();
			if (op.m_type == TokenType::TT_MULT)
				buf.consume(TokenType::TT_MULT, "Expected * or /");
			else if (op.m_type == TokenType::TT_DIV)
				buf.consume(TokenType::TT_DIV, "Expected * or /");
			
			node = BiOpNode { node, tokToOperation(op), factor(buf) };
		}

		return node;
	};

	expr = [&term, &tokToOperation](TokenBuffer& buf) -> Node
	{
		Node node = term(buf);
		while (buf.current().m_type == TokenType::TT_PLUS || buf.current().m_type == TokenType::TT_MINUS)
		{
			const Token op = buf.current();
			if (op.m_type == TokenType::TT_PLUS)
			{
				buf.consume(TokenType::TT_PLUS, "Expected1 + or -");
			}
			if (op.m_type == TokenType::TT_MINUS)
			{
				std::cerr << std::to_string(op.m_type) << '\n';
				buf.consume(TokenType::TT_MINUS, "Expected2 + or -");
			}
			
			node = BiOpNode { node, tokToOperation(op), term(buf) };
		}

		return node;
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

inline const bool isComparison(const Token& tok)
{
	return tok.m_type == TokenType::TT_EQ
			|| tok.m_type == TokenType::TT_NEQ
			|| tok.m_type == TokenType::TT_GT
			|| tok.m_type == TokenType::TT_GTE
			|| tok.m_type == TokenType::TT_LT
			|| tok.m_type == TokenType::TT_LTE;
}

// Global variable to keep track of if statements
size_t ifCount = 0;
// Global variable to keep track of while statements
size_t whileCount = 0;

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
				const TypeNode typeNode { type.baseType.m_val, type.isPointer };

				if (!buf.hasNext())
				{
					std::cerr << "Error: Expected identifier after keyword at line " << current.m_lineno << '\n';
					std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
					drawArrows(current.m_start, current.m_end, current.m_lineno);
					exit(-1);
				}
				Token next = buf.current();
				const Token identifier = buf.current();
				const IdentifierNode identNode { identifier.m_val };

				buf.advance();
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

					const Node expression = expressionParser(buf);
					
					prog.statements.push_back(
						VarDefineNode {
							typeNode,
							identNode,
							expression
						}
					);
				}

				// Function definition
				else if (next.m_type == TokenType::TT_OPEN_PAREN)
				{
					FunctionNode func { typeNode, identNode };

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
						if (!buf.hasNext() || buf.current().m_type != TokenType::TT_IDENTIFIER)
						{
							std::cerr << "Error: Expected identifier after keyword at line " << next.m_lineno << '\n';
							std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
							drawArrows(next.m_start, next.m_end, next.m_lineno);
							exit(-1);
						}
						next = buf.current();
						const Token& identifier = buf.current();

						const FunctionNode::Argument arg { TypeNode { _type.m_val, false }, IdentifierNode { identifier.m_val } };

						func.args.push_back(arg);

						buf.advance();
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
					// Parse function body
					if (!buf.hasNext() || buf.current().m_type != TokenType::TT_OPEN_BRACE)
					{
						std::cerr << "Error: Expected '{' at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}
					next = buf.current();

					buf.advance();
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

					if (!buf.hasNext() || buf.current().m_type != TokenType::TT_CLOSE_BRACE)
					{
						const Token& tok = body[body.size() - 1];
						std::cerr << "Error: Expected closing '}' after function body at line " << tok.m_lineno << '\n';
						std::cerr << tok.m_lineno << ": " << getSourceLine(glob_src, tok.m_lineno);
						drawArrows(tok.m_start, tok.m_end, tok.m_lineno);
						exit(-1);
					}

					func.body = makeAst(body);

					prog.functions.push_back(func);
				}

				break;
			}

			case TokenType::TT_SEMICOLON: break;

			default:
			{
				std::cerr << "Unexpected token at line " << current.m_lineno << '\n';
				std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
				drawArrows(current.m_start, current.m_end, current.m_lineno);
				exit(-1);
			}
		}

		buf.advance();
	}

	return prog;
}
