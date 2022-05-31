#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>

#include <compiler/compiler.h>
#include <util.h>
#include <compiler/lexer.h>
#include <compiler/parser.h>

void compiler(const std::string& inputFileName, const std::string& outputFileName, const bool& debugSymbols)
{
	// Storing entire source
	std::string src;

	// File stream to read input file
	std::ifstream inputFileStream(inputFileName);
	
	// Get all non-comment tokens into src
	std::string str;
	while(std::getline(inputFileStream, str))
	{
		auto _toks = split(str, ' ');
		
		for (auto tok: _toks)
		{
			if (tok.starts_with("//"))
				break;
			
			src += replace(tok, "\t", "") + ' ';
		}

		src += '\n';
	}

	// We dont need the input file stream anymore, so close it
	inputFileStream.close();
	auto toks = tokenize(src);
	// for (const auto& tok: toks)
	// 	 std::cout << tok.toString() + '\n';
	std::ofstream(outputFileName) << compile(toks, debugSymbols).str();
}
