#obj-m += hello.o
obj-m += hello_seq.o

# If set, don't report pages that have no users.
#EXTRA_CFLAGS=-DSKIP_UNUSED_PAGES

KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
