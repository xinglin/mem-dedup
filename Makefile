#
# Modules and the files they are made from
#
PAGEINFO = pageinfo
SINGLEPAGE = singlepage
MODULES = $(PAGEINFO) $(SINGLEPAGE)
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

# insmod can't be used to install multiple modules at once. So create 
# this to install all modules at once.
install: pageinfo_install singlepage_install

pageinfo_install: $(PAGEINFO).ko
	insmod $(PAGEINFO).ko

singlepage_install: $(SINGLEPAGE).ko
	insmod $(SINGLEPAGE).ko

deinstall:
	rmmod $(KOFILES)

reinstall: deinstall install

.PHONY: install deinstall reinstall clean
