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

    // File stream to read input file
    std::ifstream inputFileStream(inputFileName);

    // File stream to write to output file
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

    // We dont need the input file stream anymore, so close it
    inputFileStream.close();

    auto toks = split(src, ' ');

    // Temporary print thing
    for (auto tok: toks)
        std::cout << tok << ' ';
    std::cout << '\n';
}
