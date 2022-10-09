#include <iostream>
#include <string>

#include <util.h>
#include <compiler/compiler.h>
#include <importer/importHelper.h>

int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		std::cerr << "Invalid number of arguments\nUsage: hexagn file.hxgn or hexagn file.hxgn -o file.urcl\n";
		return -1;
	}

	std::string inputFileName;
	std::string outputFileName = "out.urcl";
	bool debugSymbols = false;
	bool emitEntryPoint = true;

	// Reused index variable for arguments
	int index = 1;
	bool allow_flags = true;

	while (index < argc)
	{
		const std::string val = argv[index];
		if (val[0] == '-' && allow_flags) {
			size_t size = val.size();
			if (size == 1) {
				std::cerr << "\033[1m" << argv[0] << ": \033[31merror: \033[0mStray '-' in arguments\n";
				return -1;
			}
			if (val[1] != '-') {
				for (size_t i = 1; i < size; i++) {
					bool found = false;
					if (val[i] == 'g') {
						debugSymbols = true;
						found = true;
					}
					// add more flags here

					// allow flags with arguments at end of flag cluster
					if (i == val.size() - 1) {
						if (val[i] == 'o') {
							index++;
							if (index >= argc) {
								std::cerr << "\033[1m" << argv[0] << ": \033[31merror: \033[0mMissing filename after '-o'\n";
								return -1;
							}
							outputFileName = argv[index];
							found = true;
						}
						else if (val[i] == 'L') {
							index++;
							if (index >= argc) {
								std::cerr << "\033[1m" << argv[0] << ": \033[31merror: \033[0mMissing path after '-L'\n";
								return -1;
							}
							addPath(argv[index]);
							found = true;
						}
					}
					else {
						if (val[i] == 'o' || val[i] == 'L') {
							std::cerr << "\033[1m" << argv[0] <<
								": \033[31merror: \033[0mFlag with argument ('-" << val[i] << "') not at end of argument cluster ('" << val << "')\n";
							return -1;
						}
					}
					if (!found) {
						std::cerr << "\033[1m" << argv[0] << ": \033[31merror: \033[0mInvalid flag '-" << val[i] << "'\n";
						return -1;
					}
				}
			}
			else {
				if (size == 2) {
					allow_flags = false;
				}
				else if (val == "--no-main") {
					emitEntryPoint = false;
				}
				else {
					std::cerr << "\033[1m" << argv[0] << ": \033[31merror: \033[0mInvalid flag '" << val << "'\n";
					return -1;
				}
			}
		}
		else {
			if (!inputFileName.empty()) {
				std::cerr << "\033[1m" << argv[0] << ": \033[31merror: \033[0mToo many input files\n";
				return -1;
			}
			inputFileName = val;
		}

		index++;
	}

	if (inputFileName.empty())
	{
		std::cerr << "\033[1m" << argv[0] << ": \033[31mfatal error: \033[0mNo input filename provided\nCompilation terminated.\n";
		return -1;
	}
	
	compiler(inputFileName, outputFileName, debugSymbols, emitEntryPoint);
}
