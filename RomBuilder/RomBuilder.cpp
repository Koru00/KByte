#include "RomBuilder.h"
#include <windows.h>

int main(int argc, char* argv[])
{
    std::string output = "a.out";
    std::string input;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-o")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "-o requires argument\n";
                return 1;
            }

            output = argv[++i]; // consume next argument
        }
        else if (arg[0] == '-')
        {
            std::cerr << "Unknown flag: " << arg << "\n";
            return 1;
        }
        else
        {
            input = arg;
        }
    }

    std::cout << "Input: " << input << "\n";
    std::cout << "Output: " << output << "\n";

    if (assembler("..\\..\\..\\..\\examples\\test-asm.xba", "..\\..\\..\\..\\examples\\test-asm.xbc"))
    {
        return 1;
    }

    if (Interpreter("..\\..\\..\\..\\examples\\test-asm.xbc"))
    {
        return 1;
    }

    return 0;
}
