#ifndef IMPORT_HELPER_H
#define IMPORT_HELPER_H

#include <string>
#include <compiler/linker.h>

void importLibrary(Linker& targetLinker, const std::string& libName);

#endif // IMPORT_HELPER_H
