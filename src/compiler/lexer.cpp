#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <stack>
#include <queue>
#include <functional>

#include <compiler/lexer.h>
#include <util.h>

// Global source variable for the compile function to print errors
std::string glob_src;

//--------------------------------//
//		  LEXER
//--------------------------------//

const std::string KEYWORDS[] =
{
	// Data types

	"void",

	// Signed data types
	"int8",
	"int16",
	"int32",
	"int64",

	// Unsigned data types
	"uint8",
	"uint16",
	"uint32",
	"uint64"
};

class Buffer
{
private:
	std::string m_data;
	size_t m_index = 0;

public:
	Buffer(const std::string &data)
		: m_data(data)
	{
	}

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

TokenTypeAndWord makeWord(const char& data, Buffer& buf, const std::string& source, const size_t& lineno)
{
	std::string word;

	size_t start = buf.pos() - find_nth(source, '\n', lineno - 1);
	size_t end = start;
	
	// Is keyword or identifier
	if (isalpha(data) || data == '_')
	{
		word += buf.current();
		end++;
		while (buf.hasNext() && isalnum(buf.next()))
		{
			word += buf.current();
			end++;
		}

		return std::find(std::begin(KEYWORDS), std::end(KEYWORDS), word) != std::end(KEYWORDS)
				   ? TokenTypeAndWord{TokenType::TT_KEYWORD, word, start, end}
				   : TokenTypeAndWord{TokenType::TT_IDENTIFIER, word, start, end};
	}

	// Is number
	else if (isdigit(data))
	{
		while (buf.hasNext())
		{
			const char& curr = buf.current();
			if (!isdigit(curr))
			{
				std::cerr << "Invalid number at line " << lineno << '\n';
				std::cerr << getSourceLine(source, lineno);
				drawArrows(start, end);
				exit(-1);
			}

			word += curr;
			end++;
		}

		return TokenTypeAndWord{TokenType::TT_NUM, word, start, end};
	}

	// Invalid character
	else
	{
		std::cerr << "Invalid character at line " << lineno << '\n';
		std::cerr << getSourceLine(source, lineno);
		drawArrows(start, end);
		exit(-1);
	}
}

std::vector<Token> tokenize(std::string source)
{
	glob_src = source;

	std::vector<Token> toks;
	Buffer buf(source);

	size_t lineno = 1;

	while (buf.hasNext())
	{
		const char& data = buf.current();

		if (data == ' ' || data == '\t')
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

		if (data == ',')
		{
			toks.push_back(Token(lineno, TokenType::TT_COMMA, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));

			buf.advance();
			continue;
		}

		if (data == '{')
		{
			toks.push_back(Token(lineno, TokenType::TT_OPEN_BRACE, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
			buf.advance();
			continue;
		}
		else if (data == '}')
		{
			toks.push_back(Token(lineno, TokenType::TT_CLOSE_BRACE, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
			buf.advance();
			continue;
		}

		if (isalpha(data))
		{
			auto [type, word, start, end] = makeWord(data, buf, source, lineno);
			toks.push_back(Token(lineno, type, word, start, end));
			continue;
		}

		else if (isdigit(data))
		{
			std::string word = "";
			word += data;

			size_t start = buf.pos() - find_nth(source, '\n', lineno - 1);
			size_t end = start;

			while (isdigit(buf.next()))
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
			std::cerr << getSourceLine(source, lineno);
			size_t i = buf.pos() - find_nth(source, '\n', lineno - 1);
			size_t j = source.find_first_of(' ', i);
			drawArrows(i, j);
			exit(-1);
		}

		buf.advance();
	}

	return toks;
}
