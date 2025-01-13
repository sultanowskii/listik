#include "cache.h"

#include <linux/module.h>

enum cache_type get_cache_type(unsigned int eax) {
    return eax & 0b11111;
}

u16 get_ways_of_cache_associativity(unsigned int ebx) {
    return ((ebx >> 22) & 0b1111111111) + 1;
}

u64 get_cache_size(unsigned int ebx, unsigned int ecx) {
    u64 coherency_line_size = (ebx & 0b111111111111) + 1;
    u64 physical_line_partitions = ((ebx >> 12) & 0b1111111111) + 1;
    u64 ways_of_cache_associativity = get_ways_of_cache_associativity(ebx);
    u64 cache_sets = ecx + 1;
    u64 size = coherency_line_size * physical_line_partitions * ways_of_cache_associativity * cache_sets;
    return size;
}

u8 get_cache_level(unsigned int eax) {
    return (eax >> 5) & 3;
}
