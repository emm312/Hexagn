#include <string>

#include <util.h>

int indexOf(char* argv[], std::string element, int size)
{
    for (int i = 0; i < size; ++i)
    {
        std::string copy = argv[i];
        if (copy == element)
            return i;
    }

    return -1;
}
