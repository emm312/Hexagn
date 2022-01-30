#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>

int indexOf(char* argv[], std::string element, int size);
std::vector<std::string> split(std::string str, char sep);

// alt's version of function
int intIndexOf(std::string str, std::string prefix);
bool doesStartsWith(std::string str, std::string prefix);

#endif // UTIL_H
