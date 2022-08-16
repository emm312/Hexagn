#include <importer/importHelper.h>

#include <iostream>

#include <string>
#include <vector>
#include <algorithm>
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

// Protection against importing same file twice
static std::vector<std::filesystem::path> imported;

void importLibrary(Linker& targetLinker, const std::string& libName)
{
	const std::vector<std::string>& vec = split(libName, ':');

	if (vec.size() > 2)
	{
		std::cerr << "Error: Malformed file import " << libName << '\n';
		exit(-1);
	}

	std::string libPath = replace(vec[0], ".", "/");
	if (vec.size() == 2)
		libPath += '/' + vec[1];

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

	// We add it at the start because
	// 1) If theres an error, the entire program quits anyways
	// 2) Prevent file from importing itself

	if (vec.size() == 1)
		for (const auto& file: std::filesystem::directory_iterator(libDir))
		{
			if (file.is_directory()) continue;

			const std::filesystem::path filePath = file;
			if (std::find(imported.begin(), imported.end(), filePath) != imported.end()) return;
			imported.push_back(filePath);

			if (filePath.extension() == ".urcl")
				parseURCLSource(targetLinker, file);
			else if (filePath.extension() == ".hxgn")
				parseHexagnSource(targetLinker, file);
			else
			{
				std::cerr << "Error: Unrecognized file format for library file: " << file << '\n';
				exit(-1);
			}
		}
	else
	{
		if (std::find(imported.begin(), imported.end(), libDir) != imported.end()) return;
		imported.push_back(libDir);

		if (libDir.extension() == ".urcl")
			parseURCLSource(targetLinker, libDir);
		else if (libDir.extension() == ".hxgn")
			parseHexagnSource(targetLinker, libDir);
		else
		{
			std::cerr << "Error: Unrecognized file format for library file: " << libDir << '\n';
			exit(-1);
		}
	}
}

void addPath(const std::string& path)
{
	const std::filesystem::path _path(path);
	libPaths.push_back(_path);
}
