#include <string>
#include <vector>
#include <sstream>

#include <util.h>

int indexOf(char* arr[], std::string element, int size)
{
    for (int i = 0; i < size; ++i)
    {
        std::string copy = arr[i];
        if (copy == element)
            return i;
    }

    return -1;
}

std::vector<std::string> split(std::string str, char sep)
{
    std::stringstream stream(str);
    std::string _str;
    std::vector<std::string> res;

    while (std::getline(stream, _str, sep))
    {
        if (_str == "")
            continue;

        res.push_back(_str);
    }

    return res;
}
