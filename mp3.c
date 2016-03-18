#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/mutex.h> 
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include "mp3_given.h"

#define DEBUG 1

#define TSK_SLEEPING 0
#define TSK_RUNNING 1
#define TSK_READY 2

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group_06");
MODULE_DESCRIPTION("CS-423 MP3");

// Proc file
static struct proc_dir_entry *proc_dir;
static struct proc_dir_entry *proc_entry;

// IO buffers
static char input_buf[80];
static char output_buf[160];

// Task list
LIST_HEAD(task_list);

// Locks
DEFINE_MUTEX(list_lock);

/*
 * Registers task to list from global input buffer
 */
void mp3_register(void) {
    //TODO: Implement
    int pid; 

	// Read new task from input buffer
	sscanf(input_buf, "R, %d", &pid);
}

/*
 * Deregisters a task given by global input buffer
 */
void mp3_deregister(void) {
    //TODO: implement 
	int pid;
	sscanf(input_buf, "D, %d", &pid);
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
 * Static procfile struct
 */
static const struct file_operations mp3_file = {
	.owner = THIS_MODULE,
	.read = mp3_read,
	.write = mp3_write,
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

	// Clean cache

	printk(KERN_ALERT "MP3 MODULE UNLOADED\n");
}

// Register init and exit funtions
module_init(mp3_init);
module_exit(mp3_exit);
