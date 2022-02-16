#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

// WHY IS THIS NEEDED
enum class TokenType: uintmax_t;

#include <compiler/parser.h>

int indexOf(char* arr[], std::string element, int size);
std::vector<std::string> split(std::string str, char sep);

namespace std
{
	string to_string(TokenType tokenType);
}

void drawArrows(size_t start, size_t end);

size_t find_nth(std::string haystack, const char& needle, const size_t& nth);

#endif // UTIL_H
