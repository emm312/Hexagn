#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <functional>
#include <ranges>

#include <compiler/parser.h>
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

void VarStack::push(const std::string &name)
{
	if (!m_vars.empty())
		m_vars.push_back( { name, m_vars[m_vars.size() - 1].stackOffset + 1 } );
	else
		m_vars.push_back( { name, 1 } );
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

const size_t VarStack::getSize() const
{
	return m_vars.size();
}

struct VarStackFrame
{
	std::string val;
	std::string code;
};

struct Function
{
	struct ArgType
	{
		const Token type;
		const Token identifer;
	};

	const std::string name;
	std::vector<ArgType> args;
	std::string code;
};

VarStackFrame parseExpr(const std::vector<Token>& toks, const VarStack& locals, const VarStack& funcArgs)
{
	if (toks.size() == 1)
		return VarStackFrame{ toks[0].m_val, "" };
	else
	{
		// Convert infix to prefix from toks vector

		bool (*isOperator)(const Token&) = [](const Token& tok)
		{
			switch (tok.m_type)
			{
				case TokenType::TT_PLUS:
				case TokenType::TT_MINUS:
				case TokenType::TT_MULT:
				case TokenType::TT_DIV:
				case TokenType::TT_POW:
					return true;

				default:
					return false;
			}
		};

		size_t (*getPriority)(const Token&) = [](const Token& tok) -> size_t
		{
			switch (tok.m_type)
			{
				case TokenType::TT_PLUS:
				case TokenType::TT_MINUS:
					return 1;
				case TokenType::TT_MULT:
				case TokenType::TT_DIV:
					return 2;
				case TokenType::TT_POW:
					return 3;
				default:
					return 0;
			}
		};

		// I really hate auto lambdas

		auto infixToPostfix = [&isOperator, &getPriority](const std::vector<Token>& toks)
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
		};

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

		std::string (*getVal)(const Token&) = [](const Token& tok) -> std::string
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

				case TokenType::TT_STR:
				{
					return "\" " + tok.m_val + "\"";
					break;
				}

				case TokenType::TT_CHAR:
				{
					return "'" + tok.m_val + "'";
					break;
				}

				default:
					return "";
			}
		};

		std::string code;
		TokenBuffer buf(prefix);
		std::stack<std::string> codeStack;
		std::queue<std::string> varsQueue;

		std::string (*getOpName)(const Token&) = [](const Token& tok) -> std::string
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

				default:
					return "";
			}
		};

		// I HATE THAT I HAVE TO USE STD::FUNCTION

		std::function<std::string(TokenBuffer&, size_t&)> parseOp = [&isOperator, &getVal, &getOpName, &codeStack, &varsQueue, &parseOp, &locals, &funcArgs](TokenBuffer& buf, size_t& regIndex) -> std::string
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
						std::cout << "Error: Identifier " << next.m_val << " is not defined in current context\n";
						printLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end);
						exit(-1);
					}

					offset = funcArgs.getOffset(next.m_val);
					if (offset == size_t(-1))
					{
						std::cout << "Error: Identifier " << next.m_val << " is not defined in current context\n";
						printLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end);
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
						std::cout << "Error: Identifier " << next.m_val << " is not defined in current context\n";
						printLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end);
						exit(-1);
					}

					offset = funcArgs.getOffset(next.m_val);
					if (offset == size_t(-1))
					{
						std::cout << "Error: Identifier " << next.m_val << " is not defined in current context\n";
						printLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end);
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

std::vector<Function> funcs;

std::stringstream compile(std::vector<Token> tokens, bool isFunc, const VarStack& funcArgs)
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
			case TokenType::TT_KEYWORD:
			{
				Token next = buf.next();
				if (!buf.hasNext() || next.m_type != TokenType::TT_IDENTIFIER)
				{
					std::cerr << "Error: Expected identifier after keyword at line " << current.m_lineno << '\n';
					printLine(glob_src, current.m_lineno);
					drawArrows(current.m_start, current.m_end);
					exit(-1);
				}
				const Token& identifier = buf.current();

				next = buf.next();
				if (!buf.hasNext() && !(next.m_type == TokenType::TT_ASSIGN || next.m_type == TokenType::TT_OPEN_PAREN))
				{
					std::cerr << "Error: Expected '=' or '(' at line " << next.m_lineno << '\n';
					printLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end);
					exit(-1);
				}

				// Variable definition
				if (next.m_type == TokenType::TT_ASSIGN)
				{
					buf.advance();
					if (!buf.hasNext())
					{
						std::cerr << "Error: Expected expression after '=' at line " << next.m_lineno << '\n';
						printLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end);
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
					code << "PSH " << val << "\n\n";

					locals.push(identifier.m_val);
				}

				// Function definition
				else if (next.m_type == TokenType::TT_OPEN_PAREN)
				{
					Function func { identifier.m_val };

					next = buf.next();
					if (!buf.hasNext())
					{
						std::cerr << "Error: Expected keyword or ')' at line " << next.m_lineno << '\n';
						printLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end);
						exit(-1);
					}

					/**
					 * @brief Append parameters in loop
					*/
					VarStack funcArgsStack;
					while (buf.hasNext() && buf.current().m_type != TokenType::TT_CLOSE_PAREN)
					{
						const Token& type = buf.current();
						if (!buf.hasNext() || type.m_type != TokenType::TT_KEYWORD)
						{
							std::cerr << "Error: Expected keyword or ')' at line " << type.m_lineno << '\n';
							printLine(glob_src, type.m_lineno);
							drawArrows(type.m_start, type.m_end);
							exit(-1);
						}

						next = buf.next();
						if (!buf.hasNext() || next.m_type != TokenType::TT_IDENTIFIER)
						{
							std::cerr << "Error: Expected identifier after keyword at line " << next.m_lineno << '\n';
							printLine(glob_src, next.m_lineno);
							drawArrows(next.m_start, next.m_end);
							exit(-1);
						}
						const Token& identifier = buf.current();

						next = buf.next();

						func.args.push_back( { type, identifier } );
						funcArgsStack.push(identifier.m_val);

						if (!buf.hasNext())
						{
							std::cerr << "Error: Expected ',' at line " << next.m_lineno << '\n';
							printLine(glob_src, next.m_lineno);
							drawArrows(next.m_start, next.m_end);
							exit(-1);
						}
						
						if (next.m_type == TokenType::TT_CLOSE_PAREN)
							break;
						else if (next.m_type != TokenType::TT_COMMA)
						{
							std::cerr << "Error: Expected ',' at line " << next.m_lineno << '\n';
							printLine(glob_src, next.m_lineno);
							drawArrows(next.m_start, next.m_end);
							exit(-1);
						}

						buf.advance();
					}

					next = buf.next();

					// Parse function body

					if (!buf.hasNext() || next.m_type != TokenType::TT_OPEN_BRACE)
					{
						std::cerr << "Error: Expected '{' at line " << next.m_lineno << '\n';
						printLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end);
						std::cout << std::to_string(next.m_type) << '\n';
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

					func.code = compile(body, true, funcArgsStack).str();
					funcs.push_back(func);
				}

				break;
			}

			// Variable reassignment or function call
			case TokenType::TT_IDENTIFIER:
			{
				const Token& identifier = buf.current();
				size_t offset = locals.getOffset(identifier.m_val);
				bool isInArgs = false;

				Token next = buf.next();
				if (!buf.hasNext() && !(next.m_type == TokenType::TT_ASSIGN || next.m_type == TokenType::TT_OPEN_PAREN))
				{
					std::cerr << "Error: Expected '=' or function call at line " << next.m_lineno << '\n';
					printLine(glob_src, next.m_lineno);
					drawArrows(next.m_start, next.m_end);
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
							printLine(glob_src, identifier.m_lineno);
							drawArrows(identifier.m_start, identifier.m_end);
							exit(-1);
						}
					}

					buf.advance();
					if (!buf.hasNext())
					{
						std::cerr << "Error: Expected expression after '=' at line " << next.m_lineno << '\n';
						printLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end);
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
						printLine(glob_src, next.m_lineno);
						drawArrows(next.m_start, next.m_end);
						exit(-1);
					}

					/**
					 * @brief Append function arguments in loop
					 */
					std::vector<Token> args;
					while (buf.hasNext() && buf.current().m_type != TokenType::TT_CLOSE_PAREN)
					{
						const Token& current = buf.current();
						if (!buf.hasNext() || (current.m_type != TokenType::TT_IDENTIFIER && !isVal(current)))
						{
							std::cerr << "Error: Expected identifier or value at line " << buf.current().m_lineno << '\n';
							printLine(glob_src, buf.current().m_lineno);
							drawArrows(buf.current().m_start, buf.current().m_end);
							exit(-1);
						}

						if (current.m_type == TokenType::TT_IDENTIFIER && locals.getOffset(current.m_val) == size_t(-1) && funcArgs.getOffset(current.m_val) == size_t(-1))
						{
							std::cerr << "Error: No such variable '" << current.m_val << "' in current context at line " << current.m_lineno << '\n';
							printLine(glob_src, current.m_lineno);
							drawArrows(current.m_start, current.m_end);
							exit(-1);
						}

						args.push_back(current);

						buf.advance();

						if (buf.current().m_type == TokenType::TT_CLOSE_PAREN)
							break;

						if (!buf.hasNext() || buf.current().m_type != TokenType::TT_COMMA)
						{
							std::cerr << "Error: Expected ',' or ')' at line " << buf.current().m_lineno << '\n';
							printLine(glob_src, buf.current().m_lineno);
							drawArrows(buf.current().m_start, buf.current().m_end);
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
								printLine(glob_src, arg.m_lineno);
								drawArrows(arg.m_start, arg.m_end);
								exit(-1);
							}

							val = "R2";
						}
						else
							val = arg.m_val;
						
						code << "PSH " << val << '\n';
					}
					
					code << "CAL ." << identifier.m_val << '\n';

					// Stack cleanup
					code << "ADD SP SP " << args.size() << "\n\n";
				}
			}

			case TokenType::TT_SEMICOLON: { buf.advance(); continue; }
			default: break;
		}

		buf.advance();
	}

	if (!isFunc)
	{
		code << "\nCAL .main\nMOV SP R1\nHLT\n\n";
		for (const auto& func: funcs)
		{
			code << '.' << func.name << '\n';
			
			// cdecl calling convention entry
			code << "PSH R1\nMOV R1 SP\n\n";

			code << func.code;

			// cdecl calling convention exit
			code << "MOV SP R1\nPOP R1\nRET\n\n";
		}
	}

	return code;
}
