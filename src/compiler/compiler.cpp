#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>

#include <compiler/compiler.h>
#include <util.h>

std::vector<std::string> split(std::string str, char sep);

void compiler(std::string inputFileName, std::string outputFileName)
{
    std::string src;                                            // Storing entire source

    std::ifstream inputFileStream(inputFileName);               // File stream to read input file
    std::ofstream outputFileStream(outputFileName);
    
    // Get all non-comment tokens into src
    std::string str;
    while(std::getline(inputFileStream, str))
    {
        auto _toks = split(str, ' ');
        
        for (auto tok: _toks)
        {
            if (tok.starts_with("//"))
                break;
            
            src += tok + ' ';
        }
    }

    std::cout << src << '\n';
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
