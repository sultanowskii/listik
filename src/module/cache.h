#pragma once

#include <linux/module.h>

enum cache_type {
    CACHE_TYPE_NO_MORE = 0,
    CACHE_TYPE_DATA = 1,
    CACHE_TYPE_INSTRUCTION = 2,
    CACHE_TYPE_UNIFIED = 3,
    CACHE_TYPE_RESERVED = 4
};

u16             get_ways_of_cache_associativity(unsigned int ebx);
u64             get_cache_size(unsigned int ebx, unsigned int ecx);
u8              get_cache_level(unsigned int eax);
enum cache_type get_cache_type(unsigned int eax);
