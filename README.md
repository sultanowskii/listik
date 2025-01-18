# listik

`lscpu` parody.

Info is gathered in the kernel space (`listik.ko` module) and then it's exposed via procfs (`/proc/listik`) so that user space CLI (`./listik`) could read it and print it.

## Build

Kernel 'toolkit' is required to build a module. The location - `/usr/lib/modules/$(shell uname -r)/build`

```bash
make
```

## Usage

Once built, use it like this:

```bash
❯ # load kernel module 
❯ make load
sudo insmod listik.ko;
❯ # check that it's there 
❯ lsmod | grep listik
listik                 16384  0
❯ # run
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

## Limitations

- AMD/Intel-only
- Not sure it's going to work with big endian
- Some info `lscpu` provides is omitted/ignored
- CPU flags are bin-only. Kernel doesn't seem to grant access to the `x86_cap_flags` (an array of CPU features' string representations), and I don't want to manually write and hardcode all that
- Regarding CPU vulnerabilities, I've included only ones that `lscpu` showed me on my machine

## Note

This project is a part of OS course @ ITMO. I've managed to do 1st and 2nd assignments in zig, which I quite enjoyed.

At first, I have tried to write this on in `Zig` too, but the hassle turned out to far outweighs the benefits.

Too much c-to-zig glue is required - to the point where `VSCode` shows dumb errors, doesn't provide hints and the 'programming' part feels like I'm still writing in C, but with few extra steps - so, what's the point?
