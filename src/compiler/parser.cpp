#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <functional>
#include <ranges>
#include <math.h>

#include <compiler/parser.h>
#include <compiler/linker.h>
#include <util.h>

class TokenBuffer
{
private:
	std::vector<Token> m_tokens;
	size_t m_index = 0;

public:
	TokenBuffer(const std::vector<Token>& tokens)
		: m_tokens(tokens)
	{}

	bool hasNext() const
	{
		return m_index < m_tokens.size();
	}

	void advance()
	{
		m_index++;
	}

	const Token& next()
	{
		return m_tokens[++m_index];
	}

	const Token& current()
	{
		return m_tokens[m_index];
	}

	size_t& pos()
	{
		return m_index;
	}
};

void VarStack::push(const std::string& name, const Token& type)
{
	if (!m_vars.empty())
		m_vars.push_back( { name, m_vars[m_vars.size() - 1].stackOffset + 1, type } );
	else
		m_vars.push_back( { name, 1, type } );
}

void VarStack::pop()
{
	m_vars.pop_back();
}

const size_t VarStack::getOffset(const std::string& name) const
{
	for (const auto& var: m_vars)
		if (var.name == name)
			return var.stackOffset;
	return -1;
}

const Token VarStack::getType(const std::string& name) const
{
	for (const auto& var: m_vars)
		if (var.name == name)
			return var.type;
	return Token(-1, (TokenType) -1, "", -1, -1);
}

const size_t VarStack::getSize() const
{
	return m_vars.size();
}

struct VarStackFrame
{
	std::string val;
	std::string code;
};

const std::string Function::getSignature() const
{
	std::stringstream ss;
	ss << name.m_val << "(";
	for (const auto& arg: argTypes)
		ss << arg.m_val << ';';
	ss << ");" << returnType.m_val;
	return ss.str();
}

bool operator ==(const Token& lhs, const Token& rhs)
{
	return lhs.m_type == rhs.m_type && lhs.m_val == rhs.m_val;
}

bool isOperator(const Token& tok)
{
	switch (tok.m_type)
	{
		case TokenType::TT_PLUS:
		case TokenType::TT_MINUS:
		case TokenType::TT_MULT:
		case TokenType::TT_DIV:
			return true;

		default:
			return false;
	}
}

size_t getPriority(const Token& tok)
{
	switch (tok.m_type)
	{
		case TokenType::TT_PLUS:
		case TokenType::TT_MINUS:
			return 1;
		case TokenType::TT_MULT:
		case TokenType::TT_DIV:
		case TokenType::TT_MOD:
			return 2;
		default:
			return 0;
	}
}

std::vector<Token> infixToPostfix(const std::vector<Token>& toks)
{
	std::vector<Token> _new;
	_new.push_back(Token(toks[0].m_lineno, TokenType::TT_OPEN_PAREN, std::string('(', 1), 0, 0));
	_new.insert(_new.end(), toks.begin(), toks.end());
	_new.push_back(Token(toks[toks.size() - 1].m_lineno, TokenType::TT_CLOSE_PAREN, std::string(')', 1), 0, 0));

	std::vector<Token> postfix;
	std::stack<Token> stack;

	for (const auto& tok: _new)
	{
		if (tok.m_type == TokenType::TT_IDENTIFIER
			|| tok.m_type == TokenType::TT_NUM
			|| tok.m_type == TokenType::TT_FLOAT
			|| tok.m_type == TokenType::TT_STR
			|| tok.m_type == TokenType::TT_CHAR)
		{
			postfix.push_back(tok);
		}
		else if (tok.m_type == TokenType::TT_OPEN_PAREN)
			stack.push(tok);
		else if (tok.m_type == TokenType::TT_CLOSE_PAREN)
		{
			while (!stack.empty() && stack.top().m_type != TokenType::TT_OPEN_PAREN)
			{
				postfix.push_back(stack.top());
				stack.pop();
			}

			stack.pop();
		}
		else
		{
			if (isOperator(stack.top()))
				while (getPriority(stack.top()) >= getPriority(tok))
				{
					postfix.push_back(stack.top());
					stack.pop();
				}

			stack.push(tok);
		}
	}

	while (!stack.empty())
	{
		postfix.push_back(stack.top());
		stack.pop();
	}

	return postfix;
}

std::string getVal(const Token& tok)
{
	switch (tok.m_type)
	{
		case TokenType::TT_NUM:
		{
			return tok.m_val;
			break;
		}

		case TokenType::TT_FLOAT:
		{
			return tok.m_val + "f32";
			break;
		}

		default:
			return "";
	}
}

std::string getOpName(const Token& tok)
{
	switch (tok.m_type)
	{
		case TokenType::TT_PLUS:
			return "ADD";

		case TokenType::TT_MINUS:
			return "SUB";

		case TokenType::TT_MULT:
			return "MLT";

		case TokenType::TT_DIV:
			return "DIV";
		
		case TokenType::TT_MOD:
			return "MOD";

		default:
			return "";
	}
}

VarStackFrame parseExpr(const std::vector<Token>& toks, const VarStack& locals, const VarStack& funcArgs)
{
	if (toks.size() == 1 && toks[0].m_type != TokenType::TT_IDENTIFIER)
		return VarStackFrame{ toks[0].m_val, "" };
	else
	{
		if (toks.size() == 1)
		{
			size_t index = locals.getOffset(toks[0].m_val);
			if (index == size_t(-1))
			{
				index = funcArgs.getOffset(toks[0].m_val);

				if (index == size_t(-1))
				{
					std::cerr << "No such variable " << toks[0].m_val << " in current context at line " << toks[0].m_lineno << '\n';
					std::cerr << toks[0].m_lineno << ": " << getSourceLine(glob_src, toks[0].m_lineno);
					drawArrows(toks[0].m_start, toks[0].m_end, toks[0].m_lineno);
					exit(-1);
				}

				return { "R2", "LLOD R2 R1 " + std::to_string(index) + '\n' };
			}
			else
				return { "R2", "LLOD R2 R1 -" + std::to_string(index) + '\n' };
		}

		// Convert infix to prefix from toks vector

		// Get prefix from infix using postfix
		auto copy = toks;
		size_t l = copy.size();
		std::reverse(copy.begin(), copy.end());
		for (size_t i = 0; i < l; ++i)
			if (copy[i].m_type == TokenType::TT_OPEN_PAREN)
			{
				copy[i].m_type = TokenType::TT_CLOSE_PAREN;
				copy[i].m_val = std::string(')', 1);
			}
			else if (copy[i].m_type == TokenType::TT_CLOSE_PAREN)
			{
				copy[i].m_type = TokenType::TT_OPEN_PAREN;
				copy[i].m_val = std::string('(', 1);
			}
		std::vector<Token> prefix = infixToPostfix(copy);
		std::reverse(prefix.begin(), prefix.end());

		std::string code;
		TokenBuffer buf(prefix);
		std::stack<std::string> codeStack;
		std::queue<std::string> varsQueue;

		// I HATE THAT I HAVE TO USE STD::FUNCTION
		std::function<std::string(TokenBuffer&, size_t&)> parseOp = [&codeStack, &varsQueue, &parseOp, &locals, &funcArgs](TokenBuffer& buf, size_t& regIndex) -> std::string
		{
			std::string _return;

			const Token& tok = buf.current();
			_return += getOpName(tok) + ' ';
			_return += "R" + std::to_string(regIndex) + ' ';

			Token next = buf.next();
			if (isOperator(next))
			{
				regIndex++;
				codeStack.push(parseOp(buf, regIndex));
				_return += "R" + std::to_string(regIndex) + ' ';
			}
			else if (next.m_type == TokenType::TT_IDENTIFIER)
			{
				regIndex++;
				size_t offset = locals.getOffset(next.m_val);

				if (offset == size_t(-1))
				{
					// Variable is not in locals, but maybe in function arguments

					// If there are no function arguments then this identifier doesnt exist
					if (funcArgs.getSize() == 0)
					{
						std::cerr << "Error: No such variable " << next.m_val << " in current context at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}

					// Check for variable in function arguments
					offset = funcArgs.getOffset(next.m_val);
					if (offset == size_t(-1))
					{
						std::cerr << "Error: No such variable " << next.m_val << " in current context at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}

					varsQueue.push("LLOD R" + std::to_string(regIndex) + " R1 " + std::to_string(offset) + '\n');
					_return += "R" + std::to_string(regIndex) + ' ';
				}
				else
				{
					varsQueue.push("LLOD R" + std::to_string(regIndex) + " R1 -" + std::to_string(offset) + '\n');
					_return += "R" + std::to_string(regIndex) + ' ';
				}
			}
			else
				_return += getVal(next) + ' ';

			next = buf.next();
			if (isOperator(next))
			{
				regIndex++;
				codeStack.push(parseOp(buf, regIndex));
				_return += "R" + std::to_string(regIndex) + '\n';
			}
			else if (next.m_type == TokenType::TT_IDENTIFIER)
			{
				regIndex++;
				size_t offset = locals.getOffset(next.m_val);

				if (offset == size_t(-1))
				{
					// Variable is not in locals, but maybe in function arguments

					// If there are no function arguments then this identifier doesnt exist
					if (funcArgs.getSize() == 0)
					{
						std::cerr << "Error: No such variable " << next.m_val << " in current context at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}

					offset = funcArgs.getOffset(next.m_val);
					if (offset == size_t(-1))
					{
						std::cerr << "Error: No such variable " << next.m_val << " in current context at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}

					varsQueue.push("LLOD R" + std::to_string(regIndex) + " R1 " + std::to_string(offset) + '\n');
					_return += "R" + std::to_string(regIndex) + '\n';
				}
				else
				{
					varsQueue.push("LLOD R" + std::to_string(regIndex) + " R1 -" + std::to_string(offset) + '\n');
					_return += "R" + std::to_string(regIndex) + '\n';
				}
			}
			else
				_return += getVal(next) + '\n';

			return _return;
		};

		size_t startRegIndex = 2;
		std::string code2 = parseOp(buf, startRegIndex);

		while (!varsQueue.empty())
		{
			code += varsQueue.front();
			varsQueue.pop();
		}
		while (!codeStack.empty())
		{
			code += codeStack.top();
			codeStack.pop();
		}

		code += code2;
		return { "R2", code };
	}
}

const inline bool isDataType(const Token& tok)
{
	return tok.m_type == TokenType::TT_INT
			|| tok.m_type == TokenType::TT_UINT
			|| tok.m_type == TokenType::TT_FLOAT
			|| tok.m_type == TokenType::TT_STRING
			|| tok.m_type == TokenType::TT_CHARACTER;
}

const inline bool isIntegerDataType(const Token& tok)
{
	return tok.m_type == TokenType::TT_INT || tok.m_type == TokenType::TT_UINT;
}

const inline bool isFloatDataType(const Token& tok)
{
	return tok.m_type == TokenType::TT_FLOAT;
}
std::vector<std::string> global_variables;
std::stringstream compile(const std::vector<Token>& tokens, const bool& debugSymbols, const bool& isFunc, const VarStack& funcArgs)
{
	TokenBuffer buf(tokens);
	std::stringstream code;
	VarStack locals;

	// Headers
	if (!isFunc)
	{
		code << "BITS == 32\n";
		code << "MINHEAP 4096\n";
		code << "MINSTACK 1024\n";
		code << "MOV R1 SP\n\n\n";
	}

	while (buf.hasNext())
	{
		const Token& current = buf.current();

		switch (current.m_type)
		{
			// Variable or function definition
			case TokenType::TT_INT:
			case TokenType::TT_UINT:
			case TokenType::TT_FLOAT:
			case TokenType::TT_STRING:
			case TokenType::TT_CHARACTER:
			{
				Token next = buf.next();
				if (!buf.hasNext())
				{
					std::cerr << "Error: Expected identifier after keyword at line " << current.m_lineno << '\n';
					std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
					drawArrows(current.m_start, current.m_end, current.m_lineno);
					exit(-1);
				}
				const Token& identifier = buf.current();

				next = buf.next();
				if (!(next.m_type == TokenType::TT_ASSIGN || next.m_type == TokenType::TT_OPEN_PAREN))
				{
					std::cerr << "Error: Expected '=' or '(' at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}

				// Variable definition
				if (next.m_type == TokenType::TT_ASSIGN)
				{
					if (debugSymbols)
						code << "// " << getSourceLine(glob_src, current.m_lineno);

					buf.advance();
					if (!buf.hasNext())
					{
						std::cerr << "Error: Expected expression after '=' at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}

					std::vector<Token> expr;
					while (buf.hasNext() && buf.current().m_type != TokenType::TT_SEMICOLON)
					{
						expr.push_back(buf.current());
						buf.advance();
					}

					auto [val, _code] = parseExpr(expr, locals, funcArgs);
					code << _code;

					if (isIntegerDataType(current))
					{
						// Get the number in string current.m_val
						// For example: int32 -> 32
						uintmax_t size = 0;
						for (const char& c: current.m_val)
							if (isdigit(c))
								size = (size * 10) + (c - '0');
						size = std::pow(2, size);
						size--;
						
						std::stringstream sizeStream;
						sizeStream << "0x" << std::hex << size;

						code << "AND R2 " << val << ' ' << sizeStream.str() << '\n';
						code << "PSH R2\n\n";
					}

					else if (current.m_type == TokenType::TT_STRING)
					{
						std::stringstream signature;
						signature << ".str"
						 << '_'  << identifier.m_val << '\n';

						// Set the value of the string
						std::string string = expr[0].m_val;

						// Set DWString to the string, and add a 0 to the end
						std::stringstream DWString;
						DWString << "DW [ " << '"' << string << '"' << " 0 ]\n";

						// Combine DWString and signature
						std::stringstream string_definition;					 
						string_definition << signature.str() << DWString.str();

						// Put definition to stacc
						code << "MOV R2 " << signature.str() << '\n' << "PSH R2\n\n";

						// Add the string to the global variables list, so it can be compiled later
						global_variables.push_back(string_definition.str());
					} else if (current.m_type == TokenType::TT_CHARACTER) 
					{
						code << "IMM R2 " << (int) expr[0].m_val[0] << "\nPSH R2\n\n";
					}

					locals.push(identifier.m_val, current);
				}

				// Function definition
				else if (next.m_type == TokenType::TT_OPEN_PAREN)
				{
					Function func { identifier, current };

					next = buf.next();
					if (!buf.hasNext())
					{
						std::cerr << "Error: Expected keyword or ')' at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}

					// Append parameters in loop
					VarStack funcArgsStack;
					while (buf.hasNext() && buf.current().m_type != TokenType::TT_CLOSE_PAREN)
					{
						const Token& type = buf.current();
						if (!buf.hasNext() || !isDataType(type))
						{
							std::cerr << "Error: Expected keyword or ')' at line " << type.m_lineno << '\n';
							std::cerr << type.m_lineno << ": " << getSourceLine(glob_src, type.m_lineno);
							drawArrows(type.m_start, type.m_end, type.m_lineno);
							exit(-1);
						}

						next = buf.next();
						if (!buf.hasNext() || next.m_type != TokenType::TT_IDENTIFIER)
						{
							std::cerr << "Error: Expected identifier after keyword at line " << next.m_lineno << '\n';
							std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
							drawArrows(next.m_start, next.m_end, next.m_lineno);
							exit(-1);
						}
						const Token& identifier = buf.current();

						next = buf.next();

						func.argTypes.push_back(type);
						funcArgsStack.push(identifier.m_val, current);

						if (!buf.hasNext())
						{
							std::cerr << "Error: Expected ',' at line " << next.m_lineno << '\n';
							std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
							drawArrows(next.m_start, next.m_end, next.m_lineno);
							exit(-1);
						}
						
						if (next.m_type == TokenType::TT_CLOSE_PAREN)
							break;
						else if (next.m_type != TokenType::TT_COMMA)
						{
							std::cerr << "Error: Expected ',' at line " << next.m_lineno << '\n';
							std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
							drawArrows(next.m_start, next.m_end, next.m_lineno);
							exit(-1);
						}

						buf.advance();
					}

					next = buf.next();

					// Parse function body

					if (!buf.hasNext() || next.m_type != TokenType::TT_OPEN_BRACE)
					{
						std::cerr << "Error: Expected '{' at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						std::cerr << std::to_string(next.m_type) << '\n';
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
							--scope;
							if (scope == 0)
								break;
						}

						body.push_back(buf.current());
						buf.advance();
					}

					func.code = compile(body, debugSymbols, true, funcArgsStack).str();

					linkerAddFunction(func);
				}

				break;
			}

			// Variable reassignment or function call
			case TokenType::TT_IDENTIFIER:
			{
				if (debugSymbols)
					code << "// " << getSourceLine(glob_src, current.m_lineno);

				const Token& identifier = buf.current();
				size_t offset = locals.getOffset(identifier.m_val);
				bool isInArgs = false;

				Token next = buf.next();
				if (!buf.hasNext() && !(next.m_type == TokenType::TT_ASSIGN || next.m_type == TokenType::TT_OPEN_PAREN))
				{
					std::cerr << "Error: Expected '=' or function call at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}

				// Variable reassignment
				if (next.m_type == TokenType::TT_ASSIGN)
				{
					if (offset == size_t(-1))
					{
						offset = funcArgs.getOffset(identifier.m_val);
						isInArgs = true;

						if (offset == size_t(-1))
						{
							std::cerr << "Error: No such variable '" << identifier.m_val << "' in current context at line " << identifier.m_lineno << '\n';
							std::cerr << identifier.m_lineno << ": " << getSourceLine(glob_src, identifier.m_lineno);
							drawArrows(identifier.m_start, identifier.m_end, identifier.m_lineno);
							exit(-1);
						}
					}

					buf.advance();
					if (!buf.hasNext())
					{
						std::cerr << "Error: Expected expression after '=' at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}

					std::vector<Token> expr;
					while (buf.hasNext() && buf.current().m_type != TokenType::TT_SEMICOLON)
					{
						expr.push_back(buf.current());
						buf.advance();
					}

					auto [val, _code] = parseExpr(expr, locals, funcArgs);
					code << _code;

					if (!isInArgs)
						code << "LSTR R1 -" << offset << ' ' << val << "\n\n";
					else
						code << "LSTR R1 " << offset << ' ' << val << "\n\n";
				}

				// Function call
				else if (next.m_type == TokenType::TT_OPEN_PAREN)
				{
					bool (*isVal)(const Token&) = [](const Token& tok)
					{
						return tok.m_type == TokenType::TT_NUM || tok.m_type == TokenType::TT_FLOAT || tok.m_type == TokenType::TT_STR || tok.m_type == TokenType::TT_CHAR;
					};

					buf.advance();
					if (!buf.hasNext())
					{
						std::cerr << "Error: Expected identifier or ')' at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}

					/**
					 * @brief Append function arguments in loop
					 */
					std::vector<Token> args;
					std::vector<Token> argTypes;
					while (buf.hasNext() && buf.current().m_type != TokenType::TT_CLOSE_PAREN)
					{
						const Token& current = buf.current();
						if (!buf.hasNext() || (current.m_type != TokenType::TT_IDENTIFIER && !isVal(current)))
						{
							std::cerr << "Error: Expected identifier or value at line " << buf.current().m_lineno << '\n';
							std::cerr << buf.current().m_lineno << ": " << getSourceLine(glob_src, buf.current().m_lineno);
							drawArrows(buf.current().m_start, buf.current().m_end, buf.current().m_lineno);
							exit(-1);
						}

						if (current.m_type == TokenType::TT_IDENTIFIER && locals.getOffset(current.m_val) == size_t(-1) && funcArgs.getOffset(current.m_val) == size_t(-1))
						{
							std::cerr << "Error: No such variable '" << current.m_val << "' in current context at line " << current.m_lineno << '\n';
							std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
							drawArrows(current.m_start, current.m_end, current.m_lineno);
							exit(-1);
						}

						args.push_back(current);

						Token type = locals.getType(current.m_val);
						if (type.m_type == (TokenType) -1)
							type = funcArgs.getType(current.m_val);
						argTypes.push_back(type);

						buf.advance();

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

					// Push arguments in reverse order
					for (const auto& arg: args | std::views::reverse)
					{
						std::string val;
						if (arg.m_type == TokenType::TT_IDENTIFIER)
						{
							if (locals.getOffset(arg.m_val) != size_t(-1))
								code << "LLOD R2 R1 -" + std::to_string(locals.getOffset(arg.m_val)) << '\n';
							else if (funcArgs.getOffset(arg.m_val) != size_t(-1))
								code << "LLOD R2 R1 " + std::to_string(funcArgs.getOffset(arg.m_val)) << '\n';
							else
							{
								std::cerr << "Error: No such variable '" << arg.m_val << "' in current context at line " << arg.m_lineno << '\n';
								std::cerr << arg.m_lineno << ": " << getSourceLine(glob_src, arg.m_lineno);
								drawArrows(arg.m_start, arg.m_end, arg.m_lineno);
								exit(-1);
							}

							val = "R2";
						}
						else
							val = arg.m_val;
						
						code << "PSH " << val << '\n';
					}

					const Function& func = linkerGetFunction(current, argTypes);

					code << "CAL ." << func.getSignature() << '\n';

					// Stack cleanup
					code << "ADD SP SP " << args.size() << "\n\n";
				}

				break;
			}

			case TokenType::TT_SEMICOLON: break;

			default: break;
		}

		buf.advance();
	}

	if (!isFunc)
	{
		code << "\nCAL .main();int8\nMOV SP R1\nHLT\n\n";
		for (const auto& func: linkerFunctions)
		{
			code << "." << func.getSignature() << '\n';

			// cdecl calling convention entry
			code << "PSH R1\nMOV R1 SP\n\n";

			code << func.code;

			// cdecl calling convention exit
			code << "MOV SP R1\nPOP R1\nRET\n\n";
		}
		for (auto variable : global_variables) {
			code << variable;
		}
	}

	return code;
}
