#include "assembler.h"

#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <limits>
#include <chrono>

static int createBin(const char* name, const Header& header, const std::vector<uint32_t>& code);
static int cleanFile(std::ifstream& file, std::vector<std::string>& fileCleaned, std::unordered_map<std::string, uint32_t>& callMap, size_t& index);
static int produceCode(std::vector<std::string> fileCleaned, std::unordered_map<std::string, uint32_t>& callMap, std::vector<uint32_t>& code, size_t size);
static bool isValidCall(std::string_view call);
static std::string trim(const std::string& s);
bool parseNumber(const std::string& token, uint32_t& out, bool& overflow);

std::vector<std::string_view> split_ws(std::string_view s);

int assembler(const char* filepath, const char* outName)
{
    int state = 0;

    auto start = std::chrono::steady_clock::now();
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "File " << filepath << " not found." << std::endl;
        state = 1;
    }

    std::vector<std::string> fileCleaned;
    std::unordered_map<std::string, uint32_t> callMap;
    size_t size = 0;

    if (!state && cleanFile(file, fileCleaned, callMap, size))
    {
        state = 1;
    }
    
    std::vector<uint32_t> code;
    if (!state && produceCode(fileCleaned, callMap, code, size))
    {
        state = 1;
    }
    file.close();

    Header header = {
        .magic = XBC_MAGIC,
        .isa_version = ASM_VERSION,
        .code_size = static_cast<uint32_t>(size)
    };

    header.code_offset = sizeof(Header) + sizeof(uint32_t);
    
    if (callMap.contains("MAIN"))
    {
        header.entry_point = callMap["MAIN"];
    }
    else
    {
        header.entry_point = 0;
    }
    
    if (!state && createBin(outName, header, code))
    {
        state = 1;
    }

    // 6. Calculate execution time cleanly
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now() - start
    ).count();

    std::cout << "Assembly completed in " << ns / 1000000 << " ms (Exit Code: " << state << ")\n";

    return state;
}

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

static int cleanFile(std::ifstream& file, std::vector<std::string>& fileCleaned,
    std::unordered_map<std::string, uint32_t>& callMap, size_t& index)
{
    std::string line;
    index = 0;

    while (std::getline(file, line))
    {
        // After reading the first line
        if (!line.empty() && (unsigned char)line[0] == 0xEF) {
            // UTF‑8 BOM detected
            if (line.size() >= 3 &&
                (unsigned char)line[0] == 0xEF &&
                (unsigned char)line[1] == 0xBB &&
                (unsigned char)line[2] == 0xBF)
            {
                line = line.substr(3);
            }
        }

        // 1. Remove comments first (everything after #)
        if (auto commentPos = line.find('#'); commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // 2. Trim whitespace from both ends
        line = trim(line);

        // 3. Skip if the line is now empty
        if (line.empty()) continue;

        // 4. Handle Labels (Lines starting with '.')
        if (line[0] == '.')
        {
            // Remove the dot and trim again to get the clean label name
            std::string labelName = trim(line.substr(1));
            if (!labelName.empty()) {
                callMap[labelName] = static_cast<uint32_t>(index);
            }
            continue; // Labels don't count as instructions, so don't increment index
        }

        // 5. It's a valid instruction
        fileCleaned.push_back(line);
        index++;
    }

    // Reset file for future use if needed
    file.clear();
    file.seekg(0, std::ios::beg);

    return 0;
}

static int produceCode(std::vector<std::string> fileCleaned,
    std::unordered_map<std::string, uint32_t>& callMap,
    std::vector<uint32_t>& code,
    size_t size)
{
    int state = 0;

    for (size_t i = 0; i < size; i++)
    {
        auto tokens = split_ws(fileCleaned[i]);

        // Raw hex instruction (full 32-bit) – only if the whole line is hex-like
        uint32_t raw;
        bool overflow = false;

        auto is_hex_like = [](const std::string& s) {
            return !s.empty() &&
                std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isxdigit(c); });
            };

        if (is_hex_like(fileCleaned[i]) && parseNumber(fileCleaned[i], raw, overflow))
        {
            code.push_back(raw);
            continue;
        }
        else if (is_hex_like(fileCleaned[i]) && overflow)
        {
            std::cout << "Number out of range for 32-bit: " << fileCleaned[i] << "\n";
            state = 1;
            continue;
        }

        // Allocate space for 3 operands
        std::vector<uint8_t> instruction(3, 0);

        auto op = bytecodeFromString(tokens[0]);
        if (!op)
        {
            std::cout << "Instruction not found: " << tokens[0] << "\n";
            state = 1;
            continue;
        }

        size_t operandCount = tokens.size() - 1;
        if (operandCount > 3) operandCount = 3;

        for (size_t t = 1; t <= operandCount; t++)
        {
            std::string token = trim(std::string(tokens[t]));

            // Remove leading dot from label operands
            if (!token.empty() && token[0] == '.')
                token.erase(0, 1);

            // Register?
            auto reg = registersFromString(token);
            if (reg)
            {
                instruction[t - 1] = static_cast<uint8_t>(reg.value());
                continue;
            }

            // Label?
            if (callMap.contains(token) && isValidCall(token))
            {
                uint32_t addr = callMap[token];
                instruction[0] = (addr >> 8) & 0xFF;
                instruction[1] = addr & 0xFF;
                break; // CALL/JMP only use 2 bytes
            }

            // Number?
            uint32_t value;
            bool overflowImm = false;
            if (parseNumber(token, value, overflowImm))
            {
                if (value > 0xFF)
                {
                    std::cout << "Immediate out of range (0–255): " << token << "\n";
                    state = 1;
                    continue;
                }

                instruction[t - 1] = static_cast<uint8_t>(value);
                continue;
            }
            else if (overflowImm)
            {
                std::cout << "Number too large for 32-bit: " << token << "\n";
                state = 1;
                continue;
            }

            // Unknown token
            std::cout << "Invalid operand: " << token << "\n";
            state = 1;
        }

        code.push_back(ENCODE_OP(op.value(), instruction[0], instruction[1], instruction[2]));
    }

    return state;
}



static int createBin(const char* name, const Header& header, const std::vector<uint32_t>& code)
{
    std::ofstream file_out(name, std::ios::binary);
    if (!file_out) {
        std::cerr << "Error: cannot open output file: " << name << std::endl;
        return 1;
    }

    // Write header
    file_out.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write instruction count (optional but recommended)
    uint32_t count = static_cast<uint32_t>(code.size());
    file_out.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // Write instruction data
    file_out.write(reinterpret_cast<const char*>(code.data()), code.size() * sizeof(uint32_t));

    if (!file_out.good()) {
        std::cerr << "Error: failed while writing binary file." << std::endl;
        return 1;
    }

    file_out.close();
    std::cout << "Binary file created successfully!" << std::endl;
    return 0;
}


static bool isValidCall(std::string_view call)
{
    if (call.empty())
        return false;

    // Remove leading dot if present
    if (call[0] == '.')
        call.remove_prefix(1);

    if (call.empty())
        return false;

    // --- SYSTEM RESERVED LABELS ---
    static const std::unordered_set<std::string_view> reserved = {
        "DATA", "TEXT", "HEAD", "MAIN", "SELECT",
        "BSS", "STACK", "HEAP", "ENTRY"
    };

    if (reserved.contains(call))
        return false;

    // --- REGISTERS ---
    if (registersFromString(call))
        return false;

    // --- NUMERIC START (decimal, hex, binary) ---
    if (std::isdigit(call[0]))
        return false;

    // --- HEX LIKE FF or 0xFF ---
    bool allHex = std::all_of(call.begin(), call.end(),
        [](unsigned char c) { return std::isxdigit(c); });

    if (allHex)
        return false;

    // --- BINARY LIKE 0101b ---
    if (call.back() == 'b' || call.back() == 'B')
    {
        bool all01 = std::all_of(call.begin(), call.end() - 1,
            [](char c) { return c == '0' || c == '1'; });

        if (all01)
            return false;
    }

    // --- VALID LABEL RULES ---
    // Must start with letter or underscore
    if (!std::isalpha(call[0]) && call[0] != '_')
        return false;

    // Remaining chars must be alphanumeric or underscore
    for (char c : call)
    {
        if (!std::isalnum((unsigned char)c) && c != '_')
            return false;
    }

    return true;
}


std::vector<std::string_view> split_ws(std::string_view s)
{
    std::vector<std::string_view> out;

    while (!s.empty()) {
        size_t start = s.find_first_not_of(' ');
        if (start == std::string_view::npos) break;
        s.remove_prefix(start);

        size_t end = s.find(' ');
        out.push_back(s.substr(0, end));

        if (end == std::string_view::npos) break;
        s.remove_prefix(end);
    }

    return out;
}

bool parseNumber(const std::string& token, uint32_t& out, bool& overflow)
{
    std::string s = token;
    unsigned long long tmp = 0;
    overflow = false;

    // --- HEX ---
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
    {
        try {
            tmp = std::stoull(s.substr(2), nullptr, 16);
        }
        catch (const std::out_of_range&) { overflow = true; return false; }
        catch (...) { return false; }

        if (tmp > std::numeric_limits<uint32_t>::max()) { overflow = true; return false; }
        out = static_cast<uint32_t>(tmp);
        return true;
    }

    // --- BINARY ---
    if (!s.empty() && (s.back() == 'b' || s.back() == 'B'))
    {
        std::string bin = s.substr(0, s.size() - 1);
        if (!std::all_of(bin.begin(), bin.end(), [](char c) { return c == '0' || c == '1'; }))
            return false;

        tmp = 0;
        for (char c : bin)
        {
            tmp = (tmp << 1) | (c - '0');
            if (tmp > std::numeric_limits<uint32_t>::max()) { overflow = true; return false; }
        }
        out = static_cast<uint32_t>(tmp);
        return true;
    }

    // --- DECIMAL ---
    if (std::all_of(s.begin(), s.end(), ::isdigit))
    {
        try {
            tmp = std::stoull(s, nullptr, 10);
        }
        catch (const std::out_of_range&) { overflow = true; return false; }
        catch (...) { return false; }

        if (tmp > std::numeric_limits<uint32_t>::max()) { overflow = true; return false; }
        out = static_cast<uint32_t>(tmp);
        return true;
    }

    return false;
}

bool isPureHexLine(const std::string& s)
{
    if (s.empty()) return false;

    // No prefixes, no spaces, no letters beyond hex
    return std::all_of(s.begin(), s.end(),
        [](unsigned char c) { return std::isxdigit(c); });
}

bool parseHex32(const std::string& s, uint32_t& out)
{
    try {
        unsigned long long tmp = std::stoull(s, nullptr, 16);
        if (tmp > 0xFFFFFFFFULL) return false;
        out = static_cast<uint32_t>(tmp);
        return true;
    }
    catch (...) {
        return false;
    }
}
