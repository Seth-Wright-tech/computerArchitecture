#ifndef L1sim_HPP
#define L1sim_HPP

#include <list>
#include <vector>
#include <optional>
#include <cstdint>

struct CacheBlock {
    uint64_t address;
    bool dirty;
    CacheBlock(uint64_t t) : address(t), dirty(false) {}
};

// class for L1 cache
class L1Cache {
    public:
        int num_sets;
        int blocks_per_set;
        std::vector<std::list<CacheBlock>> sets;

        L1Cache(int num_sets, int blocks_per_set) : num_sets(num_sets), blocks_per_set(blocks_per_set), sets(num_sets) {}
        bool containsBlock(std::list<CacheBlock>& cacheSet, uint64_t address);
        std::optional<CacheBlock> insertBlock(std::list<CacheBlock>& cacheSet, uint64_t address, int maxSize, bool writing);
        void removeBlock(std::list<CacheBlock>& cacheSet, uint64_t address);
};

//class for Victim cache
class Victim_cache {
    public: 
        int size;
        std::list<CacheBlock> set;
    
        Victim_cache(int size) : size(size) {}
        bool containsBlock(std::list<CacheBlock>& cacheSet, uint64_t address);
        CacheBlock removeBlock(std::list<CacheBlock>& cacheSet, uint64_t address);
        std::optional<CacheBlock> insertBlock(std::list<CacheBlock>& cacheSet, CacheBlock block, int maxSize);
};


// class for L2 cache
class L2Cache {
    public:
        int num_sets;
        int blocks_per_set;
        std::vector<std::list<CacheBlock>> sets;

        L2Cache(int num_sets, int blocks_per_set) : num_sets(num_sets), blocks_per_set(blocks_per_set), sets(num_sets) {}
        bool containsBlock(std::list<CacheBlock>& cacheSet, uint64_t address);
        void removeBlock(std::list<CacheBlock>& cacheSet, uint64_t address);
        void insertBlockLRU(std::list<CacheBlock>& cacheSet, uint64_t address, int maxSize);
        void insertBlockMRU(std::list<CacheBlock>& cacheSet, uint64_t address, int maxSize);
};
#endif