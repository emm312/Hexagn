#include <string>
#include <vector>
#include <sstream>

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


// alt's version of function
int intIndexOf(std::string str, std::string prefix) {
    for (int i = 0; i < str.length(); ++i) {
        if (str.substr(i, prefix.length()) == prefix) {
            return i;
        }
    }
    return -1;
}

bool doesStartsWith(std::string str, std::string prefix) {
    return str.substr(0, intIndexOf(str, " ")) == prefix;
}