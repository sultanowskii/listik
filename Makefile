MOD          = listik
obj-m       += listik.o
CLI          = listik
listik-objs += src/module/module.o src/module/info.o src/module/cache.o

PWD         := $(CURDIR)
KERNEL_DIR  := /usr/lib/modules/$(shell uname -r)/build

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
	rm $(CLI)

load:
	sudo insmod $(MOD).ko;

unload:
	sudo rmmod $(MOD).ko;

cli: src/cli/main.o
	gcc $^ -o $(CLI)
