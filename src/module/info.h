#pragma once

#if defined(__x86_64__) || defined(__aarch64__)
#define CPU_MODES "32-bit, 64-bit"
#elif defined(__i386__) || defined(__arm__)
#define CPU_MODES "32-bit"
#else
#define CPU_MODES "64-bit"
#endif

const char *get_endianness(void);
const char *get_virtualization(void);
