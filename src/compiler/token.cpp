#include <string>

#include <compiler/token.h>
#include <util.h>

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
