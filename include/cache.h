#pragma once

#include <memory>
#include <string>
#include <vector>

constexpr unsigned int CACHE_LINES = 32;
constexpr unsigned int BLOCK_SIZE = 32;
constexpr unsigned int WORDS_PER_BLOCK = BLOCK_SIZE / 4;

struct CacheResult {
    bool hit;
    unsigned int cycles;
    bool writebackOccurred;
    unsigned int writebackCycles;

    explicit CacheResult(const bool hit = false, unsigned int cycles = 0, const bool wb = false, const unsigned int wbCycles = 0);
    unsigned int getCycles() const;
};

class Cache {
public:
    virtual ~Cache() = default;

    virtual CacheResult readByte(unsigned int address) = 0;
    virtual CacheResult readWord(unsigned int address) = 0;
    virtual CacheResult writeByte(unsigned int address, unsigned char data) = 0;
    virtual CacheResult writeWord(unsigned int address, unsigned int data) = 0;

    virtual unsigned char getCachedByte(unsigned int address) = 0;
    virtual unsigned int getCachedWord(unsigned int address) = 0;

    virtual void reset() = 0;
    virtual std::string getType() const = 0;

protected:
    struct AddressInfo {
        unsigned int blockAddress;
        unsigned int blockOffset;
        unsigned int tag;
        unsigned int index;

        AddressInfo(const unsigned int addr, const unsigned int numSets);
    };

    static CacheResult calculateTiming(const bool hit, const bool wb = false, const unsigned int blocksToRead = 1);
};

class CacheLine {
public:
    bool valid;
    bool dirty;
    unsigned int tag;
    unsigned int lastUsed;
    std::vector<unsigned char> data;

    CacheLine();
    void invalidate();
};

class MemoryInterface {
public:
    virtual ~MemoryInterface() = default;
    virtual unsigned char readByteFromMemory(unsigned int address) = 0;
    virtual unsigned int readWordFromMemory(unsigned int address) = 0;
    virtual void writeByteToMemory(unsigned int address, unsigned char data) = 0;
    virtual void writeWordToMemory(unsigned int address, unsigned int data) = 0;
    virtual unsigned int getMemorySize() const = 0;
};

class SystemMemory final : public MemoryInterface {
private:
    unsigned char* prog_mem;
    unsigned int prog_mem_size;

public:
    SystemMemory(unsigned char* prog_mem, unsigned int prog_mem_size);

    unsigned char readByteFromMemory(unsigned int address) override;
    unsigned int readWordFromMemory(unsigned int address) override;
    void writeByteToMemory(unsigned int address, unsigned char data) override;
    void writeWordToMemory(unsigned int address, unsigned int data) override;
    unsigned int getMemorySize() const override;
};

class DirectMappedCache final : public Cache {
private:
    std::vector<CacheLine> cache;
    MemoryInterface* memory;
    unsigned int counter;

    void loadBlockFromMemory(CacheLine& line, const unsigned int tag, const unsigned int offset) const;
    void writeBackBlock(CacheLine& line, const unsigned int index) const;
    static void writeWordToBlock(CacheLine& line, const unsigned int offset, const unsigned int data);

public:
    explicit DirectMappedCache(MemoryInterface* memory);

    std::string getType() const override;
    void reset() override;
    CacheResult readByte(const unsigned int address) override;
    CacheResult readWord(const unsigned int address) override;
    unsigned char getCachedByte(const unsigned int address) override;
    unsigned int getCachedWord(const unsigned int address) override;
    CacheResult writeByte(const unsigned int address, const unsigned char data) override;
    CacheResult writeWord(const unsigned int address, const unsigned int data) override;
};

class FullyAssociativeCache final : public Cache {
private:
    std::vector<CacheLine> cache;
    MemoryInterface* memory;
    unsigned int counter;

    CacheLine& findLRULine();
    void loadBlockFromMemory(CacheLine& line, const unsigned int tag, const unsigned int offset) const;
    void writeBackBlock(const CacheLine& line) const;
    static void writeWordToBlock(CacheLine& line, const unsigned int offset, const unsigned int data);

public:
    explicit FullyAssociativeCache(MemoryInterface* memory);

    std::string getType() const override;
    void reset() override;
    CacheResult readByte(const unsigned int address) override;
    CacheResult readWord(const unsigned int address) override;
    unsigned char getCachedByte(const unsigned int address) override;
    unsigned int getCachedWord(const unsigned int address) override;
    CacheResult writeByte(const unsigned int address, const unsigned char data) override;
    CacheResult writeWord(const unsigned int address, const unsigned int data) override;
};

class TwoWaySetAssociativeCache final : public Cache {
private:
    std::vector<std::vector<CacheLine>> cache;
    MemoryInterface* memory;
    unsigned int counter;
    static constexpr unsigned int WAYS = 2;
    static constexpr unsigned int SETS = CACHE_LINES / WAYS;

    static CacheLine& findLRULine(std::vector<CacheLine>& set);
    void loadBlockFromMemory(CacheLine& line, const unsigned int tag, const unsigned int offset) const;
    void writeBackBlock(const CacheLine& line, const unsigned int index) const;
    static void writeWordToBlock(CacheLine& line, const unsigned int offset, const unsigned int data);

public:
    explicit TwoWaySetAssociativeCache(MemoryInterface* memory);

    std::string getType() const override;
    void reset() override;
    CacheResult readByte(const unsigned int address) override;
    CacheResult readWord(const unsigned int address) override;
    unsigned char getCachedByte(const unsigned int address) override;
    unsigned int getCachedWord(const unsigned int address) override;
    CacheResult writeByte(const unsigned int address, const unsigned char data) override;
    CacheResult writeWord(const unsigned int address, const unsigned int data) override;
};

class CacheFactory {
public:
    static std::unique_ptr<Cache> createCache(unsigned int type, MemoryInterface* memory);
};
