#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <stack>
#include <queue>
#include <functional>

#include <compiler/parser.h>
#include <util.h>

//--------------------------------//
//		  LEXER
//--------------------------------//

const std::string KEYWORDS[] =
{
	// Data types
	
	// Signed data types
	"int8",
	"int16",
	"int32",
	"int64",

	// Unsigned data types
	"uint8",
	"uint16",
	"uint32",
	"uint64",

	// Function keywords
	"func"
};

Token::Token(size_t lineno, TokenType type, std::string val, size_t start, size_t end)
: m_lineno(lineno), m_type(type), m_val(val), m_start(start), m_end(end)
{}

std::string Token::toString() const
{
	return "Token{ type='"  + std::to_string(m_type)
			+ "', value='"  + m_val
			+ "', line='"   + std::to_string(m_lineno)
			+ "', start='"  + std::to_string(m_start) + "', end='" + std::to_string(m_end) + "' }";
}

class Buffer
{
private:
	std::string m_data;
	size_t m_index = 0;

public:
	Buffer(const std::string& data)
	: m_data(data)
	{}

	bool hasNext() const
	{
		return m_index < m_data.size() - 1;
	}

	void advance()
	{
		m_index++;
	}

	char next()
	{
		return m_data[++m_index];
	}

	char current() const
	{
		return m_data[m_index];
	}

	size_t pos() const
	{
		return m_index;
	}
};

struct TokenTypeAndWord
{
	TokenType tokType;
	std::string word;

	size_t start;
	size_t end;
};

TokenTypeAndWord makeKeywordOrIdentifier(const char& data, Buffer& buf, const std::string& source, const size_t& lineno)
{
	std::string word = "";
	if (data != ' ')
		word += data;

	size_t start = buf.pos() - find_nth(source, '\n', lineno - 1);
	size_t end = start;

	while(isalnum(buf.next()) && buf.hasNext())
	{
		word += buf.current();
		end++;
	}

	return std::find(std::begin(KEYWORDS), std::end(KEYWORDS), word) != std::end(KEYWORDS)
		? TokenTypeAndWord{TokenType::TT_KEYWORD, word, start, end}
		: TokenTypeAndWord{TokenType::TT_IDENTIFIER, word, start, end};
}

std::vector<Token> tokenize(std::string source)
{
	std::vector<Token> toks;
	Buffer buf(source);

	size_t lineno = 1;

	while (buf.hasNext())
	{
		const char& data = buf.current();

		if (data == ' ')
		{
			buf.advance();
			continue;
		}
		
		if (data == '\n')
		{
			lineno++;
			buf.advance();
			continue;
		}
		
		if (data == ';')
		{
			toks.push_back(Token(lineno, TokenType::TT_SEMICOLON, ";",
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
			
			buf.advance();
			continue;
		}

		if (data == '=')
		{
			toks.push_back(Token(lineno, TokenType::TT_ASSIGN, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
			
			buf.advance();
			continue;
		}

		if (data == '+')
		{
			toks.push_back(Token(lineno, TokenType::TT_PLUS, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
			
			buf.advance();
			continue;
		}
		else if (data == '-')
		{
			toks.push_back(Token(lineno, TokenType::TT_MINUS, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
			
			buf.advance();
			continue;
		}
		else if (data == '*')
		{
			toks.push_back(Token(lineno, TokenType::TT_MULT, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
			
			buf.advance();
			continue;
		}
		else if (data == '/')
		{
			toks.push_back(Token(lineno, TokenType::TT_DIV, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
			
			buf.advance();
			continue;
		}
		else if (data == '^')
		{
			toks.push_back(Token(lineno, TokenType::TT_POW, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
			
			buf.advance();
			continue;
		}
		
		if (data == '(')
		{
			toks.push_back(Token(lineno, TokenType::TT_OPEN_PAREN, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
			
			buf.advance();
			continue;
		}
		else if (data == ')')
		{
			toks.push_back(Token(lineno, TokenType::TT_CLOSE_PAREN, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
			
			buf.advance();
			continue;
		}

		if (isalpha(data))
		{
			auto [type, word, start, end] = makeKeywordOrIdentifier(data, buf, source, lineno);
			toks.push_back(Token(lineno, type, word, start, end));

			if (buf.current() == '(')
			{
				toks.push_back(Token(lineno, TokenType::TT_OPEN_PAREN, std::string(1, '('), end+1, end+1));
				buf.advance();
				
				while (buf.hasNext() && buf.current() != ')')
				{
					auto [type2, word2, start2, end2] = makeKeywordOrIdentifier(buf.current(), buf, source, lineno);
					auto [type3, word3, start3, end3] = makeKeywordOrIdentifier(buf.current(), buf, source, lineno);

					toks.push_back(Token(lineno, type2, word2, start2, end2));
					toks.push_back(Token(lineno, type3, word3, start3, end3));
					
					if (buf.current() == ',')
					{
						toks.push_back(Token(lineno, TokenType::TT_COMMA, std::string(1, ','), end3+1, end3+1));
						buf.advance();
					}
				}

				if (buf.current() == ')')
				{
					toks.push_back(Token(lineno, TokenType::TT_CLOSE_PAREN, std::string(1, ')'),
										 buf.pos() - find_nth(source, '\n', lineno - 1),
										 buf.pos() - find_nth(source, '\n', lineno - 1)));
					buf.advance();
				}
				else
				{
					std::cerr << "Error: Expected ')' at line " << lineno << '\n';

					size_t i;
					for (i = find_nth(source, '\n', lineno - 1); source[i] != '\n'; ++i)
						std::cerr << source[i];
					std::cerr << '\n';

					i = buf.pos() - find_nth(source, '\n', lineno - 1) - 3;	 // WHY THE -3?????
					drawArrows(i, i);

					exit(-1);
				}
			}

			continue;
		}
		else if (isdigit(data))
		{
			std::string word = "";  
			word += data;

			size_t start = buf.pos() - find_nth(source, '\n', lineno - 1);
			size_t end = start;

			while(isdigit(buf.next()))
			{
				word += buf.current();
				end++;
			}

			toks.push_back(Token(lineno, TokenType::TT_NUM, word, start, end));

			continue;
		}

		else
		{
			std::cerr << "Invalid syntax at line " << lineno << '\n';

			size_t i;
			for (i = find_nth(source, '\n', lineno - 1); source[i] != '\n'; ++i)
				std::cerr << source[i];
			std::cerr << '\n';

			i = buf.pos() - find_nth(source, '\n', lineno - 1);
			drawArrows(i, i);

			exit(-1);
		}

		buf.advance();
	}

	return toks;
}

//--------------------------------//
//		  COMPILER
//--------------------------------//

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

	Token& next()
	{
		return m_tokens[++m_index];
	}

	Token& current()
	{
		return m_tokens[m_index];
	}

	size_t& pos()
	{
		return m_index;
	}
};

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
	void push(const std::string& name)
	{
		if (!m_vars.empty())
			m_vars.push_back({name, m_vars[m_vars.size() - 1].stackOffset + 1});
		else
			m_vars.push_back({name, 1});
	}

	void pop()
	{
		m_vars.pop_back();
	}

	const size_t getOffset(const std::string& name) const
	{
		for (const auto& var: m_vars)
			if (var.name == name)
				return var.stackOffset;
		return -1;
	}

	const size_t getSize() const
	{
		return m_vars.size();
	}
};

struct VarStackFrame
{
	std::string val;
	std::string code;
};

VarStackFrame parseExpr(std::vector<Token> toks, const VarStack& globals)
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
		size_t l = toks.size();
		std::reverse(toks.begin(), toks.end());
		for (size_t i = 0; i < l; ++i)
		{
			if (toks[i].m_type == TokenType::TT_OPEN_PAREN)
			{
				toks[i].m_type = TokenType::TT_CLOSE_PAREN;
				toks[i].m_val = std::string(')', 1);
			}
			else if (toks[i].m_type == TokenType::TT_CLOSE_PAREN)
			{
				toks[i].m_type = TokenType::TT_OPEN_PAREN;
				toks[i].m_val = std::string('(', 1);
			}
		}
		std::vector<Token> prefix = infixToPostfix(toks);
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

		std::function<std::string(TokenBuffer&, size_t&)> parseOp = [&isOperator, &getVal, &getOpName, &codeStack, &varsQueue, &parseOp, &globals](TokenBuffer& buf, size_t& regIndex) -> std::string
		{
			std::string _return;

			const Token& tok = buf.current();
			_return += getOpName(tok) + ' ';
			_return += "R" + std::to_string(regIndex) + ' ';
			
			Token& next = buf.next();
			if (isOperator(next))
			{
				regIndex++;
				codeStack.push(parseOp(buf, regIndex));
				_return += "R" + std::to_string(regIndex) + ' ';
			}
			else if (next.m_type == TokenType::TT_IDENTIFIER)
			{
				regIndex++;
				const size_t& offset = globals.getOffset(next.m_val);
				varsQueue.push("LLOD R" + std::to_string(regIndex) + " R1 -" + std::to_string(offset) + '\n');
				_return += "R" + std::to_string(regIndex) + ' ';
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
				const size_t& offset = globals.getOffset(next.m_val);
				varsQueue.push("LLOD R" + std::to_string(regIndex) + " R1 -" + std::to_string(offset) + '\n');
				_return += "R" + std::to_string(regIndex) + '\n';
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

		return VarStackFrame{ "R2", code };
	}
}

void compile(std::vector<Token> tokens, std::string outputFileName)
{
	TokenBuffer buf(tokens);
	std::stringstream code;
	VarStack globals;

	// Headers
	code << "BITS == 32\n";
	code << "MINHEAP 4096\n";
	code << "MINSTACK 1024\n";
	code << "MOV R1 SP\n\n\n";

	while (buf.hasNext())
	{
		const Token& current = buf.current();

		switch (current.m_type)
		{
			case TokenType::TT_KEYWORD:
			{
				Token next = buf.next();
				if (next.m_type != TokenType::TT_IDENTIFIER || !buf.hasNext())
				{
					std::cerr << "Error: Expected identifier after keyword " << current.m_val << " at line " << current.m_lineno << '\n';
					exit(-1);
				}

				const Token& identifier = buf.current();

				next = buf.next();
				if (!buf.hasNext())
				{
					std::cerr << "Error: Expected '=' or '(' at " << next.m_val << " at line " << next.m_lineno << '\n';
					exit(-1);
				}
				
				if (next.m_type == TokenType::TT_ASSIGN)
				{
					buf.advance();

					std::vector<Token> expr;
					while (buf.hasNext() && buf.current().m_type != TokenType::TT_SEMICOLON)
					{
						expr.push_back(buf.current());
						buf.advance();
					}

					auto [val, _code] = parseExpr(expr, globals);
					code << _code;
					code << "PSH " << val << "\n\n";

					globals.push(identifier.m_val);
				}
				else if (next.m_type == TokenType::TT_OPEN_PAREN)
				{

				}
				else
				{
					std::cerr << "Error: Expected '=' or '(' at " << next.m_val << " at line " << next.m_lineno << '\n';
					exit(-1);
				}

				break;
			}

			default:
				break;
		}

		buf.advance();
	}

	code << "\nMOV SP R1\nHLT\n";

	std::ofstream(outputFileName) << code.str();
}
