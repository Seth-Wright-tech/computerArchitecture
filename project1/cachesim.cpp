#include "cachesim.hpp"

L1Cache* l1_cache = nullptr;
L2Cache* l2_cache = nullptr;
Victim_cache* VC = nullptr;
int num_offset;
int L1_num_index;
int L1_num_tag;
int L2_num_index;
int L2_num_tag;
int l1_s;
int l2_s;
int l2_disable;

//Pseudo-Random Number Generator for RANDOM Replacement policy
static unsigned long int evict_random_next = 1;
int evict_random(void)
{
    evict_random_next = evict_random_next * 1103515243 + 12345;
    return (unsigned int)(evict_random_next / 65536) % 32768;
}

void evict_srand(unsigned int seed)
{
    evict_random_next = seed;
}

/**
 * Subroutine for initializing the cache simulator. You many add and initialize any global or heap
 * variables as needed.
 */

void sim_setup(sim_config_t *config) {
    // making L1 data structure
    cache_config_t *l1 = &config->l1_config;
    l1_s = l1->s;
    num_offset = l1->b;
    L1_num_index = l1->c - l1->b - l1->s;
    L1_num_tag = 64 - (num_offset + L1_num_index);
    l1_cache = new L1Cache(1 << L1_num_index, 1 << l1->s);
    // making L2 data structure
    cache_config_t *l2 = &config->l2_config;
    l2_s = l2->s;
    num_offset = l2->b;
    L2_num_index = l2->c - l2->b - l2->s;
    L2_num_tag = 64 - (num_offset + L2_num_index);
    l2_cache = new L2Cache(1 << L2_num_index, 1 << l2->s);
    l2_disable = l2->disabled;
    // making VC data structure
    VC = new Victim_cache(config->victim_cache_entries);
}

/**
 * Subroutine that simulates the cache one trace event at a time.
 */
void sim_access(char rw, uint64_t addr, sim_stats_t* stats) {
    uint64_t address = (addr >> num_offset);
    uint64_t L1index = (addr >> num_offset) & ((1 << L1_num_index) - 1);
    std::list<CacheBlock>& L1_set = l1_cache->sets[L1index];

    if (rw == WRITE) {
        stats->writes++;
    } else {
        stats->reads++;
    }
    stats->accesses_l1++;

    // Check L1 Cache
    if (l1_cache->containsBlock(L1_set, address)) {
        stats->hits_l1++;
        std::optional<CacheBlock> evicted = l1_cache->insertBlock(L1_set, address, l1_cache->blocks_per_set, (rw == WRITE));
        return;
    }
    
    stats->misses_l1++;

    // Check Victim Cache
    if (VC->size > 0) {
        if (VC->containsBlock(VC->set, address)) {
            stats->hits_victim_cache++;
            CacheBlock removedBlock = VC->removeBlock(VC->set, address);
            bool w;
            if (rw == WRITE) {
                w = true;
            } else {
                w = removedBlock.dirty;
            }
            std::optional<CacheBlock> evicted = l1_cache->insertBlock(L1_set, address, l1_cache->blocks_per_set, w);
            CacheBlock block = evicted.value();
            evicted = VC->insertBlock(VC->set, block, VC->size);
            return;
        }
    }   

    stats->misses_victim_cache++;

    uint64_t L2index = (addr >> num_offset) & ((1 << L2_num_index) - 1);
    std::list<CacheBlock>& L2_set = l2_cache->sets[L2index];
    stats->reads_l2++;

    if (l2_disable == 0) {

        // Check L2 Cache
    
        if (l2_cache->containsBlock(L2_set, address)) {
            stats->read_hits_l2++;
            l2_cache->removeBlock(L2_set, address);
            l2_cache->insertBlockMRU(L2_set, address, l2_cache->blocks_per_set);
            std::optional<CacheBlock> evicted = l1_cache->insertBlock(L1_set, address, l1_cache->blocks_per_set, (rw == WRITE));
            if (evicted.has_value()) {
                if (VC->size > 0) {
                    CacheBlock block = evicted.value();
                    evicted = VC->insertBlock(VC->set, block, VC->size);
                }
                if (evicted.has_value() && evicted.value().dirty) {
                    uint64_t evicted_index = evicted.value().address & ((1 << L2_num_index) - 1);
                    if (l2_cache->containsBlock(l2_cache->sets[evicted_index], evicted.value().address)) {
                        l2_cache->removeBlock(l2_cache->sets[evicted_index], evicted.value().address);
                        l2_cache->insertBlockMRU(l2_cache->sets[evicted_index], evicted.value().address, l2_cache->blocks_per_set);
                    }
                    stats->writes_l2++;
                    stats->write_backs_l1_or_victim_cache++;
                }
            }
            return;
        }
    }

    stats->read_misses_l2++;

    // Block missed all caches
    l2_cache->insertBlockLRU(L2_set, address, l2_cache->blocks_per_set);
    std::optional<CacheBlock> evicted = l1_cache->insertBlock(L1_set, address, l1_cache->blocks_per_set, (rw == WRITE));
    if (evicted.has_value()) {
        if (VC->size > 0) {
            CacheBlock block = evicted.value();
            evicted = VC->insertBlock(VC->set, block, VC->size);
        }
        if (evicted.has_value() && evicted.value().dirty) {
            uint64_t evicted_index = evicted.value().address & ((1 << L2_num_index) - 1);
            if (l2_cache->containsBlock(l2_cache->sets[evicted_index], evicted.value().address)) {
                l2_cache->removeBlock(l2_cache->sets[evicted_index], evicted.value().address);
                l2_cache->insertBlockMRU(l2_cache->sets[evicted_index], evicted.value().address, l2_cache->blocks_per_set);
            }
            stats->writes_l2++;
            stats->write_backs_l1_or_victim_cache++;
        }
    }
    return;
}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 */
void sim_finish(sim_stats_t *stats) {
    // final stat calculations
    stats->hit_ratio_l1 = (double)stats->hits_l1 / (double)stats->accesses_l1;
    stats->miss_ratio_l1 = (double)stats->misses_l1 / (double)stats->accesses_l1;
    stats->hit_ratio_victim_cache = (double)stats->hits_victim_cache / (double)stats->misses_l1;
    stats->miss_ratio_victim_cache = (double)stats->misses_victim_cache / (double)stats->misses_l1;
    stats->read_hit_ratio_l2 = (double)stats->read_hits_l2 / (double)stats->reads_l2;
    stats->read_miss_ratio_l2 = (double)stats->read_misses_l2 / (double)stats->reads_l2;
    if (l2_disable == 0) {
        stats->avg_access_time_l2 = (L2_HIT_TIME_CONST  + L2_HIT_TIME_PER_S * l2_s) + stats->read_miss_ratio_l2 * (DRAM_AT + (DRAM_AT_PER_WORD * ((1 << num_offset) / WORD_SIZE)));
    } else {
        stats->avg_access_time_l2 = 80;
    }
    stats->avg_access_time_l1 = (L1_HIT_TIME_CONST  + L1_HIT_TIME_PER_S * l1_s) + ((stats->miss_ratio_victim_cache * stats->miss_ratio_l1) * stats->avg_access_time_l2);

    // cleaning up data structures
    delete l1_cache;
    delete l2_cache;
    delete VC;
}