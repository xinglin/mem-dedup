#include <linux/module.h>	/* We're building a module */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/proc_fs.h>	/* because we use proc fs */
#include <asm/uaccess.h>	/* for copy_from_user */
#include <linux/mm.h>		/* for page_address and kmap */
#include <linux/highmem.h>

#define DRIVER_AUTHOR "FLUX Research Group"
#define DRIVER_DESC   "A driver for memory de-duplication"
#define proc_fn       "singlepage"

extern unsigned long num_physpages;

struct proc_dir_entry *proc_file = NULL;
static unsigned long pageid = 0;
char debug = 0;

int
procfile_read(char *buffer, char **start,
	      off_t offset, int count, int *peof, void *dat)
{
	struct page *page = pfn_to_page(pageid);
	void *virt = page_address(page);
	int i = 0;
	if (debug == 1)
		printk(KERN_DEBUG "offset: %lu, count: %d\n", offset, count);

	if (offset >= PAGE_SIZE) {
		printk(KERN_INFO "reach file end!\n");
		*peof = 1;
		return 0;
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

	*(int *) start = count;
	return count;
}

void show_flags(struct page * page){
	if( PageLocked(page) ){
		printk(KERN_INFO "locked\t");
	}
	if( PageMlocked(page) ){
		printk(KERN_INFO "mlocked\t");
	}
	if( PageUnevictable(page) ){
		printk(KERN_INFO "unevictable\t");
	}
	if( PageHWPoison(page) ){
		printk(KERN_INFO "hwpoison\t");
	}
	if( PageError(page) ){
		printk(KERN_INFO "error\t");
	}
	if( PageActive(page) ){
		printk(KERN_INFO "active\t");
	}
	if( PageDirty(page) ){
		printk(KERN_INFO "dirty\t");
	}
	if( PagePrivate(page) ){
		printk(KERN_INFO "private\t");
	}
	if( PageReferenced(page) ){
		printk(KERN_INFO "referenced\t");
	}
	if( PageUptodate(page) ){
		printk(KERN_INFO "uptodate\t");
	}
	if( PageWriteback(page) ){
		printk(KERN_INFO "writeback\t");
	}
	if( PageSwapCache(page) ){
		printk(KERN_INFO "swapcache\t");
	}
	if( PageLRU(page) ){
		printk(KERN_INFO "lru\t");
	}
	if( PageSlab(page) ){
		printk(KERN_INFO "slab\t");
	}
	if( PageBuddy(page) ){
		printk(KERN_INFO "buddy\t");
	}
	if( PageChecked(page) ){
		printk(KERN_INFO "checked\t");
	}
	if( PageSwapBacked(page) ){
		printk(KERN_INFO "swapbacked\t");
	}
	if( PageSlobFree(page) ){
		printk(KERN_INFO "swapbacked\t");
	}
	if( PageSlubFrozen(page) ){
		printk(KERN_INFO "slubfrozen\t");
	}
	if( PageSlubDebug(page) ){
		printk(KERN_INFO "slubdebug\t");
	}
	if( PagePrivate2(page) ){
		printk(KERN_INFO "private2\t");
	}
	if( PageOwnerPriv1(page) ){
		printk(KERN_INFO "ownerpriv1\t");
	}
	if( PageMappedToDisk(page) ){
		printk(KERN_INFO "mappedtodisk\t");
	}
	if( PageReadahead(page) ){
		printk(KERN_INFO "readahead\t");
	}
	if( PageReclaim(page) ){
		printk(KERN_INFO "reclaim\t");
	}
	printk(KERN_INFO "show flags done\n");
}

int
procfile_write(struct file *file, const char *buffer, unsigned long count,
	       void *data)
{
	unsigned long bytes_not_copied = 0;
	// page id is copied into kernel as a string. 
	char page_id[20] = { 0 };

	u64 pa = 0;

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
	if( !pfn_valid(pageid) ){
		printk(KERN_ALERT "pageid %lu not valid\n", pageid);
		pageid = 0;
		return -EFAULT;
	}
	
	pa = __pa(page_address(pfn_to_page(pageid)));
	if( !(	e820_any_mapped(pa, pa+PAGE_SIZE-1, E820_RAM) ||
			e820_any_mapped(pa, pa+PAGE_SIZE-1, E820_RESERVED_KERN) ) ){
		printk(KERN_ALERT "pageid %lu is not usable as system RAM!", pageid);
		return -EFAULT;
	}
 	
	printk(KERN_INFO "page id is %lu!\n", pageid);
	show_flags( pfn_to_page(pageid) );
	return count;
}

static int __init
lkp_init(void)
{
	printk(KERN_INFO "Hello from singlepage module\n");
	printk("num_physpages: %lu\n", num_physpages);
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
