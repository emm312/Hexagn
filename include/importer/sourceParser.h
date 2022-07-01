#ifndef SOURCE_PARSER_H
#define SOURCE_PARSER_H

#include <filesystem>

#include <compiler/linker.h>

void parseSource(Linker& targetLinker, const std::filesystem::path& file);

#endif // SOURCE_PARSER_H
