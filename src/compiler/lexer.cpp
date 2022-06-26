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
const std::string SignedIntTypes[] =
{
	"int8",
	"int16",
	"int32",
	"int64"
};

const std::string UnsignedIntTypes[] =
{
	"uint8",
	"uint16",
	"uint32",
	"uint64"
};

const std::string FloatTypes[] =
{
	"float32",
	"float64"
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

	while (buf.hasNext())
	{
		const char& curr = buf.current();

		if (curr == ' ')                                  break;
		if (!isalnum(curr) && curr != '_' && curr != '-') break;

		word += curr;
		end++;

		buf.advance();
	}

	if (word == "void") return { TokenType::TT_VOID, word, start, end };

	else if (std::find(std::begin(SignedIntTypes), std::end(SignedIntTypes), word) != std::end(SignedIntTypes))
		return { TokenType::TT_INT, word, start, end };
	
	else if (std::find(std::begin(UnsignedIntTypes), std::end(UnsignedIntTypes), word) != std::end(UnsignedIntTypes))
		return { TokenType::TT_UINT, word, start, end };

	else if (std::find(std::begin(FloatTypes), std::end(FloatTypes), word) != std::end(FloatTypes))
		return { TokenType::TT_FLOAT, word, start, end };

	else if (word == "string") return { TokenType::TT_STRING,    word, start, end };
	else if (word == "char")   return { TokenType::TT_CHARACTER, word, start, end };

	else return { TokenType::TT_IDENTIFIER, word, start, end };
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
			const size_t& start = buf.pos() - find_nth(source, '\n', lineno - 1);

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
					drawArrows(
						buf.pos() - find_nth(source, '\n', lineno - 1) - 1,
						buf.pos() - find_nth(source, '\n', lineno - 1),
						lineno
					);
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
				drawArrows(
					buf.pos() - find_nth(source, '\n', lineno - 1),
					buf.pos() - find_nth(source, '\n', lineno - 1),
					lineno
				);
				exit(-1);
			}

			const size_t& end = buf.pos() - find_nth(source, '\n', lineno - 1);

			toks.push_back(Token(lineno, TokenType::TT_CHAR, std::string(1, _char), start, end));
		}

		else if (data == '"')
		{
			const size_t& start = buf.pos() - find_nth(source, '\n', lineno - 1);

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
						drawArrows(
							buf.pos() - find_nth(source, '\n', lineno - 1),
							buf.pos() - find_nth(source, '\n', lineno - 1),
							lineno
						);
						exit(-1);
					}
				}

				else if (buf.current() == '\n')
				{
					std::cerr << "Unterminated string at line " << lineno << '\n';
					std::cerr << lineno << ": " << getSourceLine(source, lineno);
					drawArrows(
						buf.pos() - find_nth(source, '\n', lineno - 1) - 2,
						buf.pos() - find_nth(source, '\n', lineno - 1) - 2,
						lineno
					);
					exit(-1);
				}

				else
					str += buf.current();

				buf.advance();
			}

			const size_t& end = buf.pos() - find_nth(source, '\n', lineno - 1);

			toks.push_back(Token(lineno, TokenType::TT_STR, str, start, end));
		}

		else if (isalpha(data) || data == '_')
		{
			auto [type, word, start, end] = makeWord(data, buf, source, lineno);
			toks.push_back(Token(lineno, type, word, start, end));
			continue;
		}

		else if (isdigit(data))
		{
			std::string word = "";

			size_t start = buf.pos() - find_nth(source, '\n', lineno - 1);
			size_t end = start;

			while (buf.hasNext())
			{
				if (buf.current() == ';') break;

				if (!isdigit(buf.current()))
					break;

				word += buf.current();
				end++;

				buf.advance();
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
			drawArrows(i, j, lineno);
			exit(-1);
		}

		buf.advance();
	}

	return toks;
}
