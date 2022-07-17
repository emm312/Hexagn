#include <compiler/parser.h>

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

	const Token& current() const
	{
		return m_tokens[m_index];
	}

	const size_t& pos() const
	{
		return m_index;
	}
};

void VarStack::push(const std::string name, const Token type)
{
	if (!m_vars.empty())
		m_vars.push_back( { name, m_vars[m_vars.size() - 1].stackOffset + 1, type } );
	else
		m_vars.push_back( { name, 1, type } );

	frameCounter++;
}

void VarStack::pop()
{
	m_vars.pop_back();
}

void VarStack::pop(size_t num)
{
	for (size_t _ = 0; _ < num; ++_) m_vars.pop_back();
}

void VarStack::startFrame()
{
	frameCounter = 0;
}

const size_t VarStack::popFrame()
{
	pop(frameCounter);
	return frameCounter;
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

				return { "R2", "LLOD R2 R1 " + std::to_string(index + 1) + '\n' };
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
		std::queue<std::string> codeStack;
		std::queue<std::string> varsQueue;

		// I HATE THAT I HAVE TO USE STD::FUNCTION
		std::function<std::string(TokenBuffer&, size_t)> parseOp = [&codeStack, &varsQueue, &parseOp, &locals, &funcArgs](TokenBuffer& buf, size_t regIndex) -> std::string
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

					varsQueue.push("LLOD R" + std::to_string(regIndex) + " R1 " + std::to_string(offset + 1) + '\n');
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

					varsQueue.push("LLOD R" + std::to_string(regIndex) + " R1 " + std::to_string(offset + 1) + '\n');
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
			code += codeStack.front();
			codeStack.pop();
		}

		code += code2;
		return { "R2", code };
	}
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

const std::string compile(Linker& linker, const std::vector<Token>& tokens, const bool& debugSymbols, const bool& emitFunctions, const bool& emitEntryPoint, const bool& isSubScope, const bool& popFrame, const VarStack& _locals, const VarStack& funcArgs)
{
	TokenBuffer buf(tokens);
	std::stringstream code;
	VarStack locals = _locals;
	locals.startFrame();

	if (emitEntryPoint)
	{
		code << "BITS == 32\n";
		code << "MINHEAP 4096\n";
		code << "MINSTACK 1024\n";
		code << "MOV R1 SP\n\n";
	}
	
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
				if (debugSymbols)
					code << "// " << getSourceLine(glob_src, current.m_lineno);

				buf.advance();
				if (!buf.hasNext())
				{
					std::cerr << "Error: Expected identifier after keyword at line " << current.m_lineno << '\n';
					std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
					drawArrows(current.m_start, current.m_end, current.m_lineno);
					exit(-1);
				}
				Token next = buf.current();
				const Token& identifier = buf.current();

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

					std::vector<Token> expr;
					while (buf.hasNext() && buf.current().m_type != TokenType::TT_SEMICOLON)
					{
						const Token& curr = buf.current();

						if (!isNumber(curr) && !isOperator(curr) && curr.m_type != TokenType::TT_IDENTIFIER)
						{
							std::cerr << "Error: Expression cannot have non-number or non-operator or non-identifer tokens at line " << curr.m_lineno << '\n';
							std::cerr << curr.m_lineno << ": " << getSourceLine(glob_src, curr.m_lineno);
							drawArrows(curr.m_start, curr.m_end, curr.m_lineno);
							exit(-1);
						}

						expr.push_back(curr);
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
						if (expr[0].m_type != TokenType::TT_STR)
						{
							std::cerr << "Error: Expected string literal at line " << expr[0].m_lineno << '\n';
							std::cerr << expr[0].m_lineno << ": " << getSourceLine(glob_src, expr[0].m_lineno);
							drawArrows(expr[0].m_start, expr[0].m_end, expr[0].m_lineno);
							exit(-1);
						}

						const std::string& string = expr[0].m_val;

						// Register the string into the string table and get the signature
						const std::string& signature = registerString(string);

						// Put definition to stacc
						code << "MOV R2 " << signature << "\nPSH R2\n\n";
					}

					else if (current.m_type == TokenType::TT_CHARACTER)
					{
						const Token& tok = expr[0];

						if (tok.m_type == TokenType::TT_CHAR)
							code << "IMM R2 " << (int) expr[0].m_val[0] << '\n';
						else if (tok.m_type == TokenType::TT_NUM)
							code << "MOD R2 " << tok.m_val << " 0xff\n";
						else
						{
							std::cerr << "Error: Expected character literal or number at line " << expr[0].m_lineno << '\n';
							std::cerr << expr[0].m_lineno << ": " << getSourceLine(glob_src, expr[0].m_lineno);
							drawArrows(expr[0].m_start, expr[0].m_end, expr[0].m_lineno);
							exit(-1);
						}

						code << "PSH R2\n\n";
					}

					locals.push(identifier.m_val, current);
				}

				// Variable declaration
				else if (next.m_type == TokenType::TT_SEMICOLON)
				{
					locals.push(identifier.m_val, current);
					code << "DEC SP SP";
				}

				// Function definition
				else if (next.m_type == TokenType::TT_OPEN_PAREN)
				{
					if (isSubScope)
					{
						std::cerr << "Error: Nested functions are not supported at line " << current.m_lineno << '\n';
						std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
						drawArrows(current.m_start, current.m_end, current.m_lineno);
						exit(-1);
					}

					Function func { identifier, current };

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

						func.argTypes.push_back(type);
						funcArgsStack.push(identifier.m_val, type);

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

					func.code = compile(linker, body, debugSymbols, false, false, false, true, VarStack(), funcArgsStack);
					linker.addFunction(func);
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

				buf.advance();
				if (!buf.hasNext() && !(buf.current().m_type == TokenType::TT_ASSIGN || buf.current().m_type == TokenType::TT_OPEN_PAREN))
				{
					std::cerr << "Error: Expected '=' or function call after identifier at line " << current.m_lineno << '\n';
					std::cerr << current.m_lineno << ": " << getSourceLine(glob_src,current.m_lineno);
					drawArrows(current.m_start, current.m_end, current.m_lineno);
					exit(-1);
				}
				Token next = buf.current();

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

						offset++;
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
						const Token& curr = buf.current();

						if (!isNumber(curr) && !isOperator(curr) && curr.m_type != TokenType::TT_IDENTIFIER)
						{
							std::cerr << "Error: Expression cannot have non-number or non-operator or non-identifer tokens at line " << curr.m_lineno << '\n';
							std::cerr << curr.m_lineno << ": " << getSourceLine(glob_src, curr.m_lineno);
							drawArrows(curr.m_start, curr.m_end, curr.m_lineno);
							exit(-1);
						}

						expr.push_back(curr);
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
					buf.advance();
					if (!buf.hasNext())
					{
						std::cerr << "Error: Expected identifier or ')' at line " << next.m_lineno << '\n';
						std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end, next.m_lineno);
						exit(-1);
					}

					// Append function arguments in loop
					std::vector<Token> args;
					std::vector<Token> argTypes;
					while (buf.hasNext() && buf.current().m_type != TokenType::TT_CLOSE_PAREN)
					{
						const Token& val = buf.current();
						if (!buf.hasNext() || (val.m_type != TokenType::TT_IDENTIFIER && val.m_type != TokenType::TT_NUM && val.m_type != TokenType::TT_STR))
						{
							std::cerr << "Error: Expected identifier or value at line " << buf.current().m_lineno << '\n';
							std::cerr << buf.current().m_lineno << ": " << getSourceLine(glob_src, buf.current().m_lineno);
							drawArrows(buf.current().m_start, buf.current().m_end, buf.current().m_lineno);
							exit(-1);
						}

						if (val.m_type == TokenType::TT_IDENTIFIER && locals.getOffset(val.m_val) == size_t(-1) && funcArgs.getOffset(val.m_val) == size_t(-1))
						{
							std::cerr << "Error: No such variable '" << val.m_val << "' in current context at line " << val.m_lineno << '\n';
							std::cerr << val.m_lineno << ": " << getSourceLine(glob_src, val.m_lineno);
							drawArrows(val.m_start, val.m_end, val.m_lineno);
							exit(-1);
						}

						args.push_back(val);
						if (val.m_type == TokenType::TT_STR)
							argTypes.push_back(Token(val.m_lineno, TokenType::TT_STRING, val.m_val, val.m_start, val.m_end));
						else if (val.m_type == TokenType::TT_NUM)
							argTypes.push_back(val);
						else
						{
							Token type = locals.getType(val.m_val);
							if (type.m_type == (TokenType) -1)
								type = funcArgs.getType(val.m_val);
							argTypes.push_back(type);
						}

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
								code << "LLOD R2 R1 " + std::to_string(funcArgs.getOffset(arg.m_val) + 1) << '\n';
							else
							{
								std::cerr << "Error: No such variable '" << arg.m_val << "' in current context at line " << arg.m_lineno << '\n';
								std::cerr << arg.m_lineno << ": " << getSourceLine(glob_src, arg.m_lineno);
								drawArrows(arg.m_start, arg.m_end, arg.m_lineno);
								exit(-1);
							}

							val = "R2";
						}
						else if (arg.m_type == TokenType::TT_STR)
							val = registerString(arg.m_val);
						else if (arg.m_type == TokenType::TT_NUM)
							val = arg.m_val;
						
						code << "PSH " << val << '\n';
					}

					const Function& func = linker.getFunction(glob_src, current, argTypes);

					code << "CAL ." << func.getSignature() << '\n';

					// Stack cleanup
					code << "ADD SP SP " << args.size() << "\n\n";
				}

				break;
			}

			case TokenType::TT_IF:
			{
				if (debugSymbols)
					code << "// " << getSourceLine(glob_src, current.m_lineno);

				ifCount++;
				// Save the current ifCount since it may be modified
				size_t currIfCount = ifCount;

				int destCounter = 2;

				buf.advance();
				if (!buf.hasNext() || buf.current().m_type != TokenType::TT_OPEN_PAREN)
				{
					std::cerr << "Error: Expected '(' after if at line " << current.m_lineno << '\n';
					std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
					drawArrows(current.m_start, current.m_end, current.m_lineno);
					exit(-1);
				}
				Token next = buf.current();
				
				buf.advance();
				if (!buf.hasNext() || !(isNumber(buf.current()) || buf.current().m_type == TokenType::TT_IDENTIFIER))
				{
					std::cerr << "Error: Expected number or identifier after ( at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}
				next = buf.current();

				if (next.m_type == TokenType::TT_IDENTIFIER)
					code << "LLOD R" << destCounter++ << " R1 " << "-" << locals.getOffset(buf.current().m_val) << '\n';
				else if (next.m_type == TokenType::TT_NUM)
					code << "IMM R" << destCounter++ << " " << buf.current().m_val << '\n';\

				buf.advance();
				if (!buf.hasNext() || !isComparison(buf.current()))
				{
					std::cerr << "Error: Expected comparison operator after identifier/number at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}
				next = buf.current();

				std::string instruction;

				switch (next.m_type)
				{
					case TokenType::TT_EQ:  instruction = "BRE"; break;
					case TokenType::TT_NEQ: instruction = "BNE"; break;
					case TokenType::TT_GT:  instruction = "BRG"; break;
					case TokenType::TT_GTE: instruction = "BGE"; break;
					case TokenType::TT_LT:  instruction = "BRL"; break;
					case TokenType::TT_LTE: instruction = "BLE"; break;

					default: break;
				}
				
				buf.advance();
				if (!buf.hasNext() || !(isNumber(buf.current()) || buf.current().m_type == TokenType::TT_IDENTIFIER))
				{
					std::cerr << "Error: Expected number or identifier after comparison at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}
				next = buf.current();

				if (next.m_type == TokenType::TT_NUM)
					code << "IMM R" << destCounter++ << " " << next.m_val << '\n';
				else if (next.m_type == TokenType::TT_IDENTIFIER)
					code << "LLOD R" << destCounter++ << " R1 " << "-" << locals.getOffset(next.m_val) << '\n';

				code << instruction << " " << ".if" << currIfCount << " R" << destCounter-2 << " R" << destCounter-1 << "\n";
				code << "JMP .endif"<< currIfCount << '\n';
				code << ".if"<< currIfCount << '\n';

				buf.advance();
				if (!buf.hasNext() || buf.current().m_type != TokenType::TT_CLOSE_PAREN)
				{
					std::cerr << "Error: Expected ')' after condition at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}
				next = buf.current();
				
				buf.advance();
				if (!buf.hasNext() || buf.current().m_type != TokenType::TT_OPEN_BRACE)
				{
					std::cerr << "Error: Expecter '{' after closing ')' at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}
				next = buf.current();
				buf.advance();

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

				const std::string& outcode = compile(linker, body, debugSymbols, false, false, true, true, locals, funcArgs);
				code << outcode;
				code << ".endif" << currIfCount << '\n';

				break;
			}

			case TokenType::TT_WHILE:
			{
				if (debugSymbols)
					code << "// " << getSourceLine(glob_src, current.m_lineno);

				whileCount++;
				size_t currWhileCount = whileCount;

				int destCounter = 2;

				code << ".while"<< currWhileCount << '\n';

				buf.advance();
				if (!buf.hasNext() || buf.current().m_type != TokenType::TT_OPEN_PAREN)
				{
					std::cerr << "Error: Expected '(' after while at line " << current.m_lineno << '\n';
					std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
					drawArrows(current.m_start, current.m_end, current.m_lineno);
					exit(-1);
				}
				Token next = buf.current();
				
				buf.advance();
				if (!buf.hasNext() || !(isNumber(buf.current()) || buf.current().m_type == TokenType::TT_IDENTIFIER))
				{
					std::cerr << "Error: Expected number or identifier after ( at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}
				next = buf.current();

				if (next.m_type == TokenType::TT_IDENTIFIER)
					code << "LLOD R" << destCounter++ << " R1 " << "-" << locals.getOffset(buf.current().m_val) << '\n';
				else if (next.m_type == TokenType::TT_NUM)
					code << "IMM R" << destCounter++ << " " << buf.current().m_val << '\n';\

				buf.advance();
				if (!buf.hasNext() || !isComparison(buf.current()))
				{
					std::cerr << "Error: Expected comparison operator after identifier/number at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}
				next = buf.current();

				std::string instruction;

				switch (next.m_type)
				{
					case TokenType::TT_EQ:  instruction = "BNE"; break;
					case TokenType::TT_NEQ: instruction = "BRE"; break;
					case TokenType::TT_GT:  instruction = "BLE"; break;
					case TokenType::TT_GTE: instruction = "BRL"; break;
					case TokenType::TT_LT:  instruction = "BGE"; break;
					case TokenType::TT_LTE: instruction = "BRG"; break;

					default: break;
				}
				
				buf.advance();
				if (!buf.hasNext() || !(isNumber(buf.current()) || buf.current().m_type == TokenType::TT_IDENTIFIER))
				{
					std::cerr << "Error: Expected number or identifier after comparison at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}
				next = buf.current();

				if (next.m_type == TokenType::TT_NUM)
					code << "IMM R" << destCounter++ << " " << next.m_val << '\n';
				else if (next.m_type == TokenType::TT_IDENTIFIER)
					code << "LLOD R" << destCounter++ << " R1 " << "-" << locals.getOffset(next.m_val) << '\n';

				code << instruction << " " << ".endwhile" << currWhileCount << " R" << destCounter-2 << " R" << destCounter-1 << "\n";

				buf.advance();
				if (!buf.hasNext() || buf.current().m_type != TokenType::TT_CLOSE_PAREN)
				{
					std::cerr << "Error: Expected ')' after condition at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}
				next = buf.current();
				
				buf.advance();
				if (!buf.hasNext() || buf.current().m_type != TokenType::TT_OPEN_BRACE)
				{
					std::cerr << "Error: Expecter '{' after closing ')' at line " << next.m_lineno << '\n';
					std::cerr << next.m_lineno << ": " << getSourceLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end, next.m_lineno);
					exit(-1);
				}
				next = buf.current();
				buf.advance();

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

				const std::string& outcode = compile(linker, body, debugSymbols, false, false, true, true, locals, funcArgs);
				code << outcode;
				code << "JMP .while" << currWhileCount << '\n';
				code << ".endwhile" << currWhileCount << '\n';

				break;
			}

			case TokenType::TT_IMPORT:
			{
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

				importLibrary(linker, libName);

				break;
			}

			case TokenType::TT_URCL_BLOCK:
			{
				if (debugSymbols)
					code << "// " << getSourceLine(glob_src, current.m_lineno);

				buf.advance();
				if (!buf.hasNext() || buf.current().m_type != TokenType::TT_STR)
				{
					std::cerr << "Error: Expected URCL Code in string after urcl at line " << current.m_lineno << '\n';
					std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
					drawArrows(current.m_start, current.m_end, current.m_lineno);
					exit(-1);
				}
				const Token& curr = buf.current();

				code << curr.m_val << "\n\n";

				buf.advance();
				if (!buf.hasNext() || buf.current().m_type != TokenType::TT_SEMICOLON)
				{
					std::cerr << "Error: Expected `;` after URCL code block at line " << curr.m_lineno << '\n';
					std::cerr << curr.m_lineno << ": " << getSourceLine(glob_src, curr.m_lineno);
					drawArrows(curr.m_start, curr.m_end, curr.m_lineno);
					exit(-1);
				}

				break;
			}

			case TokenType::TT_RETURN:
			{
				if (debugSymbols)
					code << "// " << getSourceLine(glob_src, current.m_lineno);

				buf.advance();
				if (!buf.hasNext())
				{
					std::cerr << "Error: Expected expression after return at line " << current.m_lineno << '\n';
					std::cerr << current.m_lineno << ": " << getSourceLine(glob_src, current.m_lineno);
					drawArrows(current.m_start, current.m_end, current.m_lineno);
					exit(-1);
				}

				std::vector<Token> expr;
				while (buf.hasNext() && buf.current().m_type != TokenType::TT_SEMICOLON)
				{
					const Token& curr = buf.current();

					if (!isNumber(curr) && !isOperator(curr) && curr.m_type != TokenType::TT_IDENTIFIER)
					{
						std::cerr << "Error: Expression cannot have non-number or non-operator or non-identifer tokens at line " << curr.m_lineno << '\n';
						std::cerr << curr.m_lineno << ": " << getSourceLine(glob_src, curr.m_lineno);
						drawArrows(curr.m_start, curr.m_end, curr.m_lineno);
						exit(-1);
					}

					expr.push_back(curr);
					buf.advance();
				}

				auto [val, _code] = parseExpr(expr, locals, funcArgs);
				if (_code.size() == 0)
					code << "IMM R2 " << val << "\n\n";
				else
					code << _code << '\n';
				
				// cdecl calling convention exit
				code << "MOV SP R1\nPOP R1\nRET\n\n";

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

	if (emitEntryPoint)
		code << "\nCAL ._Hx4maini8\nMOV SP R1\nHLT\n\n";

	if (popFrame)
		code << "ADD SP SP " << locals.popFrame() << '\n';
	
	if (emitFunctions)
	{
		for (const Function& func: linker.getFunctions())
		{
			code << '.' << func.getSignature() << '\n';

			// cdecl calling convention entry
			code << "PSH R1\nMOV R1 SP\n\n";

			code << func.code;

			// cdecl calling convention exit
			code << "MOV SP R1\nPOP R1\nRET\n\n";
		}

		for (const std::string& str: getStrings())
			code << str << "\n\n";
	}

	return code.str();
}
