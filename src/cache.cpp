#include "../include/cache.h"

CacheResult::CacheResult(const bool hit, const unsigned int cycles, const bool wb, const unsigned int wbCycles)
    : hit(hit), cycles(cycles), writebackOccurred(wb), writebackCycles(wbCycles) {}

unsigned int CacheResult::getCycles() const {
    return hit ? cycles : cycles + writebackCycles;
}

Cache::AddressInfo::AddressInfo(const unsigned int addr, const unsigned int numSets) {
    blockAddress = addr / BLOCK_SIZE;
    blockOffset = addr % BLOCK_SIZE;

    if (numSets > 0) {
        index = blockAddress % numSets;
        tag = blockAddress / numSets;
    } else {
        index = 0;
        tag = blockAddress;
    }
}

CacheResult Cache::calculateTiming(const bool hit, const bool wb, const unsigned int blocksToRead) {
    unsigned int readCycles = 1;
    unsigned int writebackCycles = 0;
    if (hit) {
        return CacheResult(hit, readCycles, wb, writebackCycles);
    }

    readCycles += 8 + 2 * (blocksToRead * WORDS_PER_BLOCK - 1);

    if (wb) {
        writebackCycles = 8 + 2 * (WORDS_PER_BLOCK - 1);
    }

    return CacheResult(hit, readCycles, wb, writebackCycles);
}

CacheLine::CacheLine() : valid(false), dirty(false), tag(0), lastUsed(0), data(BLOCK_SIZE, 0) {}

void CacheLine::invalidate() {
    valid = false;
    dirty = false;
    tag = 0;
    lastUsed = 0;
}

SystemMemory::SystemMemory(unsigned char* prog_mem, const unsigned int prog_mem_size)
    : prog_mem(prog_mem), prog_mem_size(prog_mem_size) {}

unsigned char SystemMemory::readByteFromMemory(const unsigned int address) {
    if (address >= prog_mem_size) { return 0; }
    return prog_mem[address];
}

unsigned int SystemMemory::readWordFromMemory(const unsigned int address) {
    if (address + 3 >= prog_mem_size) { return 0; }
    return (prog_mem[address + 3] << 24) |
           (prog_mem[address + 2] << 16) |
           (prog_mem[address + 1] << 8) |
            prog_mem[address];
}

void SystemMemory::writeByteToMemory(const unsigned int address, const unsigned char data) {
    if (address >= prog_mem_size) { return; }
    prog_mem[address] = data;
}

void SystemMemory::writeWordToMemory(const unsigned int address, const unsigned int data) {
    if (address + 3 >= prog_mem_size) { return; }
    prog_mem[address] = data & 0xFF;
    prog_mem[address + 1] = (data >> 8) & 0xFF;
    prog_mem[address + 2] = (data >> 16) & 0xFF;
    prog_mem[address + 3] = (data >> 24) & 0xFF;
}

unsigned int SystemMemory::getMemorySize() const {
    return prog_mem_size;
}

DirectMappedCache::DirectMappedCache(MemoryInterface* memory) : memory(memory), counter(0) {
    cache.resize(CACHE_LINES);
}

std::string DirectMappedCache::getType() const {
    return "Direct Mapped Cache";
}

void DirectMappedCache::reset() {
    for (CacheLine& line : cache) {
        line.invalidate();
    }
    counter = 0;
}

CacheResult DirectMappedCache::readByte(const unsigned int address) {
    const AddressInfo addr(address, CACHE_LINES);
    CacheLine& line = cache[addr.index];

    if (line.valid && line.tag == addr.tag) {
        line.lastUsed = ++counter;
        return calculateTiming(true);
    }

    const bool needsWriteback = line.valid && line.dirty;
    const CacheResult result = calculateTiming(false, needsWriteback);

    if (needsWriteback) {
        writeBackBlock(line, addr.index);
    }

    loadBlockFromMemory(line, addr.tag, address - addr.blockOffset);
    line.lastUsed = ++counter;

    return result;
}

unsigned char DirectMappedCache::getCachedByte(const unsigned int address) {
    const AddressInfo addr(address, CACHE_LINES);
    const CacheLine& line = cache[addr.index];

    return line.data[addr.blockOffset];
}

CacheResult DirectMappedCache::readWord(const unsigned int address) {
    if ((address % BLOCK_SIZE) + 4 > BLOCK_SIZE) {
        const CacheResult result1 = readByte(address);
        const CacheResult result2 = readByte(address + 3);
        return CacheResult(result1.hit && result2.hit,
            result1.getCycles() + result2.getCycles(),
            result1.writebackOccurred || result2.writebackOccurred,
            result1.writebackCycles + result2.writebackCycles
        );
    }

    return readByte(address);
}

unsigned int DirectMappedCache::getCachedWord(const unsigned int address) {
    const AddressInfo addr(address, CACHE_LINES);
    const CacheLine& line = cache[addr.index];

    return line.data[addr.blockOffset] |
        (line.data[addr.blockOffset + 1] << 8) |
        (line.data[addr.blockOffset + 2] << 16) |
        (line.data[addr.blockOffset + 3] << 24);
}

CacheResult DirectMappedCache::writeByte(const unsigned int address, const unsigned char data) {
    const AddressInfo addr(address, CACHE_LINES);
    CacheLine& line = cache[addr.index];

    if (line.valid && line.tag == addr.tag) {
        line.data[addr.blockOffset] = data;
        line.dirty = true;
        line.lastUsed = ++counter;
        return calculateTiming(true);
    }

    const bool needsWriteback = line.valid && line.dirty;
    const CacheResult result = calculateTiming(false, needsWriteback);

    if (needsWriteback) {
        writeBackBlock(line, addr.index);
    }

    loadBlockFromMemory(line, addr.tag, address - addr.blockOffset);
    line.data[addr.blockOffset] = data;
    line.dirty = true;
    line.lastUsed = ++counter;

    return result;
}

CacheResult DirectMappedCache::writeWord(const unsigned int address, const unsigned int data) {
    if ((address % BLOCK_SIZE) + 4 > BLOCK_SIZE) {
        const CacheResult result1 = writeByte(address, data & 0xFF);
        const CacheResult result2 = writeByte(address + 1, (data >> 8) & 0xFF);
        const CacheResult result3 = writeByte(address + 2, (data >> 16) & 0xFF);
        const CacheResult result4 = writeByte(address + 3, (data >> 24) & 0xFF);
        return CacheResult(result1.hit && result2.hit && result3.hit && result4.hit,
            result1.getCycles() + result2.getCycles() + result3.getCycles() + result4.getCycles(),
            result1.writebackOccurred || result2.writebackOccurred || result3.writebackOccurred || result4.writebackOccurred,
            result1.writebackCycles + result2.writebackCycles + result3.writebackCycles + result4.writebackCycles
        );
    }

    const AddressInfo addr(address, CACHE_LINES);
    CacheLine& line = cache[addr.index];

    if (line.valid && line.tag == addr.tag) {
        writeWordToBlock(line, addr.blockOffset, data);
        line.dirty = true;
        line.lastUsed = ++counter;
        return calculateTiming(true);
    }

    const bool needsWriteback = line.valid && line.dirty;
    const CacheResult result = calculateTiming(false, needsWriteback);

    if (needsWriteback) {
        writeBackBlock(line, addr.index);
    }

    loadBlockFromMemory(line, addr.tag, address - addr.blockOffset);
    writeWordToBlock(line, addr.blockOffset, data);
    line.dirty = true;
    line.lastUsed = ++counter;

    return result;
}

void DirectMappedCache::loadBlockFromMemory(CacheLine& line, const unsigned int tag, const unsigned int offset) const {
    for (unsigned int i = 0; i < BLOCK_SIZE; i++) {
        line.data[i] = memory->readByteFromMemory(offset + i);
    }
    line.valid = true;
    line.dirty = false;
    line.tag = tag;
}

void DirectMappedCache::writeBackBlock(CacheLine& line, const unsigned int index) const {
    const unsigned int blockAddress = (line.tag * CACHE_LINES + index) * BLOCK_SIZE;
    for (unsigned int i = 0; i < BLOCK_SIZE; i++) {
        memory->writeByteToMemory(blockAddress + i, line.data[i]);
    }
    line.invalidate();
}

void DirectMappedCache::writeWordToBlock(CacheLine& line, const unsigned int offset, const unsigned int data) {
    line.data[offset] = data & 0xFF;
    line.data[offset + 1] = (data >> 8) & 0xFF;
    line.data[offset + 2] = (data >> 16) & 0xFF;
    line.data[offset + 3] = (data >> 24) & 0xFF;
}

FullyAssociativeCache::FullyAssociativeCache(MemoryInterface* memory) : memory(memory), counter(0) {
    cache.resize(CACHE_LINES);
}

std::string FullyAssociativeCache::getType() const {
    return "Fully Associative Cache";
}

void FullyAssociativeCache::reset() {
    for (CacheLine& line : cache) {
        line.invalidate();
    }
    counter = 0;
}

CacheResult FullyAssociativeCache::readByte(const unsigned int address) {
    const AddressInfo addr(address, 0);

    for (CacheLine& line : cache) {
        if (line.valid && line.tag == addr.tag) {
            line.lastUsed = ++counter;
            return calculateTiming(true);
        }
    }

    CacheLine& evictLine = findLRULine();
    const bool needsWriteback = evictLine.valid && evictLine.dirty;
    const CacheResult result = calculateTiming(false, needsWriteback);

    if (needsWriteback) {
        writeBackBlock(evictLine);
    }

    loadBlockFromMemory(evictLine, addr.tag, address - addr.blockOffset);
    evictLine.lastUsed = ++counter;

    return result;
}

unsigned char FullyAssociativeCache::getCachedByte(const unsigned int address) {
    const AddressInfo addr(address, 0);

    for (const CacheLine& line : cache) {
        if (line.valid && line.tag == addr.tag) {
            return line.data[addr.blockOffset];
        }
    }

    return 0;
}

CacheResult FullyAssociativeCache::readWord(const unsigned int address) {
    if ((address % BLOCK_SIZE) + 4 > BLOCK_SIZE) {
        const CacheResult result1 = readByte(address);
        const CacheResult result2 = readByte(address + 3);
        return CacheResult(result1.hit && result2.hit,
            result1.getCycles() + result2.getCycles(),
            result1.writebackOccurred || result2.writebackOccurred,
            result1.writebackCycles + result2.writebackCycles
        );
    }

    return readByte(address);
}

unsigned int FullyAssociativeCache::getCachedWord(const unsigned int address) {
    const AddressInfo addr(address, 0);

    for (const CacheLine& line : cache) {
        if (line.valid && line.tag == addr.tag) {
            return line.data[addr.blockOffset] |
                (line.data[addr.blockOffset + 1] << 8) |
                (line.data[addr.blockOffset + 2] << 16) |
                (line.data[addr.blockOffset + 3] << 24);
        }
    }
    return 0;
}

CacheResult FullyAssociativeCache::writeByte(const unsigned int address, const unsigned char data) {
    const AddressInfo addr(address, 0);

    for (CacheLine& line : cache) {
        if (line.valid && line.tag == addr.tag) {
            line.data[addr.blockOffset] = data;
            line.dirty = true;
            line.lastUsed = ++counter;
            return calculateTiming(true);
        }
    }

    CacheLine& evictLine = findLRULine();
    const bool needsWriteback = evictLine.valid && evictLine.dirty;
    const CacheResult result = calculateTiming(false, needsWriteback);

    if (needsWriteback) {
        writeBackBlock(evictLine);
    }

    loadBlockFromMemory(evictLine, addr.tag, address - addr.blockOffset);
    evictLine.data[addr.blockOffset] = data;
    evictLine.dirty = true;
    evictLine.lastUsed = ++counter;

    return result;
}

CacheResult FullyAssociativeCache::writeWord(const unsigned int address, const unsigned int data) {
    if ((address % BLOCK_SIZE) + 4 > BLOCK_SIZE) {
        const CacheResult result1 = writeByte(address, data & 0xFF);
        const CacheResult result2 = writeByte(address + 1, (data >> 8) & 0xFF);
        const CacheResult result3 = writeByte(address + 2, (data >> 16) & 0xFF);
        const CacheResult result4 = writeByte(address + 3, (data >> 24) & 0xFF);
        return CacheResult(result1.hit && result2.hit && result3.hit && result4.hit,
            result1.getCycles() + result2.getCycles() + result3.getCycles() + result4.getCycles(),
            result1.writebackOccurred || result2.writebackOccurred || result3.writebackOccurred || result4.writebackOccurred,
            result1.writebackCycles + result2.writebackCycles + result3.writebackCycles + result4.writebackCycles
        );
    }

    const AddressInfo addr(address, 0);

    for (CacheLine& line : cache) {
        if (line.valid && line.tag == addr.tag) {
            writeWordToBlock(line, addr.blockOffset, data);
            line.dirty = true;
            line.lastUsed = ++counter;
            return calculateTiming(true);
        }
    }

    CacheLine& evictLine = findLRULine();
    const bool needsWriteback = evictLine.valid && evictLine.dirty;
    const CacheResult result = calculateTiming(false, needsWriteback);

    if (needsWriteback) {
        writeBackBlock(evictLine);
    }

    loadBlockFromMemory(evictLine, addr.tag, address - addr.blockOffset);
    writeWordToBlock(evictLine, addr.blockOffset, data);
    evictLine.dirty = true;
    evictLine.lastUsed = ++counter;

    return result;
}

CacheLine& FullyAssociativeCache::findLRULine() {
    CacheLine* lruLine = &cache[0];
    for (CacheLine& line : cache) {
        if (!line.valid) {
            return line;
        }
        if (line.lastUsed < lruLine->lastUsed) {
            lruLine = &line;
        }
    }
    return *lruLine;
}

void FullyAssociativeCache::loadBlockFromMemory(CacheLine& line, const unsigned int tag, const unsigned int offset) const {
    for (unsigned int i = 0; i < BLOCK_SIZE; i++) {
        line.data[i] = memory->readByteFromMemory(offset + i);
    }
    line.valid = true;
    line.dirty = false;
    line.tag = tag;
}

void FullyAssociativeCache::writeBackBlock(const CacheLine& line) const {
    const unsigned int blockAddress = line.tag * BLOCK_SIZE;
    for (unsigned int i = 0; i < BLOCK_SIZE; i++) {
        memory->writeByteToMemory(blockAddress + i, line.data[i]);
    }
}

void FullyAssociativeCache::writeWordToBlock(CacheLine& line, const unsigned int offset, const unsigned int data) {
    line.data[offset] = data & 0xFF;
    line.data[offset + 1] = (data >> 8) & 0xFF;
    line.data[offset + 2] = (data >> 16) & 0xFF;
    line.data[offset + 3] = (data >> 24) & 0xFF;
}

TwoWaySetAssociativeCache::TwoWaySetAssociativeCache(MemoryInterface* memory) : memory(memory), counter(0) {
    cache.resize(SETS, std::vector<CacheLine>(WAYS));
}

std::string TwoWaySetAssociativeCache::getType() const {
    return "Two Way Set Associative Cache";
}

void TwoWaySetAssociativeCache::reset() {
    for (std::vector<CacheLine>& set : cache) {
        for (CacheLine& line : set) {
            line.invalidate();
        }
    }
    counter = 0;
}

CacheResult TwoWaySetAssociativeCache::readByte(const unsigned int address) {
    const AddressInfo addr(address, SETS);
    auto& set = cache[addr.index];

    for (CacheLine& line : set) {
        if (line.valid && line.tag == addr.tag) {
            line.lastUsed = ++counter;
            return calculateTiming(true);
        }
    }

    CacheLine& evictLine = findLRULine(set);
    const bool needsWriteback = evictLine.valid && evictLine.dirty;
    const CacheResult result = calculateTiming(false, needsWriteback);

    if (needsWriteback) {
        writeBackBlock(evictLine, addr.index);
    }

    loadBlockFromMemory(evictLine, addr.tag, address - addr.blockOffset);
    evictLine.lastUsed = ++counter;

    return result;
}

unsigned char TwoWaySetAssociativeCache::getCachedByte(const unsigned int address) {
    const AddressInfo addr(address, SETS);
    const auto& set = cache[addr.index];

    for (const CacheLine& line : set) {
        if (line.valid && line.tag == addr.tag) {
            return line.data[addr.blockOffset];
        }
    }

    return 0;
}


CacheResult TwoWaySetAssociativeCache::readWord(const unsigned int address) {
    if ((address % BLOCK_SIZE) + 4 > BLOCK_SIZE) {
        const CacheResult result1 = readByte(address);
        const CacheResult result2 = readByte(address + 3);
        return CacheResult(result1.hit && result2.hit,
            result1.getCycles() + result2.getCycles(),
            result1.writebackOccurred || result2.writebackOccurred,
            result1.writebackCycles + result2.writebackCycles
        );
    }

    return readByte(address);
}

unsigned int TwoWaySetAssociativeCache::getCachedWord(const unsigned int address) {
    const AddressInfo addr(address, SETS);
    const auto& set = cache[addr.index];

    for (const CacheLine& line : set) {
        if (line.valid && line.tag == addr.tag) {
            return line.data[addr.blockOffset] |
                (line.data[addr.blockOffset + 1] << 8) |
                (line.data[addr.blockOffset + 2] << 16) |
                (line.data[addr.blockOffset + 3] << 24);
        }
    }

    return 0;
}

CacheResult TwoWaySetAssociativeCache::writeByte(const unsigned int address, const unsigned char data) {
    const AddressInfo addr(address, SETS);
    auto& set = cache[addr.index];

    for (CacheLine& line : set) {
        if (line.valid && line.tag == addr.tag) {
            line.data[addr.blockOffset] = data;
            line.dirty = true;
            line.lastUsed = ++counter;
            return calculateTiming(true);
        }
    }

    CacheLine& evictLine = findLRULine(set);
    const bool needsWriteback = evictLine.valid && evictLine.dirty;
    const CacheResult result = calculateTiming(false, needsWriteback);

    if (needsWriteback) {
        writeBackBlock(evictLine, addr.index);
    }

    loadBlockFromMemory(evictLine, addr.tag, address - addr.blockOffset);
    evictLine.data[addr.blockOffset] = data;
    evictLine.dirty = true;
    evictLine.lastUsed = ++counter;

    return result;
}

CacheResult TwoWaySetAssociativeCache::writeWord(const unsigned int address, const unsigned int data) {
    if ((address % BLOCK_SIZE) + 4 > BLOCK_SIZE) {
        const CacheResult result1 = writeByte(address, data & 0xFF);
        const CacheResult result2 = writeByte(address + 1, (data >> 8) & 0xFF);
        const CacheResult result3 = writeByte(address + 2, (data >> 16) & 0xFF);
        const CacheResult result4 = writeByte(address + 3, (data >> 24) & 0xFF);
        return CacheResult(result1.hit && result2.hit && result3.hit && result4.hit,
            result1.getCycles() + result2.getCycles() + result3.getCycles() + result4.getCycles(),
            result1.writebackOccurred || result2.writebackOccurred || result3.writebackOccurred || result4.writebackOccurred,
            result1.writebackCycles + result2.writebackCycles + result3.writebackCycles + result4.writebackCycles
        );
    }
    const AddressInfo addr(address, SETS);
    auto& set = cache[addr.index];

    for (CacheLine& line : set) {
        if (line.valid && line.tag == addr.tag) {
            writeWordToBlock(line, addr.blockOffset, data);
            line.dirty = true;
            line.lastUsed = ++counter;
            return calculateTiming(true);
        }
    }

    CacheLine& evictLine = findLRULine(set);
    const bool needsWriteback = evictLine.valid && evictLine.dirty;
    const CacheResult result = calculateTiming(false, needsWriteback);

    if (needsWriteback) {
        writeBackBlock(evictLine, addr.index);
    }

    loadBlockFromMemory(evictLine, addr.tag, address - addr.blockOffset);
    writeWordToBlock(evictLine, addr.blockOffset, data);
    evictLine.dirty = true;
    evictLine.lastUsed = ++counter;

    return result;
}

CacheLine& TwoWaySetAssociativeCache::findLRULine(std::vector<CacheLine>& set) {
    CacheLine* lruLine = &set[0];
    for (CacheLine& line : set) {
        if (!line.valid) {
            return line;
        }
        if (line.lastUsed < lruLine->lastUsed) {
            lruLine = &line;
        }
    }
    return *lruLine;
}

void TwoWaySetAssociativeCache::loadBlockFromMemory(CacheLine& line, const unsigned int tag, const unsigned int offset) const {
    for (unsigned int i = 0; i < BLOCK_SIZE; i++) {
        line.data[i] = memory->readByteFromMemory(offset + i);
    }
    line.valid = true;
    line.dirty = false;
    line.tag = tag;
}

void TwoWaySetAssociativeCache::writeBackBlock(const CacheLine& line, const unsigned int index) const {
    const unsigned int blockAddress = (line.tag * SETS + index) * BLOCK_SIZE;
    for (unsigned int i = 0; i < BLOCK_SIZE; i++) {
        memory->writeByteToMemory(blockAddress + i, line.data[i]);
    }
}

void TwoWaySetAssociativeCache::writeWordToBlock(CacheLine& line, const unsigned int offset, const unsigned int data) {
    line.data[offset] = data & 0xFF;
    line.data[offset + 1] = (data >> 8) & 0xFF;
    line.data[offset + 2] = (data >> 16) & 0xFF;
    line.data[offset + 3] = (data >> 24) & 0xFF;
}

std::unique_ptr<Cache> CacheFactory::createCache(const unsigned int type, MemoryInterface* memory) {
    switch (type) {
        case 1:
            return std::make_unique<DirectMappedCache>(memory);
        case 2:
            return std::make_unique<FullyAssociativeCache>(memory);
        case 3:
            return std::make_unique<TwoWaySetAssociativeCache>(memory);
        default:
            return nullptr;
    }
}
