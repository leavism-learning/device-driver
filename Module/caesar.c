/**************************************************************
* Class:  CSC-415-01 Fall 2023
* Name: Giahuy Dang
* Student ID: 922722304
* GitHub ID: leavism
* Project: Assignment 6 – Device Driver
*
* File: caesar.c
*
* Description: The device driver for the Caesar Cipher encryption
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
#define KEY 0  // Default key is no encryption

#define ENCRYPT _IO('e', 0)
#define DECRYPT _IO('e', 1)
#define SETKEY _IO('e', 3)

static int encrypt(int key);
static int decrypt(int key);

char* kernel_buffer;

struct cdev my_cdev;

MODULE_AUTHOR("Giahuy Dang");
MODULE_DESCRIPTION("A simple Caesar Cipher program");
MODULE_LICENSE("GPL");

// Tracks how many times data was written
struct caesarTracker {
	int key;  // Caesar Cipher key
} caesarTracker;

// Copies the user buffer to the kernel buffer.
// Returns how many bytes were passed in, or -1 on failure.
static ssize_t myWrite(struct file* fs, const char __user* buf, size_t hsize, loff_t* off)
{
	int error;
	struct caesarTracker* tracker;

	tracker = (struct caesarTracker*) fs->private_data;
	error = copy_from_user(kernel_buffer + *off, buf, hsize);
	if (error != 0) {
		printk(KERN_ERR "Failed to copy_from_user into the kernel_buffer.\n");
		return -1;
	}
	printk(KERN_INFO "Copied %lu to the kernel buffer.\n", hsize);

	return hsize;
}

// Copies the kernal buffer to the user buffer.
// Returns how many bytes were passed in, or -1 on failure.
static ssize_t myRead(struct file* fs, char __user* buf, size_t hsize, loff_t* off)
{
	int error;
	int length;
	struct caesarTracker* tracker;

	tracker = (struct caesarTracker*) fs->private_data;
	length = strlen(kernel_buffer);

	error = copy_to_user(buf, kernel_buffer + *off, hsize);
	if (error != 0) {
		printk(KERN_ERR "Failed to copy_from_user into the user buffer.\n");
		return -1;
	}

	printk(KERN_INFO "Copied %lu to the user buffer.\n", hsize);
	return 0;
}

// Opens the file.
// Returns 0 on success.
static int myOpen(struct inode* inode, struct file* fs)
{
	struct caesarTracker* tracker;
	tracker = vmalloc(sizeof(struct caesarTracker));

	if (tracker == 0) {
		printk(KERN_ERR "Failed to vmalloc.\n");
		return -1;
	}

	tracker->key = KEY;
	fs->private_data = tracker;
	return 0;
}

// Closes the file and frees any allocated memory.
// Returns 0 on success.
static int myClose(struct inode* inode, struct file* fs)
{
	struct caesarTracker* tracker;
	tracker = (struct caesarTracker*) fs->private_data;

	vfree(tracker);
	return 0;
}

// Encrypts the kernel buffer with a Caesar Cipher.
// Returns 0 on success.
static int encrypt(int key)
{
	int index;
	int length = strlen(kernel_buffer);
	int overlap = key % 26;
	if (overlap != 0) {
		for (index = 0; index < length; index++) {
			char ch = kernel_buffer[index];

			if (ch >= 'A' && ch <= 'Z') {  // uppercase letters
				ch = 'A' + ((ch - 'A' + key) % 26);
			} else if (ch >= 'a' && ch <= 'z') {  // lowercase letters
				ch = 'a' + ((ch - 'a' + key) % 26);
			}

			kernel_buffer[index] = ch;
		}
	}

	printk(KERN_INFO "Encrypted Text:\n%s\n", kernel_buffer);
	return 0;
}

// Decrypts the kernel buffer with a Caesar Cipher.
// Returns 0 on success.
static int decrypt(int key)
{
	int index;
	int length = strlen(kernel_buffer);
	int overlap = key % 26;
	if (overlap != 0) {
		for (index = 0; index < length; index++) {
			char ch = kernel_buffer[index];

			if (ch >= 'A' && ch <= 'Z') {  // uppercase letters
				ch = 'A' + ((ch - 'A' - key + (26 * overlap)) % 26);
			} else if (ch >= 'a' && ch <= 'z') {  // lowercase letters
				ch = 'a' + ((ch - 'a' - key + (26 * overlap)) % 26);
			}

			kernel_buffer[index] = ch;
		}
	}

	printk(KERN_INFO "Decrypted Text:\n%s\n", kernel_buffer);
	return 0;
}

// Handles the logic for the IO controller
// Does the appropriate function call based on the passed in command and data.
// Returns 0 on success.
static long myIoCtl(struct file* fs, unsigned int command, unsigned long data)
{
	int result;
	struct caesarTracker* tracker;

	tracker = (struct caesarTracker*) fs->private_data;

	switch (command) {
		case ENCRYPT:
			result = encrypt(tracker->key);
			break;
		case DECRYPT:
			result = decrypt(tracker->key);
			break;
		case SETKEY:
			tracker->key = (int) data;
			result = 0;
			break;
		default:
			printk(KERN_ERR "Failed IOCTL.\n");
			result = -1;
	}

	return result;
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

	kernel_buffer = vmalloc(512);
	if (kernel_buffer == NULL) {
		printk(KERN_ERR "Failed to vmalloc kernel_buffer.\n");
		return -1;
	}

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
