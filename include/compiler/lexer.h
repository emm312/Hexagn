#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

#include <compiler/token.h>

std::vector<Token> tokenize(std::string source);

#endif // LEXER_H
