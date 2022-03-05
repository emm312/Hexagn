#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

#include <compiler/lexer.h>

int indexOf(char* arr[], std::string element, int size);
std::vector<std::string> split(std::string str, char sep);
void drawArrows(size_t start, size_t end);
size_t find_nth(std::string haystack, const char& needle, const size_t& nth);
void printLine(std::string src, size_t line);

namespace std
{
	string to_string(TokenType tokenType);
}

#endif // UTIL_H
