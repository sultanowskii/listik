# Лабораторная работа №3

Вариант: `Linux`, `lscpu`, `procfs`

ФИО: `Султанов Артур Радикович`

Группа: `P3313`

Преподаватель: `Смирнов Виктор Игоревич`

## Текст задания

Разработать комплекс программ на пользовательском уровне и уровне ярда, который собирает информацию на стороне ядра и передает информацию на уровень пользователя, и выводит ее в удобном для чтения человеком виде. Программа на уровне пользователя получает на вход аргумент(ы) командной строки (не адрес!), позволяющие идентифицировать из системных таблиц необходимый путь до целевой структуры, осуществляет передачу на уровень ядра, получает информацию из данной структуры и распечатывает структуру в стандартный вывод. Загружаемый модуль ядра принимает запрос через указанный в задании интерфейс, определяет путь до целевой структуры по переданному запросу и возвращает результат на уровень пользователя.

Интерфейс передачи между программой пользователя и ядром и целевая структура задается преподавателем. Интерфейс передачи может быть один из следующих:

- syscall - интерфейс системных вызовов.
- ioctl - передача параметров через управляющий вызов к файлу/устройству.
- procfs - файловая система /proc, передача параметров через запись в файл.
- debugfs - отладочная файловая система /sys/kernel/debug, передача параметров через запись в файл.

Целевая структура может быть задана двумя способами:

- Именем структуры в заголовочных файлах Linux
- Файлом в каталоге /proc. В этом случае необходимо определить целевую структуру по пути файла в /proc и выводимым данным.

## Обзор кода

### Модуль

В `cache.c` / `cache.h` содержатся вспомогательные функции для получения информации о кэшах - на основе значений регистров после выполнения системного вызова `cpuid`. Примеры:

```c
enum cache_type get_cache_type(unsigned int eax) {
    return eax & 0b11111;
}

u16 get_ways_of_cache_associativity(unsigned int ebx) {
    return ((ebx >> 22) & 0b1111111111) + 1;
}
```

Информация об этом системном вызове и его специфике была взята с:

- [https://en.wikipedia.org/wiki/CPUID](https://en.wikipedia.org/wiki/CPUID)
- [https://github.com/tpn/pdfs/blob/master/AMD%20-%20CPUID.pdf](https://github.com/tpn/pdfs/blob/master/AMD%20-%20CPUID.pdf)

В `info.c` / `info.h` - вспомогательные функции / макросы для получения различной информации о процессоре и архитектуре. Пример:

```c
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
    if (edx & (1 << 1)) {
        return "AMD-V";
    }

    return NULL;
}
```

В `module.c` содержится основная логика модуля ядра. В `listik_init()` создается "файл" в `procfs`, у которого операции настроены на `seq_file` - механизм для упрощения вывода.

Логика вывода - в функции `listik_seq_show()`. Тело:

```c
static int listik_seq_show(struct seq_file *file, void *v) {
    unsigned long          current_frequency;
    unsigned int           cpu;
    struct cpuinfo_x86    *cpu_information;
    struct cpufreq_policy *cpu_frequency_policy;

    cpu = get_cpu();

    cpu_frequency_policy = cpufreq_cpu_get(cpu);
    if (cpu_frequency_policy == NULL) {
        pr_info("Failed to get cpufreq policy, ignoring\n");
    }
    current_frequency = cpufreq_get(cpu);

    cpu_information = &cpu_data(cpu);
    if (cpu_information == NULL) {
        pr_err("Failed to load cpu data\n");
        put_cpu();
        return -1;
    }

    if (cpu_frequency_policy != NULL) {
        cpufreq_cpu_put(cpu_frequency_policy);
    }
    put_cpu();

    // Architecture
    listik_seq_show_architecture_section(file, cpu_information);

    // CPU(s)
    listik_seq_show_cpu_section(file);

    // Vendor ID
    listik_seq_show_cpu_info_section(file, cpu_information, cpu_frequency_policy, current_frequency);

    // Virtualization features
    listik_seq_show_virtualization_section(file);

    // Caches
    listik_seq_show_caches_section(file, cpu_information);

    // NUMA
    listik_seq_show_numa_section(file);

    // Vulnerabilities
    listik_seq_show_vulnerabilities(file, cpu_information);

    return 0;
}
```

Она получает информацию о процессоре и далее вызывает дополнительные функции, которые печатают информацию о различных аспектах (по аналогии с `lscpu`).

### Утилита командной строки

Ее задача - чтение "файла" (`/proc/listik`) и вывод содержимого на экран. Код:

```c
#include <errno.h>
#include <stdio.h>

int main() {
    FILE *file = fopen("/proc/listik", "r");
    if (file == NULL) {
        perror("Can't open the /proc/listik");
        return errno;
    }

    char buffer[16];

    while (1) {
        size_t fread_result = fread(buffer, sizeof(char), 16, file);
        if (fread_result == 0) {
            if (feof(file)) {
                break;
            } else {
                perror("Error reading /proc/listik");
                return errno;
            }
        }

        size_t fwrite_result = fwrite(buffer, sizeof(char), fread_result, stdout);
        if (fwrite_result == 0) {
            perror("Can't write to stdout");
            return errno;
        }
    }

    return 0;
}
```

## Пример запуска и сравнение с `lscpu`

```bash
❯ lscpu
Architecture:             x86_64
  CPU op-mode(s):         32-bit, 64-bit
  Address sizes:          39 bits physical, 48 bits virtual
  Byte Order:             Little Endian
CPU(s):                   8
  On-line CPU(s) list:    0-7
Vendor ID:                GenuineIntel
  Model name:             11th Gen Intel(R) Core(TM) i7-1165G7 @ 2.80GHz
    CPU family:           6
    Model:                140
    Thread(s) per core:   2
    Core(s) per socket:   4
    Socket(s):            1
    Stepping:             1
    CPU(s) scaling MHz:   22%
    CPU max MHz:          4700.0000
    CPU min MHz:          400.0000
    BogoMIPS:             5608.00
    Flags:                fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pa
                          t pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall 
                          nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_
                          good nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq 
                          pni pclmulqdq dtes64 monitor ds_cpl vmx est tm2 ssse3 sdbg fma c
                          x16 xtpr pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadlin
                          e_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpui
                          d_fault epb cat_l2 invpcid_single cdp_l2 ssbd ibrs ibpb stibp ib
                          rs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbas
                          e tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdt_a avx512f avx5
                          12dq rdseed adx smap avx512ifma clflushopt clwb intel_pt avx512c
                          d sha_ni avx512bw avx512vl xsaveopt xsavec xgetbv1 xsaves split_
                          lock_detect dtherm ida arat pln pts hwp hwp_notify hwp_act_windo
                          w hwp_epp hwp_pkg_req avx512vbmi umip pku ospke avx512_vbmi2 gfn
                          i vaes vpclmulqdq avx512_vnni avx512_bitalg avx512_vpopcntdq rdp
                          id movdiri movdir64b fsrm avx512_vp2intersect md_clear flush_l1d
                           arch_capabilities
Virtualization features:  
  Virtualization:         VT-x
Caches (sum of all):      
  L1d:                    192 KiB (4 instances)
  L1i:                    128 KiB (4 instances)
  L2:                     5 MiB (4 instances)
  L3:                     12 MiB (1 instance)
NUMA:                     
  NUMA node(s):           1
  NUMA node0 CPU(s):      0-7
Vulnerabilities:          
  Gather data sampling:   Mitigation; Microcode
  Itlb multihit:          Not affected
  L1tf:                   Not affected
  Mds:                    Not affected
  Meltdown:               Not affected
  Mmio stale data:        Not affected
  Reg file data sampling: Not affected
  Retbleed:               Not affected
  Spec rstack overflow:   Not affected
  Spec store bypass:      Mitigation; Speculative Store Bypass disabled via prctl and secc
                          omp
  Spectre v1:             Mitigation; usercopy/swapgs barriers and __user pointer sanitiza
                          tion
  Spectre v2:             Vulnerable: eIBRS with unprivileged eBPF
  Srbds:                  Not affected
  Tsx async abort:        Not affected
❯
❯
❯ ./listik
architecture=x86_64
cpu_modes=32-bit, 64-bit
address_sizes=39 bits physical, 48 bits virtual
byte_order=little
cpus=8
online_cpus=0 1 2 3 4 5 6 7 
vendor_id=GenuineIntel
cpu_family=6
model=140
model_name=11th Gen Intel(R) Core(TM) i7-1165G7 @ 2.80GHz
stepping=1
cpus_scaling_mhz=0
cpu_max_mhz=4700
cpu_min_mhz=400
bogomips=5608.00
flags=11111111110111111101011111111101 00000000000100000000100000110100 00000000101111011010111011001001 11111101110111110101111111111110 10000100100000000000000000000000 01010101000000011101010001110011 11111000000000000100000000000000 11010111111001011111110111001111 11110000000000000000000000000000 00001110000000000100000000000000 11101111111101111110100000000000 01111011111110100000001100011000 00001000111000000000100000111111 01001000000000000000000000000000 
virtualization=VT-x
l1d_size=49152
l1i_size=32768
l2_size=1310720
l3_size=12582912
numa_nodes=1
numa_node0_cpus=0 1 2 3 4 5 6 7 
vulnerabilities=gather_data_sampling spec_store_bypass spectre_v1 spectre_v2
```

Как видно, вывод моей реализации упрощен, при том информация вполне идентична - с небольшими погрешностями на формат вывода (особенно флагов).
