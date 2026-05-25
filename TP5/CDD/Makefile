obj-m := sensor_driver.o

KDIR  := /lib/modules/$(shell uname -r)/build
BUILD := $(PWD)/build

all:
	mkdir -p $(BUILD)
	make -C $(KDIR) M=$(PWD) modules
	find $(PWD) -maxdepth 1 -name "*.o" -o -name "*.ko" -o -name "*.mod*" -o -name "*.order" -o -name "*.symvers" -o -name "*.cmd" | xargs -I{} mv -f {} $(BUILD)/ 2>/dev/null || true
	find $(PWD) -maxdepth 1 -name ".*.cmd" | xargs -I{} mv -f {} $(BUILD)/ 2>/dev/null || true

clean:
	rm -rf $(BUILD)
	make -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod $(BUILD)/sensor_driver.ko
	sudo chmod 666 /dev/sensor_driver

unload:
	sudo rmmod sensor_driver

log:
	sudo dmesg | tail -15