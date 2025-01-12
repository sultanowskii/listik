#include <linux/kernel.h>
#include <linux/module.h>   // for modules
#include <linux/printk.h>   // pr_*
#include <linux/proc_fs.h>  // procfs
#include <linux/seq_file.h> // seqfile
#include <linux/smp.h>      // get_cpu(), put_cpu()
#include <asm/processor.h>  // cpu_data(), struct cpuinfo_x86
#include <asm/cpufeature.h> // x86_cap_flags
#include <linux/utsname.h>  // utsname()
#include <linux/cpu.h>
#include <linux/cpufreq.h>

#if defined(__x86_64__) || defined(__aarch64__)
    #define CPU_MODES "32-bit, 64-bit"
#elif defined(__i386__) || defined(__arm__)
    #define CPU_MODES "32-bit"
#else
    #define CPU_MODES "64-bit"
#endif

static const char *get_endianness(void) {
    u64 n = 1;
    if (*(u8 *)&n == 1) {
        return "little";
    }
    return "big";
}

static const char *PROCFS_FILENAME = "listik";
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

static const char *get_virtualization(void) {
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

static int listik_seq_show(struct seq_file *file, void *v) {
    unsigned int cpu = get_cpu();

    struct cpufreq_policy *cpufreq_policy = cpufreq_cpu_get(cpu);
    if (cpufreq_policy == NULL) {
        pr_info("Failed to get cpufreq policy, ignoring...\n");
    }
    unsigned long current_frequency = cpufreq_get(cpu);

    struct cpuinfo_x86 *cpu_information = &cpu_data(cpu);
    if (cpu_information == NULL) {
        pr_err("Failed to load cpu data\n");
        put_cpu();
        return -1;
    }

    if (cpufreq_policy != NULL) {
        cpufreq_cpu_put(cpufreq_policy);
    }
    put_cpu();

    struct new_utsname *uts = utsname();

    // Architecture
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

    // CPU(s)
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

    // Vendor ID
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


    if (cpufreq_policy != NULL) {
        unsigned int max_frequency = cpufreq_policy->max;
        unsigned int min_frequency = cpufreq_policy->min;
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
        cpu_information->loops_per_jiffy / (500000 / HZ), (cpu_information->loops_per_jiffy/ (5000 / HZ)) % 100
    );

    seq_puts(file, "flags=");
    size_t i;
    for (i = 0; i < sizeof(cpu_information->x86_capability) / sizeof(cpu_information->x86_capability[0]); i++) {
        u32 cap = cpu_information->x86_capability[i];
        if (cap) {
            seq_printf(file, "%u ", cpu_information->x86_capability[i]);
        }
    }
    seq_putc(file, '\n');

    // Virtualization features
    const char *virt = get_virtualization();
    if (virt != NULL) {
        seq_printf(file, "virtualization=%s\n", virt);
    }

    // TODO: Caches

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
