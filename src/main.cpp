#include <cstring>

#include "../include/emu4380.h"
#include <iostream>
#include <fstream>

int main(const int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <bytecode_file> [-m memory_size] [-c cache_type]\n";
        return 1;
    }

    std::string filename;
    unsigned int mem_size = 131072;
    unsigned int cache_type = 0;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Invalid memory configuration. Aborting.\n";
                return 2;
            }

            try {
                mem_size = std::stoul(argv[++i]);
            } catch (std::invalid_argument&) {
                std::cerr << "Invalid memory configuration. Aborting.\n";
                return 2;
            }
        }
        else if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Invalid cache configuration. Aborting.\n";
                return 2;
            }

            try {
                cache_type = std::stoul(argv[++i]);
                if (cache_type > 3) {
                    std::cerr << "Invalid cache configuration. Aborting.\n";
                    return 2;
                }
            } catch (std::invalid_argument&) {
                std::cerr << "Invalid cache configuration. Aborting.\n";
                return 2;
            }
        }
        else if (argv[i][0] == '-') {
            std::cerr << "Usage: " << argv[0] << " <bytecode_file> [-m memory_size] [-c cache_type]\n";
            return 1;
        }
        else {
            if (filename.empty()) {
                filename = argv[i];
            }
        }
    }

    if (filename.empty()) {
        std::cerr << "Usage: " << argv[0] << " <bytecode_file> [-m memory_size] [-c cache_type]\n";
        return 1;
    }

    if (!init_mem(mem_size)) {
        std::cerr << "Failed to initialize memory\n";
        return 1;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "Failed to open input file\n";
        return 1;
    }

    input.seekg(0, std::ios::end);
    const std::streamsize file_size = input.tellg();
    input.seekg(0, std::ios::beg);

    if (file_size > mem_size) {
        std::cout << "INSUFFICIENT MEMORY SPACE\n";
        return 2;
    }
    if (!init_registers(file_size)) {
        std::cerr << "Failed to initialize registers\n";
        return 1;
    }

    input.read(reinterpret_cast<char*>(prog_mem), file_size);

    reg_file[PC] = (prog_mem[3] << 24) | (prog_mem[2] << 16) |
                   (prog_mem[1] << 8) | prog_mem[0];
    mem_cycle_cntr = 0;

    init_cache(cache_type);

    while (true) {
        if (!fetch()) {
            std::cout << "fINVALID INSTRUCTION AT: " << reg_file[PC] - 8 << std::flush;
            return 1;
        }

        if (!decode()) {
            std::cout << "dINVALID INSTRUCTION AT: " << reg_file[PC] - 8 << std::flush;
            return 1;
        }

        if (!execute()) {
            std::cout << "eINVALID INSTRUCTION AT: " << reg_file[PC] - 8 << std::flush;
            return 1;
        }
    }
}
