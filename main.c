/****
 * Basic Guide to reading this code,
 * 
 * 
 * 
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrew_Infinite");
MODULE_DESCRIPTION("Bit_Bang Module");
#define DRIVER_NAME "BitBangDevice"
#define DRIVER_CLASS "BitBangClass"
#define TX_PIN 17
#define RX_PIN 27

/*Prototype*/
static int device_file_open(struct inode *, struct file *);
static int device_file_close(struct inode *, struct file *);
static ssize_t device_file_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_file_write(struct file *, const char *, size_t, loff_t *);

/* Buffer for data */
#define buffer_cap 2000
static int buffer_end_ptr;
static char buffer_send[buffer_cap];
static char buffer_receive[buffer_cap];

/* Variables for device and device class */
static dev_t my_device_num;
static struct class *my_class;
static struct cdev my_device;

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_file_open,
    .release = device_file_close,
	.read = device_file_read,
	.write = device_file_write
};

/**
 * @brief Filesys open, load file to memory
*/
static int device_file_open(struct inode *device_file, struct file *instance)
{
    printk("Bit_Bang Module is open\n");
    return 0;
}

/**
 * @brief Filesys close, remove file from memory
*/
static int device_file_close(struct inode *device_file, struct file *instance)
{
    printk("Bit_Bang Module is close\n");
    return 0;
}

/**
 * @brief Filesys read, Read data out of the buffer
 * 
 * Data stop when delta is 0.
 */
static ssize_t device_file_read(struct file *File, char *user_buffer, size_t count, loff_t *offset) 
{
	int to_copy, not_copied, delta;

	/* Get amount of data to copy */
	to_copy = min(count, buffer_end_ptr);

	/* Copy data to user */
	not_copied = copy_to_user(user_buffer, buffer_receive, to_copy);

	/* Calculate data */
	delta = to_copy - not_copied;

	return delta;
}

/**
 * @brief Filesys write, write data to buffer
 */
static ssize_t device_file_write(struct file *File, const char *user_buffer, size_t count, loff_t *offset) 
{
	int to_copy, not_copied, delta;

	/* Get amount of data to copy */
	to_copy = min(count, sizeof(buffer_send));

	/* Copy data to user */
	not_copied = copy_from_user(buffer_send, user_buffer, to_copy);

	/* Calculate data */
	delta = to_copy - not_copied;
	if(strncmp(buffer_send,"clear\n",6) == 0)
	{
		char *clear = "\033[2J\033[H";
		while(*clear)
		{
			int j = -1;
			gpio_set_value(TX_PIN, 0);
			udelay(1000000 / 9600);
			while(++j < 8)
			{
				gpio_set_value(TX_PIN, (*clear >> j) & 0x01);
				udelay(1000000 / 9600);  // Baud rate: 9600
			}
			gpio_set_value(TX_PIN, 1);
			udelay(1000000 / 9600);
			clear++;
		}
	}
	else
	{
		int i = -1;
		while(++i < delta)
		{
			int j = -1;
			gpio_set_value(TX_PIN, 0);
			udelay(1000000 / 9600);
			while(++j < 8)
			{
				gpio_set_value(TX_PIN, (*(buffer_send + i) >> j) & 0x01);
				udelay(1000000 / 9600);  // Baud rate: 9600
			}
			gpio_set_value(TX_PIN, 1);
			udelay(1000000 / 9600);
		}
	}
	return delta;
}

/**
 * @brief Kernel Module Init Function, this is called when module is loaded to kernel
*/
static int __init ModuleInit(void)
{
	int retval;

	/* Dynamically allocate a device number */
	if( alloc_chrdev_region(&my_device_num, 0, 1, DRIVER_NAME) < 0) {
		printk("ERROR: Bit_Bang Module: Device Number could not be allocated!\n");
		return -1;
	}
	printk("Bit_Bang Module: number: Major: %d, Minor: %d!\n", MAJOR(my_device_num), MINOR(my_device_num));

	/* Create device class */
	if((my_class = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL) {
		printk("ERROR: Bit_Bang Module: Device class can not be created!\n");
		goto ClassError;
	}

	/* create device file */
	if(device_create(my_class, NULL, my_device_num, NULL, DRIVER_NAME) == NULL) {
		printk("ERROR: Bit_Bang Module: Can not create device file!\n");
		goto FileError;
	}

	/* Initialize device file */
	cdev_init(&my_device, &fops);

	/* Regisering device to kernel */
	if(cdev_add(&my_device, my_device_num, 1) == -1) {
		printk(KERN_ERR "ERROR: Bit_Bang Module: Registering of device to kernel failed!\n");
		goto AddError;
	}

    if (gpio_request(TX_PIN, "tx_pin") || gpio_request(RX_PIN, "rx_pin")) {
        printk(KERN_ERR "ERROR: Bit_Bang Module: Failed to request GPIO pins\n");
        goto GpioReqError;
    }

    if(gpio_direction_output(TX_PIN, 1) || gpio_direction_input(RX_PIN))
    {
        printk(KERN_ERR "ERROR: Bit_Bang Module: Failed to config pins\n");
        goto GpioSetError;
    }
	return 0;

GpioSetError:
    gpio_free(TX_PIN);
    gpio_free(RX_PIN);
GpioReqError:
    /*do nothing*/;
AddError:
    device_destroy(my_class, my_device_num);
FileError:
    class_destroy(my_class);
ClassError:
    unregister_chrdev_region(my_device_num, 1);
    return -1;
}

/**
 * @brief Kernel Module Destruct Function, this is called when module is unloaded from kernel
*/
static void __exit ModuleExit(void)
{
    gpio_direction_output(TX_PIN, 0);
    gpio_free(TX_PIN);
    gpio_free(RX_PIN);

    cdev_del(&my_device);
	device_destroy(my_class, my_device_num);
	class_destroy(my_class);
	unregister_chrdev_region(my_device_num, 1);
    printk("Bit_Bang Module unloaded\n");
}

module_init(ModuleInit);
module_exit(ModuleExit);