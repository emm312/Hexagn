#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>

#include <interpreter/interpreter.h>
#include <util.h>

void interpreter(std::string inputFileName, std::string outputFileName)
{
    std::string src;                                            // Storing entire source

    std::ifstream inputFileStream(inputFileName);               // File stream to read input file
    std::ifstream* outputFileStream = nullptr;                  // File stream to read output file
                                                                // set to null since we may not always have an output file
    
    if (!outputFileName.empty())
        outputFileStream = new std::ifstream(outputFileName);   // Since outputFileName string is not empty, we can have a file stream to it
    
    std::vector<std::string> (*split)(std::string, char) = [](std::string str, char sep)
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
    };
    
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

    // toks now is a vector of all non-comment tokens
    auto toks = split(src, ' ');
    
    for (auto tok: toks)
        std::cout << tok << ' ';
    
    std::cout << '\n';
}
