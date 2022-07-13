#include <importer/sourceParser.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include <compiler/parser.h>
#include <compiler/token.h>
#include <util.h>

const TokenType strToType(const std::string& val);

void parseSource(Linker& targetLinker, const std::filesystem::path& file)
{
	// Storing entire source
	std::string src;

	// File stream to read input file
	std::ifstream inputFileStream(file.string());

	if (!inputFileStream.is_open())
	{
		std::cout << "Error: Could not open input file: " << file.string() << '\n';
		exit(-1);
	}
	
	// Get all non-comment tokens into src
	std::string str;
	while(std::getline(inputFileStream, str))
	{
		auto _toks = split(str, ' ');
		
		for (auto tok: _toks)
		{
			tok = replace(tok, "\t", "  ");

			if (tok.starts_with("//"))
				break;

			src += tok + ' ';
		}

		src += '\n';
	}

	// We dont need the input file stream anymore, so close it
	inputFileStream.close();

	const std::vector<std::string>& lines = split(src, '\n');
	for (size_t i = 0; i < lines.size(); ++i)
	{
		std::vector<std::string> toks = split(lines[i], ' ');
		if (toks.size() == 0)
			continue;
		

		if (toks[0] == "@FUNC")
		{
			if (toks.size() != 2)
			{
				std::cerr << "Error: @FUNC expects one argument (name) in library file " << file.string() << " at line " << i << '\n';
				exit(-1);
			}

			const Token name = Token(size_t(-1), TokenType::TT_IDENTIFIER, toks[1], size_t(-1), size_t(-1));

			do
			{
				++i;
				toks = split(lines[i], ' ');
			}
			while (toks.size() == 0);

			if (toks[0] != "@SIGNATURE")
			{
				std::cerr << "Error: Expected @SIGNATURE on line after @FUNC in library file " << file.string() << " at line " << i << '\n';
				exit(-1);
			}
			if (toks.size() < 1)
			{
				std::cerr << "Error: @SIGNATURE expects one or more argument(s) (returnType args...) in library file " << file.string() << " at line " << i << '\n';
				exit(-1);
			}
			const Token& returnType = Token(size_t(-1), strToType(toks[1]), toks[1], size_t(-1), size_t(-1));
			std::vector<Token> argTypes;
			for (size_t j = 2; j < toks.size(); ++j)
				argTypes.push_back(
					Token(size_t(-1), strToType(toks[j]), toks[j], size_t(-1), size_t(-1))
				);

			Function func { name, returnType, argTypes };

			std::stringstream funcCode;
			while (i < lines.size() && toks.size() > 0)
			{
				++i;
				toks = split(lines[i], ' ');
				if (toks.size() == 0) continue;

				if (toks[0] == "@RETURN")
					funcCode << "MOV SP R1\nPOP R1\nRET\n\n";

				else if (toks[0] == "@END")
					break;

				else if (toks[0] == "@CALL")
				{
					if (toks.size() < 2)
					{
						std::cerr << "Error: @CALL needs atleast a function name after it at library file " << file.string() << " at line " << i << '\n';
						exit(-1);
					}

					const Token& name = Token(i, TokenType::TT_IDENTIFIER, toks[1], 6, 6 + toks[1].size());
					std::vector<Token> args;
					size_t start = name.m_end + 1;
					for (size_t j = 2; j < toks.size(); ++j)
					{
						const std::string& argStr = toks[j];
						args.push_back(Token(i, strToType(argStr), argStr, start, start + argStr.size()));
						start += argStr.size() + 1;
					}

					const Function& func2 = targetLinker.getFunction(src, name, args);

					funcCode << "CAL ." << func2.getSignature() << '\n';
					funcCode << "ADD SP SP " << args.size() << '\n';
				}

				else
					funcCode << lines[i] << '\n';
			}
			func.code = funcCode.str();

			targetLinker.addFunction(func);
		}
	}
}

const TokenType strToType(const std::string& val)
{
		 if (val == "void")    return TokenType::TT_VOID;
	else if (val == "int8"
		  || val == "int16"
		  || val == "int32"
		  || val == "int64")   return TokenType::TT_INT;
	else if (val == "uint8"
		  || val == "uint16"
		  || val == "uint32"
		  || val == "uint64")  return TokenType::TT_UINT;
	else if (val == "float32"
		  || val == "float64") return TokenType::TT_FLOAT;
	else if (val == "string")  return TokenType::TT_STRING;
	else if (val == "char")    return TokenType::TT_CHARACTER;
	else                       return (TokenType) -1;
}
