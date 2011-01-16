#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <linux/mm.h>
#include <asm/e820.h>
//#include <linux/mmzone.h>

#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/err.h>

#define DRIVER_AUTHOR "FLUX Research Group"
#define DRIVER_DESC   "A driver for memory de-duplication - UNMA"
#define proc_fn       "pageinfo-NUMA"

extern unsigned long num_physpages;
extern int e820_any_mapped(u64 start, u64 end, unsigned type);
static unsigned long pfn = 0;
char debug = 1;

/* This function is called at the beginning of a sequence.
 * ie, when:
 *	- the /proc file is read (first time)
 *	- after the function stop (end of sequence)
 *  pos is the position in /proc/filename. 
 */
static void *
de_seq_start(struct seq_file *s, loff_t * pos)
{
	if (debug == 1) {
		printk(KERN_INFO "start: seq_start, pos: %8lu\n",
	    	   *(unsigned long *) pos);
	}
	
	pfn = *(unsigned long *) pos;
	while(  !pfn_valid(pfn) && pfn < num_physpages) {
		pfn ++;
	}

	if (pfn < num_physpages) {
		/* begin the sequence. */
		*pos = pfn;
		return pfn_to_page(pfn);
	} else {
		printk(KERN_INFO "seq_start: %8lu > num_physpages\n", pfn);
		*pos = 0;
		return NULL;
	}
};

/*
 * This function is called after the beginning of a sequence.
 * It's called until the return is NULL (this ends the sequence).
 */
static void *
de_seq_next(struct seq_file *s, void *v, loff_t * pos)
{
	pfn = *pos + 1;

	while(  !pfn_valid(pfn) && pfn < num_physpages) {
		pfn ++;
	}
	*pos = pfn;

	if (debug == 1)
		printk(KERN_INFO "seq_next, pfn:%8lu\n", pfn);

	if (pfn >= num_physpages) {
		printk(KERN_INFO "seq_next: %8lu >= num_physpages!\n", pfn);
		return NULL;
	}
	return pfn_to_page(pfn);
}

/*
 * This function is called for each "step" of a sequence
 */
static int
de_seq_show(struct seq_file *s, void *v)
{
	struct page *page = (struct page *) v;
	u64 va = (u64)page_address(page);
	u64 pa = __pa(va);
	struct scatterlist sg;
	struct crypto_hash *tfm;
	struct hash_desc desc;
	unsigned char result[20];
	if (debug == 1)
		printk(KERN_INFO "seq_show: %8lu\n", pfn);

#ifdef SKIP_UNUSED_PAGES
	if (page_count(page) == 0) {
		if (debug == 1)
			printk(KERN_INFO "skipping page: %8lu\n", pfn);
		return 1;
	}
#endif

	if (va == 0) {
		printk(KERN_ALERT "This should not happen in 64-bit machine\n");
		return 1;
	}

	if( !(e820_any_mapped(pa, pa+PAGE_SIZE-1, E820_RAM) ||
		e820_any_mapped(pa, pa+PAGE_SIZE-1, E820_RESERVED_KERN)) ){
		printk(KERN_INFO "%8lu not usable\n", pfn);
		return 1;
	}
	/* get hash of this page */
	tfm = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		printk(KERN_ALERT "Fail to allocate transformer object!");
		return -EFAULT;
	}
	sg_init_table(&sg, 1);
	sg_set_page(&sg, page, PAGE_SIZE, 0);
	desc.tfm = tfm;
	desc.flags = 0;
	if (crypto_hash_digestsize(tfm) > sizeof (result)) {
		printk("digest size(%u) > outputbuffer(%zu)\n",
		       crypto_hash_digestsize(tfm), sizeof (result));
		return -EFAULT;
	}

	if (crypto_hash_digest(&desc, &sg, PAGE_SIZE, (u8 *) result)) {
		printk(KERN_ALERT "Fail to call digest function!");
		return -EFAULT;
	}
	crypto_free_hash(tfm);
	seq_printf(s, "%8lu: 0x%lx, "
		   "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
		   "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x,"
		   " %d, %d, %d, %d\n",
		   pfn, (unsigned long) va,
		   result[0], result[1], result[2], result[3], result[4],
		   result[5], result[6], result[7], result[8], result[9],
		   result[10], result[11], result[12], result[13], result[14],
		   result[15], result[16], result[17], result[18], result[19],
		   page_count(page), PageActive(page),
		   PageReserved(page), PageReclaim(page));

	return 0;
}

/*
 * This function is called at the end of a sequence
 */
static void
de_seq_stop(struct seq_file *s, void *v)
{
	if (debug == 1){
		printk(KERN_INFO "seq_read finished \n");
	}
}

static struct seq_operations de_seq_ops = {
	.start = de_seq_start,
	.next = de_seq_next,
	.stop = de_seq_stop,
	.show = de_seq_show
};

static int
proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &de_seq_ops);
};

static struct file_operations file_ops = {
	.owner = THIS_MODULE,
	.open = proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release
};

static int __init
lkp_init(void)
{
	struct proc_dir_entry *proc_file = NULL;

	printk(KERN_INFO "Hello from memory de-duplication module for NUMA\n");
	printk("num_physpages: %lu\n", num_physpages);
	proc_file = create_proc_entry(proc_fn, 0644, NULL);
	if (proc_file == NULL) {
		printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
		       proc_fn);
		return -ENOMEM;
	}
	proc_file->proc_fops = &file_ops;
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
