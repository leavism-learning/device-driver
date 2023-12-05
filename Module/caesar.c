/**************************************************************
* Class:  CSC-415-01 Fall 2023
* Name: Giahuy Dang
* Student ID: 922722304
* GitHub ID: leavism
* Project: Assignment 6 – Device Driver
*
* File: caesar.c
*
* Description: The program for the Caesar Cipher encryption
* device driver. It takes character input and applies a
* Caesar cipher.
*
**************************************************************/

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>

#define MY_MAJOR 415
#define MY_MINOR 0
#define DEVICE_NAME "caesar"

char* kernel_buffer;

struct cdev my_cdev;

MODULE_AUTHOR("Giahuy Dang");
MODULE_DESCRIPTION("A simple Caesar Cipher program");
MODULE_LICENSE("GPL");

// Tracks how many times data was written
struct writeTracker {
	int count;
} writeTracker;

// Increments writeTracker's count
// Returns how many bytes were passed in.
// 0 is in, 1 is out, 2 is error, 3 is the first file handle
static ssize_t myWrite(struct file* fs, const char __user* buf, size_t hsize, loff_t* off)
{
	struct writeTracker* tracker;
	tracker = (struct writeTracker*) fs->private_data;
	tracker->count = tracker->count + 1;
	printk(KERN_INFO "Wrote %lu on write number %d\n", hsize, ds->count);
	return hsize;
}

static ssize_t myRead(struct file* fs, char __user* buf, size_t hsize, loff_t* off)
{
	struct writeTracker* tracker;
	tracker = (struct writeTracker*) fs->private_data;
	tracker->count = tracker->count + 1;
	printk(KERN_INFO "Read %lu on write number %d\n", hsize, tracker->count);
	return 0;
}

static int myOpen(struct inode* inode, struct file* fs)
{
	struct writeTracker* tracker;
	tracker = vmalloc(sizeof(struct writeTracker));

	if (tracker == 0) {
		printk(KERN_ERR "Failed to vmalloc.\n");
		return -1;
	}

	tracker->count = 0;
	fs->private_data = tracker;
	return 0;
}

static int myClose(struct inode* inode, struct file* fs)
{
	struct writeTracker* tracker;
	tracker = (struct writeTracker*) fs->private_data;
	vfree(tracker);
	return 0;
}

// Handles device files that don't have a read or write
// Counts how many times write was called
static long myIoCtl(struct file* fs, unsigned int command, unsigned long data)
{
	int* count;
	struct writeTracker* tracker;
	tracker = (struct writeTracker*) fs->private_data;
	if (command != 3) {
		printk(KERN_ERR "IoCtl failed.\n");
		return -1;
	}
	count = (int*) data;
	*count = tracker->count;
	return 0;
}

int init_module(void)
{
	int result, registers;
	dev_t devno;

	devno = MKDEV(MY_MAJOR, MY_MINOR);

	registers = register_chrdev_region(devno, 1, DEVICE_NAME);
	printk(KERN_INFO "Register chardev succeeded 1: %d\n", registers);
	cdev_init(&my_cdev, &fops);
}

void cleanup_module(void)
{
	dev_t devno;

	devno = MKDEV(MY_MAJOR, MY_MINOR);
	unregister_chrdev_region(devno, 1);
	cdev_del(&my_cdev);

	vfree(kernel_buffer);
	kernel_buffer = NULL;

	printk(KERN_INFO "Goodbye world.\n");
}

struct file_operations fops = {
				.open = myOpen,
				.release = myClose,
				.write = myWrite,
				.read = myRead,
				.unlocked_ioctl = myIoCtl,
				.owner = THIS_MODULE,
}
