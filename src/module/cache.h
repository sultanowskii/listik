#pragma once

#include <linux/module.h>

u16 get_ways_of_cache_associativity(unsigned int ebx);
u64 get_cache_size(unsigned int ebx, unsigned int ecx);