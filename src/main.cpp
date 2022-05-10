#include <iostream>
#include <string>

#include <util.h>
#include <compiler/compiler.h>

int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		std::cerr << "Invalid number of arguments\n" << "Usage: hexagn file.hxgn or hexagn file.hxgn -o file.urcl\n";
		return -1;
	}

	std::string inputFileName;
	std::string outputFileName = "out.urcl";
	bool debugSymbols = false;

	// Reused index variable for arguments
	int index = 1;

	while (index < argc)
	{
		const std::string val = argv[index];

		if (val == "-o")
		{
			index++;

			if (index >= argc)
			{
				std::cerr << "\033[1m" << argv[0] << ": \033[31merror: \033[0mMissing filename after '-o'\n";
				return -1;
			}
			outputFileName = argv[index];
		}
		else if (val == "-g")
			debugSymbols = true;
		else
			inputFileName = val;

		index++;
	}

	if (inputFileName.empty())
	{
		std::cerr << "\033[1m" << argv[0] << ": \033[31mfatal error: \033[0mNo input filename provided\nCompilation terminated.\n";
		return -1;
	}
	
	compiler(inputFileName, outputFileName, debugSymbols);
}
