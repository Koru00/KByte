#include "RomBuilder.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <vector>
#include <string>
#include <string_view>

// 1. Group state into a struct for deterministic behavior
struct Config
{
    std::string output = "a.xbc";
    std::string run_file = "";
    std::vector<std::string> inputs;
    bool run = false;
};

void print_help(std::string_view program_name) 
{
    std::cout << "Usage: " << program_name << " [options] <input_files...>\n"
              << "Options:\n"
              << "  -o, --output <file>    Specify output executable name (default: a.xbc)\n"
              << "  -r, --run <file>       Specify a file to run immediately\n"
              << "  -h, --help             Display this help message\n";
}

int main(int argc, char* argv[])
{
    Config config;
    std::string_view program_name = argv[0];

    // 2. Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-o" || arg == "--output") {
            if (++i >= argc) {
                std::cerr << "Error: '" << arg << "' requires a filename argument.\n";
                return 1;
            }
            config.output = argv[i];
        } 
        else if (arg == "-r" || arg == "--run") {
            if (++i >= argc) {
                std::cerr << "Error: '" << arg << "' requires a filename argument.\n";
                return 1;
            }
            config.run = true;
            config.run_file = argv[i];
        } 
        else if (arg == "-h" || arg == "--help") {
            print_help(program_name);
            return 0;
        } 
        // 3. Catch misspelled or unknown flags
        else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Error: Unknown flag '" << arg << "'.\n"
                      << "Run '" << program_name << " --help' for usage.\n";
            return 1;
        } 
        // 4. GCC-style positional inputs (anything not starting with '-')
        else {
            config.inputs.emplace_back(arg);
        }
    }

    // 5. Deterministic validation (e.g., GCC fails if no inputs are provided)
    if (config.inputs.empty() && !config.run) {
        std::cerr << "Error: No input files provided.\n";
        return 1;
    }

    if (!config.run)
    {
    if (assembler(config.inputs[0].c_str(), config.output.c_str()))
    {
        return 1;
    }
    }
    else
    {
    if (Interpreter(config.run_file.c_str()))
    {
        return 1;
    }
    }

    return 0;
}
