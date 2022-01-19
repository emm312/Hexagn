#include <iostream>

#include <util.h>
#include <interpreter/interpreter.h>

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        std::cerr << "Invalid number of arguments\n" << "Usage: hexagn file.hxgn or hexagn file.hxgn -urcl file.urcl\n";
        return -1;
    }

    int index;                  // Reused index variable for arguments

    std::string inputFileName = argv[1];
    std::string outputFileName;

    if ((index = indexOf(argv, "-urcl", argc)) != -1)
        try
        {
            outputFileName = argv[index + 1];
        }
        catch (const std::out_of_range& e)
        {
            std::cerr << "No output filename provided\n";
            return -1;
        }
    
    interpreter(inputFileName, outputFileName);
}