#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h> 
#include <linux/workqueue.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include "mp3_given.h"

#define DEBUG 1

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_06");
MODULE_DESCRIPTION("CS-423 MP3");

// Proc file
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;

// Character device driver
static struct cdev mp3_cdev;
static dev_t mp3_dev_no;

// Cache
struct kmem_cache * mp3_cachep;

// IO buffers
static char input_buf[80];
static char output_buf[160];

// profile buffer
unsigned long *profile_buffer;

size_t write_offset = 0;

// Work queue
static struct workqueue_struct *wq;

struct delayed_work dwork;

int list_size;

// Task list
LIST_HEAD(task_list);

// Locks
DEFINE_MUTEX(list_lock);

struct mp3_task_struct {
	struct task_struct* linux_task;
	struct list_head list;

	int pid;
	int cpu_use;
	int major_fault_count;
	int minor_fault_count;
};

void work_callback(struct work_struct *work) {
	unsigned long minor_fault_count, major_fault_count, cpu_util;
	minor_fault_count = major_fault_count = cpu_util = 0;

	struct mp3_task_struct *iter;
	unsigned long min_fault, maj_fault, u_time, s_time;
	
	// Lock list, traverse 'safe' because we remove entry
	mutex_lock_interruptible(&list_lock);
    list_for_each_entry(iter, &task_list, list) {
    	int fail = get_cpu_use(iter->pid, &min_fault, &maj_fault, &u_time, &s_time);
		if (!fail) {
			// Get data for process
			cpu_util += 100 * (cputime_to_jiffies(u_time) + cputime_to_jiffies(s_time)) / msecs_to_jiffies(50);
			minor_fault_count += min_fault;
			major_fault_count += maj_fault;
		}
	}
	mutex_unlock(&list_lock); // Unlock list

	// Write to buffer
	profile_buffer[write_offset] = minor_fault_count;
	profile_buffer[write_offset + 1] = major_fault_count;
	profile_buffer[write_offset + 2] = cpu_util;
	profile_buffer[write_offset + 3] = jiffies;
	write_offset = (write_offset + 4) % 48000;

	// Sched delayed work
	INIT_DELAYED_WORK(&dwork, work_callback);
	queue_delayed_work(wq, &dwork, msecs_to_jiffies(50));
}

/*
 * Registers task to list from global input buffer
 */
void mp3_register(void) {
    int pid;
	struct mp3_task_struct * new_task_entry;
	// Read new task from input buffer
	sscanf(input_buf, "R, %d", &pid);

	new_task_entry = kmem_cache_alloc(mp3_cachep, GFP_KERNEL);

	new_task_entry -> pid = pid;
	new_task_entry -> cpu_use = 0;
	new_task_entry -> major_fault_count = 0;
	new_task_entry -> minor_fault_count = 0;

	new_task_entry->linux_task = find_task_by_pid(new_task_entry->pid);

	mutex_lock_interruptible(&list_lock);
	if (!list_size) {
		// Create work queue
		wq = create_singlethread_workqueue("mp3_wq");
		INIT_DELAYED_WORK(&dwork, work_callback);

		queue_delayed_work(wq,&dwork,msecs_to_jiffies(50));
	}
	list_add(&new_task_entry->list, &task_list);
	list_size += 1;
	mutex_unlock(&list_lock);
}

/*
 * Deregisters a task given by global input buffer
 */
void mp3_deregister(void) {
    //TODO: implement 
	int pid;
	sscanf(input_buf, "D, %d", &pid);

	struct list_head *pos, *q;
	struct mp3_task_struct *curr;

	mutex_lock_interruptible(&list_lock);
	list_for_each_safe(pos, q, &task_list) {
		curr = list_entry(pos, struct mp3_task_struct, list);
		if(curr->pid == pid) {

			list_del(pos);
			kmem_cache_free(mp3_cachep, curr);
			list_size -= 1;

			if (!list_size) {
				cancel_delayed_work(&dwork);
				flush_workqueue(wq);
				destroy_workqueue(wq);
			}

		}
	}
	mutex_unlock(&list_lock);
}

/*
 * Write function reads to input buffer and passes to helper function
 */
static ssize_t mp3_write(struct file *file, const char __user *buffer, size_t count, loff_t *data) {
	// Read to input buffer
	if (copy_from_user(input_buf, buffer, count) ) {
		return -EFAULT;
	}

	// Call appropriate action function
	switch (input_buf[0]) {
		case 'R':
			mp3_register();
			break;
		case 'U':
			mp3_deregister();
			break;
		default:
			printk(KERN_ALERT "invalid write to proc/mp3/status\n");
			break;
	}

	// Clear input buffer to prevent trailing residual input
	memset(input_buf, 0, 80);
	return count;
}

/*
 * Read function to user app, prints contents of task list to output buffer
 */
static ssize_t mp3_read(struct file *file, char __user *buffer, size_t count, loff_t *data) {
    //TODO: Implement 
	size_t size;
	
	// If requesting extra data, return 0 to stop
	if(*data > 0)
		return 0;

	size = 0;
	
	// Set last character to end of string, return size of buffer
	output_buf[size] = '\0';
	copy_to_user(buffer, output_buf, size+1);
	*data += size;
	return size + 1;
}

/*
 * Character device driver
 */

static int cdev_mmap(struct file *file, struct vm_area_struct *vm_area) {
	int i;
	for (i = 0; i < 128; i++) {
		unsigned long pfn = vmalloc_to_pfn((char *)profile_buffer + i * PAGE_SIZE);
		remap_pfn_range(vm_area, vm_area->vm_start + i * PAGE_SIZE, pfn, PAGE_SIZE, PAGE_SHARED);
	}
	return 0;
}

static int cdev_open(struct inode *inode, struct file *file) {
	return 0;
}

static int cdev_release(struct inode *inode, struct file *file) {
	return 0;
}

/*
 * Static procfile struct
 */
static const struct file_operations mp3_file = {
	.owner = THIS_MODULE,
	.read = mp3_read,
	.write = mp3_write,
};

/* 
 * Static cdev struct 
 */

static const struct file_operations mp3_cdev_fops = {
	.owner = THIS_MODULE,
	.release = cdev_release,
	.open = cdev_open,
	.mmap = cdev_mmap
};

/*
 * mp3_init - Called when module is loaded
 * Initializes the needed data structures
 */
int __init mp3_init(void) {
	#ifdef DEBUG
	printk(KERN_ALERT "MP3 MODULE LOADING\n");
	#endif

	// Create procfile
	proc_dir = proc_mkdir("mp3", NULL); 
	proc_entry = proc_create("status", 0666, proc_dir, &mp3_file);

	// Create char device driver
	alloc_chrdev_region(&mp3_dev_no, 0, 1, "mp3");
	cdev_init(&mp3_cdev, &mp3_cdev_fops);
	cdev_add(&mp3_cdev, mp3_dev_no, 1);

	// Procfile buffer
	profile_buffer = vmalloc(128 * PAGE_SIZE);

	printk(KERN_ALERT "MP3 MODULE LOADED\n");
	return 0;
}

/*
 * mp3_exit - Called when module is unloaded
 * Gets rid of the data structures
 */
void __exit mp3_exit(void) {
	#ifdef DEBUG
	printk(KERN_ALERT "MP3 MODULE UNLOADING\n");
	#endif

	// Remove proc file
	remove_proc_entry("status", proc_dir);
	remove_proc_entry("mp3", NULL);

	// Delete dev driver
	cdev_del(&mp3_cdev);
	unregister_chrdev_region(mp3_dev_no, 1);

	// Clean work queue
	cancel_delayed_work(&dwork);
	flush_workqueue(wq);
	destroy_workqueue(wq);

	printk(KERN_ALERT "MP3 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp3_init);
module_exit(mp3_exit);
