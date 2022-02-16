#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <string>
#include <vector>

//--------------------------------//
//            LEXER
//--------------------------------//

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
	TT_POW
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


//--------------------------------//
//		  FUNCTIONS
//--------------------------------//

std::vector<Token> tokenize(std::string source);
void compile(std::vector<Token> tokens, std::string outputFileName);

#endif // PARSER_H
