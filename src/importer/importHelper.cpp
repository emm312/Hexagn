#include <importer/importHelper.h>

#include <iostream>

#include <string>
#include <vector>
#include <filesystem>

#include <importer/sourceParser.h>
#include <util.h>

// This holds the paths in which the compiler which search for libraries
static std::vector<std::filesystem::path> libPaths = {
#ifdef _WIN32
	"C:\\Program Files (x86)\\hexagn\\hexagn-stdlib",
#else
	"/usr/lib/hexagn/hexagn-stdlib/",
#endif

	// For dev
	"./hexagn-stdlib/"
};

void importLibrary(Linker& targetLinker, const std::string& libName)
{
	const std::string libPath = replace(libName, ".", "/");
	std::filesystem::path libDir;
	bool libFound = false;
	for (const auto& path: libPaths)
	{
		const std::filesystem::path libDirPath = path / libPath;
		if (std::filesystem::exists(libDirPath))
		{
			libFound = true;
			libDir = libDirPath;
			break;
		}
	}

	if (!libFound)
	{
		std::cerr << "Error: Library " << libName << " does not exist\n";
		exit(-1);
	}

	for (const auto& file: std::filesystem::directory_iterator(libDir))
	{
		if (file.is_directory()) continue;
		parseSource(targetLinker, file);
	}
}
