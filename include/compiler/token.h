#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType: size_t
{
	TT_KEYWORD,
	TT_IDENTIFIER,
	TT_ASSIGN,
	TT_NUM,
	TT_FLOAT,
	TT_STR,
	TT_CHAR,

	TT_OPEN_PAREN,
	TT_CLOSE_PAREN,
	TT_COMMA,

	TT_SEMICOLON,

	TT_PLUS,
	TT_MINUS,
	TT_MULT,
	TT_DIV,
	TT_POW,
	TT_MOD,

	TT_OPEN_BRACE,
	TT_CLOSE_BRACE,
};

extern const std::string KEYWORDS[];

struct Token
{
	size_t m_lineno;
	TokenType m_type;
	std::string m_val;

	size_t m_start;
	size_t m_end;

	Token(size_t lineno, TokenType type, std::string val, size_t start, size_t end);

	std::string toString() const;
};

#endif // TOKEN_H
