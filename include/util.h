#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

#include <compiler/ast/nodes.h>

int indexOf(char* arr[], std::string element, int size);
std::string replace(std::string str, const std::string& from, const std::string& to);
std::vector<std::string> split(std::string str, char sep);
void drawArrows(size_t start, size_t end, const size_t& lineno);
size_t find_nth(std::string haystack, const char& needle, const size_t& nth);
const std::string getSourceLine(const std::string& src, const size_t& line);

const bool isIntegerDataType(const TypeNode* const type);
const bool isFloatDataType(const TypeNode* const type);
const bool isNumber(const TypeNode* const type);

namespace std
{
	string to_string(TokenType tokenType);
}

#endif // UTIL_H
