#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <sstream>
#include <string>

#include <compiler/token.h>

extern std::string glob_src;
extern const std::string KEYWORDS[];

std::stringstream compile(std::vector<Token> tokens);

#endif // PARSER_H
