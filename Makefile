obj-m += cdm.o

KBUILD = /lib/modules/$(shell uname -r)/build/

default:
	$(MAKE) -C $(KBUILD) M=$(PWD) modules

clean:
	$(MAKE) -C $(KBUILD) M=$(PWD) clean	
