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
static ssize_t
myWrite(struct file* fs, const char __user* buf, size_t hsize, loff_t* off)
{
	struct writeTracker* tracker;
	tracker = (struct writeTracker*) fs->private_data;

	tracker->count = tracker->count + 1;

	printk(KERN_INFO "Wrote %lu on write number %d\n", hsize, tracker->count);
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

static int encrypt(int key)
{
	int i;
	int length = strlen(kernel_buffer);
	for (i = 0; i < length - 1; i++) {
		char ch = kernel_buffer[i];

		if (ch >= 'A' && ch <= 'Z') {// uppercase letters
			ch = 'A' + ((ch - 'A' + key) % 26);
		} else if (ch >= 'a' && ch <= 'z') {// lowercase letters
			ch = 'a' + ((ch - 'a' + key) % 26);
		} else if (ch >= '0' && ch <= '9') {// digits 0 - 9
			ch = (ch - '0' - key + 10) % 10 + '0';
		}
	}

	printk(KERN_INFO "Encrypted Text:\n%s\n", kernel_buffer);
	return 0;
}

static long myIoCtl(struct file* fs, unsigned int command, unsigned long data)
{
}

struct file_operations fops = {
				.open = myOpen,
				.release = myClose,
				.write = myWrite,
				.read = myRead,
				.unlocked_ioctl = myIoCtl,
				.owner = THIS_MODULE,
};

int init_module(void)
{
	int result, registers;
	dev_t deviceNum;

	// Combines major and minor to produce a device ID
	deviceNum = MKDEV(MY_MAJOR, MY_MINOR);

	registers = register_chrdev_region(deviceNum, 1, DEVICE_NAME);
	printk(KERN_INFO "Register chardev succeeded 1: %d\n", registers);
	cdev_init(&my_cdev, &fops);

	result = cdev_add(&my_cdev, deviceNum, 1);

	return result;
}

void cleanup_module(void)
{
	dev_t deviceNum;

	deviceNum = MKDEV(MY_MAJOR, MY_MINOR);
	unregister_chrdev_region(deviceNum, 1);
	cdev_del(&my_cdev);

	vfree(kernel_buffer);
	kernel_buffer = NULL;

	printk(KERN_INFO "Goodbye world.\n");
}
