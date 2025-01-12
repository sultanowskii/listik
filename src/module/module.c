#include <asm/cpufeature.h> // x86_cap_flags
#include <asm/processor.h>  // cpu_data(), struct cpuinfo_x86
#include <linux/cpufreq.h>  // cpu frequency
#include <linux/module.h>   // for modules
#include <linux/printk.h>   // pr_*
#include <linux/proc_fs.h>  // procfs
#include <linux/seq_file.h> // seqfile
#include <linux/smp.h>      // get_cpu(), put_cpu()
#include <linux/utsname.h>  // utsname()

#include "cache.h"
#include "info.h"

static const char            *PROCFS_FILENAME = "listik";
static struct proc_dir_entry *procfs_file;

static void *listik_seq_start(struct seq_file *s, loff_t *pos) {
    if (*pos == 0) {
        return (void *)1;
    }
    return NULL;
}

static void *listik_seq_next(struct seq_file *s, void *v, loff_t *pos) {
    (*pos)++;
    return NULL;
}

static void listik_seq_stop(struct seq_file *s, void *v) {
}

static void listik_seq_show_architecture_section(
    struct seq_file    *file,
    struct cpuinfo_x86 *cpu_information
) {
    struct new_utsname *uts = utsname();
    seq_printf(
        file,
        "architecture=%s\n"
        "cpu_modes=%s\n"
        "address_sizes=%u bits physical, %u bits virtual\n"
        "byte_order=%s\n",
        uts->machine,
        CPU_MODES,
        cpu_information->x86_phys_bits, cpu_information->x86_virt_bits,
        get_endianness()
    );
}

static void listik_seq_show_cpu_section(struct seq_file *file) {
    unsigned int cpu;

    seq_printf(
        file,
        "cpus=%d\n"
        "online_cpus=",
        num_online_cpus()
    );
    for_each_online_cpu(cpu) {
        seq_printf(
            file,
            "%d ",
            cpu
        );
    }
    seq_putc(file, '\n');
}

static void listik_seq_show_cpu_info_section(
    struct seq_file       *file,
    struct cpuinfo_x86    *cpu_information,
    struct cpufreq_policy *cpu_frequency_policy,
    unsigned long          current_frequency
) {
    size_t i;

    seq_printf(
        file,
        "vendor_id=%s\n"
        "cpu_family=%d\n"
        "model=%d\n"
        "model_name=%s\n"
        "stepping=%d\n",
        cpu_information->x86_vendor_id,
        cpu_information->x86,
        cpu_information->x86_model,
        cpu_information->x86_model_id,
        cpu_information->x86_stepping
    );

    if (cpu_frequency_policy != NULL) {
        unsigned int max_frequency = cpu_frequency_policy->max;
        unsigned int min_frequency = cpu_frequency_policy->min;
        unsigned int scaling_percent = (current_frequency * 100) / max_frequency;

        seq_printf(
            file,
            "cpus_scaling_mhz=%u\n"
            "cpu_max_mhz=%u\n"
            "cpu_min_mhz=%u\n",
            scaling_percent,
            max_frequency / 1000,
            min_frequency / 1000
        );
    }

    seq_printf(
        file,
        "bogomips=%lu.%02lu\n",
        cpu_information->loops_per_jiffy / (500000 / HZ), (cpu_information->loops_per_jiffy / (5000 / HZ)) % 100
    );

    seq_puts(file, "flags=");
    for (i = 0; i < sizeof(cpu_information->x86_capability) / sizeof(cpu_information->x86_capability[0]); i++) {
        u32 cap = cpu_information->x86_capability[i];
        if (cap) {
            seq_printf(file, "%u ", cpu_information->x86_capability[i]);
        }
    }
    seq_putc(file, '\n');
}

static void listik_seq_show_virtualization_section(struct seq_file *file) {
    const char *virt = get_virtualization();
    if (virt != NULL) {
        seq_printf(file, "virtualization=%s\n", virt);
    }
}

static void _listik_seq_show_caches_section(
    struct seq_file    *file,
    struct cpuinfo_x86 *cpu_information,
    unsigned int        op
) {
    u64 l1d_size = 0;
    u16 l1d_instances = 0;
    u64 l1i_size = 0;
    u16 l1i_instances = 0;
    u64 l2_size = 0;
    u16 l2_instances = 0;
    u64 l3_size = 0;
    u16 l3_instances = 0;

    u16 cntr;

    u8           cache_level;
    u8           cache_type;
    unsigned int eax, ebx, ecx, edx;

    // TODO: calculate cache instances???
    cntr = 0;
    while (true) {
        eax = op;
        ecx = cntr;

        pr_info("eax=%x ebx=%x ecx=%x edx=%x\n", eax, ebx, ecx, edx);
        __cpuid(&eax, &ebx, &ecx, &edx);
        pr_info("eax=%x ebx=%x ecx=%x edx=%x\n", eax, ebx, ecx, edx);

        cache_type = eax & 0b11111;
        if (cache_type == 0) {
            break;
        }
        cache_level = (eax >> 5) & 3;

        pr_info("type=%d level=%d\n", cache_type, cache_level);
        switch (cache_level) {
            case 1: {
                switch (cache_type) {
                    case 1: {
                        l1d_size = get_cache_size(ebx, ecx);
                        break;
                    }
                    case 2: {
                        l1i_size = get_cache_size(ebx, ecx);
                        break;
                    }
                }
                break;
            }
            case 2: {
                switch (cache_type) {
                    case 3: {
                        l2_size = get_cache_size(ebx, ecx);
                        break;
                    }
                }
                break;
            }
            case 3: {
                switch (cache_type) {
                    case 3: {
                        l3_size = get_cache_size(ebx, ecx);
                        break;
                    }
                }
                break;
            }
        }
        cntr++;
    }

    seq_printf(
        file,
        "l1d_size=%llu\n"
        "l1d_instances=%hu\n"
        "l1i_size=%llu\n"
        "l1i_instances=%hu\n"
        "l2_size=%llu\n"
        "l2_instances=%hu\n"
        "l3_size=%llu\n"
        "l3_instances=%hu\n",
        l1d_size,
        l1d_instances,
        l1i_size,
        l1i_instances,
        l2_size,
        l2_instances,
        l3_size,
        l3_instances
    );
}

// https://en.wikipedia.org/wiki/CPUID#EAX=4_and_EAX=8000'001Dh:_Cache_Hierarchy_and_Topology
static void listik_seq_show_caches_section(
    struct seq_file    *file,
    struct cpuinfo_x86 *cpu_information
) {
    unsigned int op;
    if (strcmp(cpu_information->x86_vendor_id, "GenuineIntel") == 0) {
        op = 0x4;
    } else if (strcmp(cpu_information->x86_vendor_id, "AuthenticAMD") == 0) {
        op = 0x8000001D;
    } else {
        pr_info("Failed to retrieve info about caches (unsupported vendor), ignoring\n");
        return;
    }

    _listik_seq_show_caches_section(file, cpu_information, op);
}

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

    // TODO: Caches
    listik_seq_show_caches_section(file, cpu_information);

    // TODO: NUMA

    // TODO: Vulnerabilities

    return 0;
}

static struct seq_operations listik_seq_ops = {
    .start = listik_seq_start,
    .next = listik_seq_next,
    .stop = listik_seq_stop,
    .show = listik_seq_show,
};

static int listik_open(struct inode *inode, struct file *file) {
    return seq_open(file, &listik_seq_ops);
};

static const struct proc_ops proc_file_fops = {
    .proc_open = listik_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release,
};

static int __init listik_init(void) {
    pr_info("hi!\n");

    procfs_file = proc_create(
        PROCFS_FILENAME,
        0644,
        NULL,
        &proc_file_fops
    );

    return 0;
}

static void __exit listik_exit(void) {
    proc_remove(procfs_file);
    pr_info("bye!\n");
}

module_init(listik_init);
module_exit(listik_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Artur Sultanov");
MODULE_DESCRIPTION("Kernel module");
