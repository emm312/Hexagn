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
	"uint64",

	// Floats
	"float32",
	"float64",

	"string",
	"char"
};

class Buffer
{
private:
	std::string m_data;
	size_t m_index = 0;

public:
	Buffer(const std::string &data)
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

TokenTypeAndWord makeWord(const char& data, Buffer& buf, const std::string& source, const size_t& lineno)
{
	std::string word;

	size_t start = buf.pos() - find_nth(source, '\n', lineno - 1);
	size_t end = start;
	
	// Is keyword or identifier
	if (isalpha(data) || data == '_')
	{
		while (buf.hasNext())
		{
			if (!isalnum(buf.current()))
				break;

			word += buf.current();
			end++;

			buf.advance();
		}

		return std::find(std::begin(KEYWORDS), std::end(KEYWORDS), word) != std::end(KEYWORDS)
				? TokenTypeAndWord { TokenType::TT_KEYWORD,    word, start, end }
				: TokenTypeAndWord { TokenType::TT_IDENTIFIER, word, start, end };
	}

	// Is number
	else if (isdigit(data))
	{
		while (buf.hasNext())
		{
			if (!isdigit(buf.current()))
			{
				std::cerr << "Error: Invalid number at line " << lineno << '\n';
				std::cerr << lineno << ": " << getSourceLine(source, lineno) << '\n';
				drawArrows(start, end);
				exit(-1);
			}

			word += buf.current();
			end++;

			buf.advance();
		}

		return TokenTypeAndWord { TokenType::TT_NUM, word, start, end };
	}

	// Invalid character
	else
	{
		std::cerr << "Invalid character at line " << lineno << '\n';
		std::cerr << lineno << ": " << getSourceLine(source, lineno);
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

		if (data == ' ')
		{
			buf.advance();
			continue;
		}

		else if (data == '\n')
			lineno++;

		else if (data == ';')
			toks.push_back(Token(lineno, TokenType::TT_SEMICOLON, ";",
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));

		else if (data == '=')
			toks.push_back(Token(lineno, TokenType::TT_ASSIGN, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));

		else if (data == '+')
			toks.push_back(Token(lineno, TokenType::TT_PLUS, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
		else if (data == '-')
			toks.push_back(Token(lineno, TokenType::TT_MINUS, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
		else if (data == '*')
			toks.push_back(Token(lineno, TokenType::TT_MULT, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
		else if (data == '/')
			toks.push_back(Token(lineno, TokenType::TT_DIV, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
		else if (data == '%')
			toks.push_back(Token(lineno, TokenType::TT_MOD, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));

		else if (data == '(')
		{
			toks.push_back(Token(lineno, TokenType::TT_OPEN_PAREN, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
		}
		else if (data == ')')
		{
			toks.push_back(Token(lineno, TokenType::TT_CLOSE_PAREN, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
		}

		else if (data == ',')
		{
			toks.push_back(Token(lineno, TokenType::TT_COMMA, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
		}

		else if (data == '{')
		{
			toks.push_back(Token(lineno, TokenType::TT_OPEN_BRACE, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
		}
		else if (data == '}')
		{
			toks.push_back(Token(lineno, TokenType::TT_CLOSE_BRACE, std::string(1, data),
								 buf.pos() - find_nth(source, '\n', lineno - 1),
								 buf.pos() - find_nth(source, '\n', lineno - 1)));
		}

		else if (data == '\'')
		{
			const size_t& start = buf.pos();

			buf.advance();
			char _char;

			if (buf.current() == '\\')
			{
				buf.advance();
				_char = buf.current();

				if (_char == 'n')
					_char = '\n';
				else if (_char == 't')
					_char = '\t';
				else if (_char == '\'')
					_char = '\'';
				else if (_char == '\\')
					_char = '\\';
				else
				{
					std::cerr << "Invalid escape sequence at line " << lineno << '\n';
					std::cerr << lineno << ": " << getSourceLine(source, lineno);
					drawArrows(buf.pos() - find_nth(source, '\n', lineno - 1) - 1,
							   buf.pos() - find_nth(source, '\n', lineno - 1));
					exit(-1);
				}
			}
			else
				_char = buf.current();

			buf.advance();
			if (buf.current() != '\'')
			{
				std::cerr << "Expected closing ' for character literal at line " << lineno << '\n';
				std::cerr << lineno << ": " << getSourceLine(source, lineno);
				drawArrows(buf.pos() - find_nth(source, '\n', lineno - 1),
						   buf.pos() - find_nth(source, '\n', lineno - 1));
				exit(-1);
			}

			const size_t& end = buf.pos();

			toks.push_back(Token(lineno, TokenType::TT_CHAR, std::string(1, _char), start, end));
		}

		else if (data == '"')
		{
			const size_t& start = buf.pos();

			std::string str;

			buf.advance();
			while (buf.current() != '"')
			{
				if (buf.current() == '\\')
				{
					buf.advance();
					if (buf.current() == 'n')
					{
						str += '\n';
					}
					else if (buf.current() == 't')
					{
						str += '\t';
					}
					else if (buf.current() == '\\')
					{
						str += '\\';
					}
					else if (buf.current() == '"')
					{
						str += '"';
					}
					else
					{
						std::cerr << "Unknown escape sequence at line " << lineno << '\n';
						std::cerr << lineno << ": " << getSourceLine(source, lineno);
						drawArrows(buf.pos() - find_nth(source, '\n', lineno - 1),
								   buf.pos() - find_nth(source, '\n', lineno - 1));
						exit(-1);
					}
				}

				else if (buf.current() == '\n')
				{
					std::cerr << "Unterminated string at line " << lineno << '\n';
					std::cerr << lineno << ": " << getSourceLine(source, lineno);
					drawArrows(buf.pos() - find_nth(source, '\n', lineno - 1) - 2,
							   buf.pos() - find_nth(source, '\n', lineno - 1) - 2);
					exit(-1);
				}

				else
					str += buf.current();

				buf.advance();
			}

			const size_t& end = buf.pos();

			toks.push_back(Token(lineno, TokenType::TT_STR, str, start, end));
		}

		else if (isalpha(data))
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
			std::cerr << lineno << ": " << getSourceLine(source, lineno);
			size_t i = buf.pos() - find_nth(source, '\n', lineno - 1);
			size_t j = source.find_first_of(' ', i);
			std::cout << j << '\n';
			drawArrows(i, j);
			exit(-1);
		}

		buf.advance();
	}

	return toks;
}
