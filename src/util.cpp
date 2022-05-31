#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <util.h>

int indexOf(char* arr[], std::string element, int size)
{
	for (int i = 0; i < size; ++i)
	{
		std::string copy = arr[i];
		if (copy == element)
			return i;
	}

	return -1;
}

std::string replace(std::string str, const std::string& from, const std::string& to)
{
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
	return str;
}

std::vector<std::string> split(std::string str, char sep)
{
	std::stringstream stream(str);
	std::string _str;
	std::vector<std::string> res;

	while (std::getline(stream, _str, sep))
	{
		if (_str == "")
			continue;

		res.push_back(_str);
	}

	return res;
}

void drawArrows(size_t start, size_t end)
{
	// Offset due to adding line number on the left
	start += 3;
	end   += 3;

	std::cerr << "\033[31m";
	for (size_t i = 0; i < start; ++i)	  std::cerr << ' ';
	for (size_t i = start; i <= end; ++i)   std::cerr << '^';
	std::cerr << "\033[0m\n";
}

size_t find_nth(std::string haystack, const char& needle, const size_t& nth)
{
	std::string read;
	for (size_t i = 1; i < nth + 1; ++i)
	{
		size_t found = haystack.find(needle);
		read += haystack.substr(0, found + 1);
		haystack.erase(0, found + 1);

		if (i == nth)
			return read.size();
	}

	return -1;
}

const std::string getSourceLine(const std::string& src, const size_t& line)
{
	std::string ret;
	ret += line + ": ";
	for (size_t i = find_nth(src, '\n', line - 1); src[i] != '\n'; ++i)
		ret += src[i];
	ret += '\n';
	return ret;
}

namespace std
{
	// Overload of to_string for TokenType input
	string to_string(TokenType tokenType)
	{
		switch (tokenType)
		{
			case TokenType::TT_KEYWORD:
				return "KEYWORD";

			case TokenType::TT_IDENTIFIER:
				return "IDENTIFIER";
			
			case TokenType::TT_ASSIGN:
				return "ASSIGNMENT";
			
			case TokenType::TT_NUM:
				return "NUMBER";
			
			case TokenType::TT_FLOAT:
				return "FLOAT";
			
			case TokenType::TT_STR:
				return "STRING";
			
			case TokenType::TT_CHAR:
				return "CHAR";
			
			case TokenType::TT_OPEN_PAREN:
				return "OPEN_PAREN";
			
			case TokenType::TT_CLOSE_PAREN:
				return "CLOSE_PAREN";

			case TokenType::TT_COMMA:
				return "COMMA";

			case TokenType::TT_SEMICOLON:
				return "SEMICOLON";
			
			case TokenType::TT_PLUS:
				return "PLUS";
			
			case TokenType::TT_MINUS:
				return "MINUS";

			case TokenType::TT_MULT:
				return "MULT";
			
			case TokenType::TT_DIV:
				return "DIV";
			
			case TokenType::TT_POW:
				return "POW";
			
			case TokenType::TT_OPEN_BRACE:
				return "OPEN_BRACE";
			
			case TokenType::TT_CLOSE_BRACE:
				return "CLOSE_BRACE";
			
			default:
				return "Unknown";
		}
	}
}
