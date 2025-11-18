#include "../include/emu.h"
#include "../include/cache.h"
#include <iostream>
#include <cstdlib>
#include <memory>

unsigned int reg_file[22] = {0};
unsigned int cntrl_regs[5] = {0};
unsigned char *prog_mem = nullptr;
unsigned int mem_cycle_cntr = 0;
unsigned int prog_mem_size = 0;
bool test_mode = false;
bool memStream = false;

// 0 = no cache, 1 = direct mapped, 2 = fully associative, 3 = 2-way set associative
static unsigned int cache_type = 0;
static std::unique_ptr<Cache> cache = nullptr;
static std::unique_ptr<MemoryInterface> memory_interface = nullptr;

void cleanupAndExit() {
    if (prog_mem != nullptr) {
        delete[] prog_mem;
        prog_mem = nullptr;
    }
    std::cout << "Execution completed. Total memory cycles: " << mem_cycle_cntr << std::endl;

    if (test_mode) {
        return;
    }
    std::exit(EXIT_SUCCESS);
}

bool init_registers(const unsigned int code_section) {
    for (int i = 0; i < PC; i++) {
        reg_file[i] = 0;
    }

    for (int i = 0; i < 5; i++) {
        cntrl_regs[i] = 0;
    }
    memStream = false;

    reg_file[PC] = 0;
    reg_file[SL] = code_section + 1;
    reg_file[SB] = prog_mem_size;
    reg_file[SP] = reg_file[SB];
    reg_file[FP] = 0;
    reg_file[HP] = reg_file[SL];


    return true;
}

bool validate_stack_pointer() {
    if (reg_file[SP] < reg_file[SL] || reg_file[SP] > reg_file[SB]) {
        return false;
    }

    return true;
}

bool init_mem(const unsigned int size) {
    if (prog_mem != nullptr) delete[] prog_mem;

    prog_mem = new(std::nothrow) unsigned char[size];

    if (prog_mem == nullptr) return false;

    prog_mem_size = size;
    mem_cycle_cntr = 0;

    return true;
}

unsigned char readByte(const unsigned int address) {
    if (address >= prog_mem_size) {
        return 0;
    }
    if (!cache) {
        if (memStream) {
            mem_cycle_cntr += 2;
        } else {
            mem_cycle_cntr += 8;
            memStream = true;
        }
        return prog_mem[address];
    }
    const CacheResult result = cache->readByte(address);
    mem_cycle_cntr += result.getCycles();

    return cache->getCachedByte(address);
}

unsigned int readWord(const unsigned int address) {
    if (address + 3 >= prog_mem_size) {
        return 0;
    }

    if (!cache) {
        if (memStream) {
            mem_cycle_cntr += 2;
        } else {
            mem_cycle_cntr += 8;
            memStream = true;
        }
        return (prog_mem[address + 3] << 24) |
            (prog_mem[address + 2] << 16) |
            (prog_mem[address + 1] << 8) |
            prog_mem[address];
    }
    const CacheResult result = cache->readWord(address);
    mem_cycle_cntr += result.getCycles();
    return cache->getCachedWord(address);
}

void writeByte(const unsigned int address, const unsigned char byte) {
    if (address >= prog_mem_size) {
        return;
    }

    if (!cache) {
        if (memStream) {
            mem_cycle_cntr += 2;
        } else {
            mem_cycle_cntr += 8;
            memStream = true;
        }
        prog_mem[address] = byte;
        return;
    }
    const CacheResult result = cache->writeByte(address, byte);
    mem_cycle_cntr += result.getCycles();
}

void writeWord(const unsigned int address, const unsigned int word) {
    if (address + 3 >= prog_mem_size) {
        return;
    }

    if (!cache) {
        if (memStream) {
            mem_cycle_cntr += 2;
        } else {
            mem_cycle_cntr += 8;
            memStream = true;
        }
        prog_mem[address] = word & 0xFF;
        prog_mem[address + 1] = (word >> 8) & 0xFF;
        prog_mem[address + 2] = (word >> 16) & 0xFF;
        prog_mem[address + 3] = (word >> 24) & 0xFF;
        return;
    }
    const CacheResult result = cache->writeWord(address, word);
    mem_cycle_cntr += result.getCycles();
}

void init_cache(const unsigned int cacheType) {
    cache_type = cacheType;

    if (cache_type > 0 && prog_mem != nullptr) {
        memory_interface = std::make_unique<SystemMemory>(prog_mem, prog_mem_size);
        cache = CacheFactory::createCache(cache_type, memory_interface.get());
    } else {
        cache = nullptr;
        memory_interface = nullptr;
    }
}

bool fetch() {
    if (reg_file[PC] > prog_mem_size - 8 || prog_mem_size < 8) {
        return false;
    }

    const unsigned int firstWord = readWord(reg_file[PC]);
    const unsigned int secondWord = readWord(reg_file[PC] + 4);
    memStream = false;

    cntrl_regs[OPERATION] = firstWord & 0xFF;
    cntrl_regs[OPERAND_1] = (firstWord >> 8) & 0xFF;
    cntrl_regs[OPERAND_2] = (firstWord >> 16) & 0xFF;
    cntrl_regs[OPERAND_3] = (firstWord >> 24) & 0xFF;

    cntrl_regs[IMMEDIATE] = (secondWord & 0xFF000000) |
                            (secondWord & 0x00FF0000) |
                            (secondWord & 0x0000FF00) |
                            (secondWord & 0x000000FF);

    reg_file[PC] += 8;
    return true;
}

bool decode() {
    switch (cntrl_regs[OPERATION]) {
        case JMP:
            if (cntrl_regs[IMMEDIATE] >= prog_mem_size) {
                return false;
            }
            break;

        case JMR:
            if (cntrl_regs[OPERAND_1] >= 22) {
                return false;
            }
            break;

        case BNZ:
        case BGT:
        case BLT:
        case BRZ:
            if (cntrl_regs[OPERAND_1] >= 22 || cntrl_regs[IMMEDIATE] >= prog_mem_size) {
                return false;
            }
            break;

        case MOV:
            if (cntrl_regs[OPERAND_1] >= 22 || cntrl_regs[OPERAND_2] >= 22) {
                return false;
            }
            break;

        case MOVI:
            if (cntrl_regs[OPERAND_1] >= 22) {
                return false;
            }
            break;

        case LDA:
        case STR:
        case LDR:
        case STB:
        case LDB:
            if (cntrl_regs[OPERAND_1] >= 22) {
                return false;
            }
            break;

        case ISTR:
        case ILDR:
        case ISTB:
        case ILDB:
            if (cntrl_regs[OPERAND_1] >= 22 || cntrl_regs[OPERAND_2] >= 22) {
                return false;
            }
            break;

        case ADD:
        case SUB:
        case MUL:
        case DIV:
        case SDIV:
            if (cntrl_regs[OPERAND_1] >= 22 ||
                cntrl_regs[OPERAND_2] >= 22 ||
                cntrl_regs[OPERAND_3] >= 22) {
                return false;
            }
            break;

        case DIVI:
            if (cntrl_regs[IMMEDIATE] == 0) {
                return false;
            }
        case ADDI:
        case SUBI:
        case MULI:
            if (cntrl_regs[OPERAND_1] >= 22 || cntrl_regs[OPERAND_2] >= 22) {
                return false;
            }
            break;

        case AND:
        case OR:
            if (cntrl_regs[OPERAND_1] >= 22 ||
                cntrl_regs[OPERAND_2] >= 22 ||
                cntrl_regs[OPERAND_3] >= 22) {
                return false;
            }
            break;

        case CMP:
            if (cntrl_regs[OPERAND_1] >= 22 ||
                cntrl_regs[OPERAND_2] >= 22 ||
                cntrl_regs[OPERAND_3] >= 22)
                {
                return false;
            }
            break;

        case CMPI:
            if (cntrl_regs[OPERAND_1] >= 22 || cntrl_regs[OPERAND_2] >= 22) {
                return false;
            }
            break;

        case TRP:
            switch (cntrl_regs[IMMEDIATE]) {
                case HALT:
                case INT_OUT:
                case INT_IN:
                case CHAR_OUT:
                case CHAR_IN:
                case STRING_OUT:
                case STRING_IN:
                case PRINT_REG:
                    break;
                default:
                    return false;
            }
            break;

        case ALCI:
            if (cntrl_regs[OPERAND_1] >= 22) {
                return false;
            }
            if (cntrl_regs[IMMEDIATE] + 3 >= prog_mem_size) {
                return false;
            }
            break;

        case ALLC:
            if (cntrl_regs[OPERAND_1] >= 22) {
                return false;
            }
            break;

        case IALLC:
            if (cntrl_regs[OPERAND_1] >= 22 ||
                cntrl_regs[OPERAND_2] >= 22) {
                return false;
            }
            break;

        case PSHR:
        case PSHB:
        case POPR:
        case POPB:
            if (cntrl_regs[OPERAND_1] >= 22) {
                return false;
            }
            break;

        case CALL:
            if (cntrl_regs[IMMEDIATE] >= prog_mem_size) {
                return false;
            }
            break;

        case RET:
            break;

        default:
            return false;
    }

    return true;
}

bool execute() {
    switch (cntrl_regs[OPERATION]) {
        case JMP:
            if (cntrl_regs[IMMEDIATE] >= prog_mem_size) {
                return false;
            }
            reg_file[PC] = cntrl_regs[IMMEDIATE];
            break;

        case JMR:
            reg_file[PC] = reg_file[cntrl_regs[OPERAND_1]];
            break;

        case BNZ:
            if (reg_file[cntrl_regs[OPERAND_1]] != 0) {
                reg_file[PC] = cntrl_regs[IMMEDIATE];
            }
            break;

        case BGT:
            if (static_cast<int>(reg_file[cntrl_regs[OPERAND_1]]) > 0) {
                reg_file[PC] = cntrl_regs[IMMEDIATE];
            }
            break;

        case BLT:
            if (static_cast<int>(reg_file[cntrl_regs[OPERAND_1]]) < 0) {
                reg_file[PC] = cntrl_regs[IMMEDIATE];
            }
            break;

        case BRZ:
            if (reg_file[cntrl_regs[OPERAND_1]] == 0) {
                reg_file[PC] = cntrl_regs[IMMEDIATE];
            }
            break;

        case MOV:
            reg_file[cntrl_regs[OPERAND_1]] = reg_file[cntrl_regs[OPERAND_2]];
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case MOVI:
            reg_file[cntrl_regs[OPERAND_1]] = cntrl_regs[IMMEDIATE];
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case LDA:
            reg_file[cntrl_regs[OPERAND_1]] = cntrl_regs[IMMEDIATE];
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case STR:
            if (cntrl_regs[IMMEDIATE] + 3 >= prog_mem_size) {
                return false;
            }
            writeWord(cntrl_regs[IMMEDIATE], reg_file[cntrl_regs[OPERAND_1]]);
            memStream = false;
            break;

        case LDR:
            if (cntrl_regs[IMMEDIATE] + 3 >= prog_mem_size) {
                return false;
            }
            reg_file[cntrl_regs[OPERAND_1]] = readWord(cntrl_regs[IMMEDIATE]);
            memStream = false;
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case STB:
            if (cntrl_regs[IMMEDIATE] >= prog_mem_size) {
                return false;
            }
            writeByte(cntrl_regs[IMMEDIATE], reg_file[cntrl_regs[OPERAND_1]] & 0xFF);
            memStream = false;
            break;

        case LDB:
            if (cntrl_regs[IMMEDIATE] >= prog_mem_size) {
                return false;
            }
            reg_file[cntrl_regs[OPERAND_1]] = readByte(cntrl_regs[IMMEDIATE]);
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            memStream = false;
            break;

        case ISTR:
            writeWord(reg_file[cntrl_regs[OPERAND_2]], reg_file[cntrl_regs[OPERAND_1]]);
            memStream = false;
            break;

        case ILDR:
            reg_file[cntrl_regs[OPERAND_1]] = readWord(reg_file[cntrl_regs[OPERAND_2]]);
            memStream = false;
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case ISTB:
            writeByte(reg_file[cntrl_regs[OPERAND_2]], reg_file[cntrl_regs[OPERAND_1]] & 0xFF);
            memStream = false;
            break;

        case ILDB:
            reg_file[cntrl_regs[OPERAND_1]] = readByte(reg_file[cntrl_regs[OPERAND_2]]);
            memStream = false;
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case ADD:
            reg_file[cntrl_regs[OPERAND_1]] = reg_file[cntrl_regs[OPERAND_2]] + reg_file[cntrl_regs[OPERAND_3]];
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case ADDI:
            reg_file[cntrl_regs[OPERAND_1]] = reg_file[cntrl_regs[OPERAND_2]] + cntrl_regs[IMMEDIATE];
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case SUB:
            reg_file[cntrl_regs[OPERAND_1]] = reg_file[cntrl_regs[OPERAND_2]] - reg_file[cntrl_regs[OPERAND_3]];
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case SUBI:
            reg_file[cntrl_regs[OPERAND_1]] = reg_file[cntrl_regs[OPERAND_2]] - cntrl_regs[IMMEDIATE];
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case MUL:
            reg_file[cntrl_regs[OPERAND_1]] = reg_file[cntrl_regs[OPERAND_2]] * reg_file[cntrl_regs[OPERAND_3]];
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case MULI:
            reg_file[cntrl_regs[OPERAND_1]] = reg_file[cntrl_regs[OPERAND_2]] * cntrl_regs[IMMEDIATE];
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case DIV:
            if (reg_file[cntrl_regs[OPERAND_3]] == 0) {
                return false;
            }
            reg_file[cntrl_regs[OPERAND_1]] = reg_file[cntrl_regs[OPERAND_2]] / reg_file[cntrl_regs[OPERAND_3]];
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }

            break;

        case DIVI:
            if (cntrl_regs[IMMEDIATE] == 0) {
                return false;
            }
            reg_file[cntrl_regs[OPERAND_1]] = static_cast<int>(reg_file[cntrl_regs[OPERAND_2]]) /
                static_cast<int>(cntrl_regs[IMMEDIATE]);
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }

            break;

        case SDIV:
            if (reg_file[cntrl_regs[OPERAND_3]] == 0) {
                return false;
            }
            reg_file[cntrl_regs[OPERAND_1]] = static_cast<int>(reg_file[cntrl_regs[OPERAND_2]]) /
                static_cast<int>(reg_file[cntrl_regs[OPERAND_3]]);
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            break;

        case AND:
            reg_file[cntrl_regs[OPERAND_1]] = (reg_file[cntrl_regs[OPERAND_2]] && reg_file[cntrl_regs[OPERAND_3]]) ? 1 : 0;
            break;

        case OR:
            reg_file[cntrl_regs[OPERAND_1]] = (reg_file[cntrl_regs[OPERAND_2]] || reg_file[cntrl_regs[OPERAND_3]]) ? 1 : 0;
            break;

        case CMP: {
            const int val1 = static_cast<int>(reg_file[cntrl_regs[OPERAND_2]]);
            const int val2 = static_cast<int>(reg_file[cntrl_regs[OPERAND_3]]);
            if (val1 == val2) {
                reg_file[cntrl_regs[OPERAND_1]] = 0;
            } else if (val1 > val2) {
                reg_file[cntrl_regs[OPERAND_1]] = 1;
            } else {
                reg_file[cntrl_regs[OPERAND_1]] = static_cast<unsigned int>(-1);
            }
        }
            break;

        case CMPI: {
            const int val1 = static_cast<int>(reg_file[cntrl_regs[OPERAND_2]]);
            const int val2 = static_cast<int>(cntrl_regs[IMMEDIATE]);
            if (val1 == val2) {
                reg_file[cntrl_regs[OPERAND_1]] = 0;
            } else if (val1 > val2) {
                reg_file[cntrl_regs[OPERAND_1]] = 1;
            } else {
                reg_file[cntrl_regs[OPERAND_1]] = static_cast<unsigned int>(-1);
            }
        }
            break;

        case ALCI: {
            const unsigned int bytes = cntrl_regs[IMMEDIATE];
            reg_file[cntrl_regs[OPERAND_1]] = reg_file[HP];
            reg_file[HP] += bytes;
            if (reg_file[HP] >= reg_file[SP]) {
                return false;
            }
        }
            break;

        case ALLC: {
                if (cntrl_regs[IMMEDIATE] + 3 >= prog_mem_size) {
                    return false;
                }

                const unsigned int word = readWord(cntrl_regs[IMMEDIATE]);
                reg_file[cntrl_regs[OPERAND_1]] = reg_file[HP];
                reg_file[HP] += word;
                memStream = false;

                if (reg_file[HP] >= reg_file[SP]) {
                    return false;
                }
            }
                break;

        case IALLC: {
            const unsigned int address = reg_file[cntrl_regs[OPERAND_2]];
            if (address + 3 >= prog_mem_size) {
                return false;
            }
            const unsigned int word = readWord(address);
            reg_file[cntrl_regs[OPERAND_1]] = reg_file[HP];
            reg_file[HP] += word;
            memStream = false;
            if (reg_file[HP] >= reg_file[SP]) {
                return false;
            }
        }
            break;

        case PSHR: {
            if (reg_file[SP] - 4 < reg_file[SL]) {
                return false;
            }
            reg_file[SP] -= 4;
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            writeWord(reg_file[SP], reg_file[cntrl_regs[OPERAND_1]]);
            memStream = false;
        }
            break;

        case PSHB: {
            if (reg_file[SP] - 1 < reg_file[SL]) {
                return false;
            }
            reg_file[SP]--;
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            writeByte(reg_file[SP], reg_file[cntrl_regs[OPERAND_1]] & 0xFF);
            memStream = false;
        }
            break;

        case POPR: {
            if (reg_file[SP] + 4 > reg_file[SB]) {
                return false;
            }
            reg_file[cntrl_regs[OPERAND_1]] = readWord(reg_file[SP]);
            reg_file[SP] += 4;
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            memStream = false;
            if (!validate_stack_pointer()) { return false; }
        }
            break;

        case POPB: {
            if (reg_file[SP] + 1 > reg_file[SB]) {
                return false;
            }
            reg_file[cntrl_regs[OPERAND_1]] = readByte(reg_file[SP]);
            reg_file[SP] += 1;
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            memStream = false;
            if (!validate_stack_pointer()) { return false; }
        }
            break;

        case CALL: {
            if (reg_file[SP] - 4 < reg_file[SL]) {
                return false;
            }
            reg_file[SP] -= 4;
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            writeWord(reg_file[SP], reg_file[PC]);
            reg_file[PC] = cntrl_regs[IMMEDIATE];
            memStream = false;
        }
            break;

        case RET: {
            if (reg_file[SP] + 4 > reg_file[SB]) {
                return false;
            }
            reg_file[PC] = readWord(reg_file[SP]);
            reg_file[SP] += 4;
            if (cntrl_regs[OPERAND_1] == SP) {
                if (!validate_stack_pointer()) { return false; }
            }
            memStream = false;
        }
            break;

        case TRP:
            switch (cntrl_regs[IMMEDIATE]) {
                case HALT:
                    cleanupAndExit();
                    return true;

                case INT_OUT:
                    std::cout << static_cast<int>(reg_file[3]) << std::flush;
                    break;

                case INT_IN:
                    int val;
                    std::cin >> val;
                    reg_file[3] = static_cast<unsigned int>(val);
                    break;

                case CHAR_OUT:
                    std::cout << static_cast<char>(reg_file[3]) << std::flush;
                    break;

                case CHAR_IN:
                    char c;
                    std::cin >> c;
                    reg_file[3] = static_cast<unsigned int>(c);
                    break;

                case STRING_OUT: {
                    const unsigned int address = reg_file[3];
                    if (address + 3 >= prog_mem_size) {
                        // check
                        return false;
                    }
                    const unsigned int len = readByte(address);
                    for (unsigned int i = 1; i <= len; i++) {
                        if (address + i >= prog_mem_size) {
                            break;
                        }
                        std::cout << static_cast<char>(readByte(address + i));
                    }
                    std::cout << std::flush;
                    memStream = false;
                }
                    break;

                case STRING_IN: {
                    const unsigned int address = reg_file[3];
                    if (address >= prog_mem_size) { // check
                        return false;
                    }
                    std::string str;
                    std::getline(std::cin, str);
                    if (str.length() > 255) {
                        str = str.substr(0, 255);
                    }
                    writeByte(address, static_cast<unsigned int>(str.length()));
                    for (unsigned int i = 0; i < str.length(); i++) {
                        if (address + i + 1 >= prog_mem_size) {
                            break;
                        }
                        writeByte(address + i + 1, static_cast<unsigned int>(str[i]));
                    }
                    if (address + str.length() + 1 < prog_mem_size) {
                        writeByte(address + str.length() + 1, 0);
                    }
                    memStream = false;
                }
                    break;

                case PRINT_REG:
                    for (int i = 0; i < PC; i++) {
                        std::cout << "R" << i << "\t" << reg_file[i] << std::endl;
                    }

                    std::cout << "PC\t" << reg_file[PC] << std::endl;
                    std::cout << "SL\t" << reg_file[SL] << std::endl;
                    std::cout << "SB\t" << reg_file[SB] << std::endl;
                    std::cout << "SP\t" << reg_file[SP] << std::endl;
                    std::cout << "FP\t" << reg_file[FP] << std::endl;
                    std::cout << "HP\t" << reg_file[HP] << std::endl;
                    break;

                default:
                    return false;
            }
            break;
        default:
            return false;
    }
    return true;
}
