#ifndef IMPORTHELPER_H
#define IMPORTHELPER_H

#include <string>
#include <compiler/linker.h>

void importLibrary(Linker& targetLinker, const std::string& libName);

#endif
