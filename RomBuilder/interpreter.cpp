#include "interpreter.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <memory>

#include "vm.h"
#include "assembler.h"

#define MEMORY_SIZE 262144
#define STACK_SIZE 131072

int Interpreter(const char* filepath)
{
    // 1. Open file and jump to the end (ate) to get total file size immediately
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Error: cannot open file: " << filepath << '\n';
        return 1;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg); // Rewind to the beginning

    // 2. Read and validate header safely
    Header header{};
    if (fileSize < sizeof(Header) || !file.read(reinterpret_cast<char*>(&header), sizeof(header))) {
        std::cerr << "Error: file too small or corrupted\n";
        return 1;
    }

    if (header.magic != XBC_MAGIC) {
        std::cerr << "Error: not compatible file\n";
        return 1;
    }

    // 3. Read Bytecode (Optimized Bulk Read)
    file.seekg(static_cast<std::streamoff>(header.code_offset), std::ios::beg);
    std::streamsize codeBytes = fileSize - header.code_offset;

    // Ensure the section size makes sense (must be divisible by 4 bytes / 32 bits)
    if (codeBytes <= 0 || codeBytes % sizeof(uint32_t) != 0) {
        std::cerr << "Error: invalid bytecode section size\n";
        return 1;
    }

    // Resize vector and read everything in one single fast operation
    std::vector<uint32_t> code(codeBytes / sizeof(uint32_t));
    if (!file.read(reinterpret_cast<char*>(code.data()), codeBytes)) {
        std::cerr << "Error: failed to read bytecode\n";
        return 1;
    }

    file.close();

    // 4. Initialize VM
    auto vm_ptr = std::make_unique<VM>(MEMORY_SIZE * sizeof(int32_t), STACK_SIZE * sizeof(int32_t), code.size() * sizeof(uint32_t));
    VM& v = *vm_ptr; // Use 'v' consistently from now on

    // Reset VM state
    v.ex = 0;
    clearAllFlags(v);
    v.sp = 0;

    // Assign code to VM
    v.code = std::move(code);
    v.codeSize = v.code.size();
    v.ip = header.entry_point;
    v.state = VMState::Running;

    size_t time_int = 0;

    auto start = std::chrono::steady_clock::now();
    
    // 5. Interpreter loop
    while (true)
    {
        if (v.state == VMState::Stopped)
        {
            break;
        }
        if (v.ip >= v.code.size()) 
        {
            std::cerr << "Runtime Error: instruction pointer out of range\n";
            break;
        }

        uint32_t instr = v.code[v.ip];

        execute(
            v, // Passed the dereferenced VM
            static_cast<Bytecode>(OP(instr)),
            static_cast<uint8_t>(A(instr)),
            static_cast<uint8_t>(B(instr)),
            static_cast<uint8_t>(C(instr))
        );

        time_int++;
    }

    // 6. Calculate execution time cleanly
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - start
    ).count();

    std::cout << "\n\nTook " << ns / 1000000 << "ms for " << time_int << " instruction" << "\n";

    return 0;
}