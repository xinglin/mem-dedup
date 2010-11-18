#
# Modules and the files they are made from
#
MODULES = hello_seq
CFILES = $(MODULES:=.c)
OBJFILES = $(CFILES:.c=.o)
KOFILES = $(OBJFILES:.o=.ko)

obj-m += $(OBJFILES)

# If set, don't report pages that have no users.
#EXTRA_CFLAGS=-DSKIP_UNUSED_PAGES

# Make sure to set up dependencies between source and object files
%.o: %.c
%.ko: %.o

KVERSION = $(shell uname -r)
all: $(CFILES)
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

#
# Convience targets
#
install: $(KOFILES)
	insmod $(KOFILES)

deinstall:
	rmmod $(KOFILES)

reinstall: deinstall install

.PHONY: install deinstall reinstall clean
