#include <linux/module.h>	/* We're building a module */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/proc_fs.h>	/* because we use proc fs */
#include <asm/uaccess.h>	/* for copy_from_user */
#include <linux/mm.h>		/* for page_address and kmap */
#include <linux/highmem.h>

#define DRIVER_AUTHOR "FLUX Research Group"
#define DRIVER_DESC   "A driver for memory de-duplication"
#define proc_fn       "singlepage"

extern struct page *mem_map;
extern unsigned long num_physpages;
extern unsigned long max_mapnr;

struct proc_dir_entry *proc_file = NULL;
static unsigned long pageid = 0;
char debug = 0;

int
procfile_read(char *buffer, char **start,
	      off_t offset, int count, int *peof, void *dat)
{
	struct page *page = &mem_map[pageid];
	void *virt = page_address(page);
	char mapped = 0;
	int i = 0;
	if (debug == 1)
		printk(KERN_DEBUG "offset: %lu, count: %d\n", offset, count);

	if (offset >= PAGE_SIZE) {
		printk(KERN_INFO "reach file end!\n");
		*peof = 1;
		return 0;
	}

	if (virt == NULL) {
		virt = kmap(page);
		if (virt == NULL) {
			printk(KERN_ALERT "Fail to map highmem!");
			return 0;
		}
		mapped = 1;
	}

	if (count > PAGE_SIZE - offset) {
		count = PAGE_SIZE - offset;
		*peof = 1;
	}

	if (debug == 1)
		printk(KERN_DEBUG "offset: %lu, count: %d\n", offset, count);

	i = 0;
	while (i < count) {
		*(unsigned char *) (buffer + i) =
		    *((unsigned char *) virt + offset + i);
		printk("%02X", *((unsigned char *) buffer + i));
		i++;
	}

	if (mapped == 1) {
		kunmap(page);
	}

	*(int *) start = count;
	return count;
}

int
procfile_write(struct file *file, const char *buffer, unsigned long count,
	       void *data)
{
	unsigned long bytes_not_copied = 0;
	// page id is copied into kernel as a string. 
	char page_id[20] = { 0 };

	bytes_not_copied = copy_from_user(page_id, buffer, count);
	if (bytes_not_copied != 0) {
		printk(KERN_ALERT "%lu bytes not copied!", bytes_not_copied);
		return -EFAULT;
	}
	pageid = simple_strtoul(page_id, NULL, 10);
	if (pageid >= num_physpages) {
		printk(KERN_ALERT "pageid >= max pageid %lu!\n", num_physpages);
		return -EFAULT;
	}
	printk(KERN_INFO "page id is %lu!\n", pageid);
	return count;
}

static int __init
lkp_init(void)
{
	printk(KERN_INFO "Hello from memory de-duplication module\n");
	printk("num_physpages: %lu, max_mapnr: %lu\n", num_physpages,
	       max_mapnr);
	proc_file = create_proc_entry(proc_fn, 0666, NULL);
	if (proc_file == NULL) {
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
		       proc_fn);
		return -ENOMEM;
	}

	proc_file->read_proc = procfile_read;
	proc_file->write_proc = procfile_write;
	printk(KERN_INFO "/proc/%s created\n", proc_fn);
	return 0;
}

static void __exit
lkp_cleanup(void)
{
	remove_proc_entry(proc_fn, NULL);
	printk(KERN_INFO "Exit from memory de-duplication module\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

module_init(lkp_init);
module_exit(lkp_cleanup);
