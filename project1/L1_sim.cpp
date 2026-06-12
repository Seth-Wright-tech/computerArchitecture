#include "L1_sim.hpp"

// L1 cache functions

// checks to see if a set has a block in it 
bool L1Cache::containsBlock(std::list<CacheBlock>& cacheSet, uint64_t address) {
    for (auto it = cacheSet.begin(); it != cacheSet.end(); ++it) {
        if (it->address == address) {
            return true; 
        }
    }
    return false;
}

// insert a cache block into the provided set if there size is greater then max size evict and if block already exists in cache move block to MRU position
std::optional<CacheBlock> L1Cache::insertBlock(std::list<CacheBlock>& cacheSet, uint64_t address, int maxSize, bool writing) {
    for (auto it = cacheSet.begin(); it != cacheSet.end(); ++it) {
        if (it->address == address) {
            CacheBlock block = *it;
            cacheSet.erase(it);
            if (writing) {
                block.dirty = true;
            }
            cacheSet.push_front(block);
            return std::nullopt;
        }
    }

    CacheBlock block = CacheBlock(address);
    if (writing) {
        block.dirty = true;
    }
    cacheSet.push_front(block);

    if (cacheSet.size() > maxSize) {
        CacheBlock evicted = cacheSet.back();
        cacheSet.pop_back();
        return evicted;
    }

    return std::nullopt;
}

// remove the block in a given set with the provided addresss
void L1Cache::removeBlock(std::list<CacheBlock>& cacheSet, uint64_t address) {
    for (auto it = cacheSet.begin(); it != cacheSet.end(); ++it) {
        if (it->address == address) {
            cacheSet.erase(it);
            return;
        }
    }
}

// victim cache functions

// checks to see if a set has a block in it 
bool Victim_cache::containsBlock(std::list<CacheBlock>& cacheSet, uint64_t address) {
    for (auto it = cacheSet.begin(); it != cacheSet.end(); ++it) {
        if (it->address == address) {
            return true; 
        }
    }
    return false;
}

// remove the block in the VC with the provided address
CacheBlock Victim_cache::removeBlock(std::list<CacheBlock>& cacheSet, uint64_t address) {
    for (auto it = cacheSet.begin(); it != cacheSet.end(); ++it) {
        if (it->address == address) {
            CacheBlock removedBlock = *it;
            cacheSet.erase(it);
            return removedBlock;
        }
    }
}

// insert a cache block into the provided set if there size is greater then max size evict
std::optional<CacheBlock> Victim_cache::insertBlock(std::list<CacheBlock>& cacheSet, CacheBlock block, int maxSize) {
    cacheSet.push_front(block);

    if (cacheSet.size() > maxSize) {
        CacheBlock evicted = cacheSet.back();
        cacheSet.pop_back();
        return evicted;
    }

    return std::nullopt;
}

// L2 cache functions

// checks to see if a set has a block in it 
bool L2Cache::containsBlock(std::list<CacheBlock>& cacheSet, uint64_t address) {
    for (auto it = cacheSet.begin(); it != cacheSet.end(); ++it) {
        if (it->address == address) {
            return true; 
        }
    }
    return false;
}

// remove the block in a given set with the provided address
void L2Cache::removeBlock(std::list<CacheBlock>& cacheSet, uint64_t address) {
    for (auto it = cacheSet.begin(); it != cacheSet.end(); ++it) {
        if (it->address == address) {
            cacheSet.erase(it);
            return;
        }
    }
}

// insert new block into LRU position of the cache and will pop from LRU position if needed (LIP)
void L2Cache::insertBlockLRU(std::list<CacheBlock>& cacheSet, uint64_t address, int maxSize) {
    if (cacheSet.size() >=  maxSize) {
        cacheSet.pop_front();
    }

    cacheSet.push_front(CacheBlock(address));

    return;
}

// insert new block into MRU position of the cache and will pop from LRU position if needed 
void L2Cache::insertBlockMRU(std::list<CacheBlock>& cacheSet, uint64_t address, int maxSize) {
    if (cacheSet.size() >= maxSize) {
        cacheSet.pop_front();
    }

    cacheSet.push_back(CacheBlock(address));

    return;
}

