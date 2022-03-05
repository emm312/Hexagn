#include <iostream>
#include <string>

#include <util.h>
#include <compiler/compiler.h>

int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		std::cerr << "Invalid number of arguments\n" << "Usage: hexagn file.hxgn or hexagn file.hxgn -u file.urcl\n";
		return -1;
	}

	// Reused index variable for arguments
	int index;

	std::string inputFileName = argv[1];
	std::string outputFileName;

	if ((index = indexOf(argv, "-o", argc)) != -1)
	{
		if (index == argc - 1)
		{
			std::cerr << "No output filename provided\n";
			return -1;
		}

		std::string fileName = argv[index + 1];
		
		if (fileName.starts_with('-'))
		{
			std::cerr << "Expected output filename, instead got compiler flag\n";
			return -1;
		}

		outputFileName = fileName;
	}
	else
		outputFileName = "out.urcl";
	
	compiler(inputFileName, outputFileName);
}