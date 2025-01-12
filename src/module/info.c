#include <linux/module.h> 

const char *get_endianness(void) {
    u64 n = 1;
    if (*(u8 *)&n == 1) {
        return "little";
    }
    return "big";
}

const char *get_virtualization(void) {
    // https://en.wikipedia.org/wiki/CPUID#EAX=1:_Processor_Info_and_Feature_Bits
    unsigned int eax, ebx, ecx, edx;

    cpuid(1, &eax, &ebx, &ecx, &edx);
    if (ecx & (1 << 5)) {
        return "VT-x";
    }

    cpuid(0x80000001, &eax, &ebx, &ecx, &edx);
    if (edx & (1 << 2)) {
        return "AMD-V";
    }

    return NULL;
}