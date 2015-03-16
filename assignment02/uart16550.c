#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include "uart16550.h"
#include "uart16550_hw.h"

MODULE_DESCRIPTION("Uart16550 driver");
MODULE_LICENSE("GPL");

#ifdef __DEBUG
 #define dprintk(fmt, ...)     printk(KERN_DEBUG "%s:%d " fmt, \
                                __FILE__, __LINE__, ##__VA_ARGS__)
#else
 #define dprintk(fmt, ...)     do { } while (0)
#endif

static struct class *uart16550_class = NULL;

static struct cdev *uart_cdev_com1;
static struct cdev *uart_cdev_com2;

/*
 * TODO: Populate major number from module options (when it is given).
 */

/* Identifies the driver associated with the device */
static int major = 42;
module_param(major, int, 0);

static int behavior = 0x3;
module_param(behavior, int, 0);

static int uart16550_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param) {
	/* TODO */
	dprintk("[uart debug] uart16550_ioctl()\n");
	return 0;
}

/* Called when a process closes the device file */
static int uart16550_release(struct inode *inode, struct file *file) {
	/* TODO */
	dprintk("[uart debug] uart16550_release()\n");
	return 0;
}

static int uart16550_open(struct inode * inode, struct file * file) {
	/* TODO Not sure, just trying */
	dprintk("[uart debug] uart16550_open()\n");
	struct serial_dev *dev = container_of(inode->i_cdev, struct serial_dev, cdev);
	file->private_data = dev;
	return 0;
}

static int uart16550_read(struct file *filp,
   char *buffer,    /* The buffer to fill with data */
   size_t length,   /* The length of the buffer     */
   loff_t *offset)  /* Our offset in the file       */ {
   	/* TODO */
   	dprintk("[uart debug] uart16550_read()\n");
   	return 0;
   }

/* To write data from userspace to the device */
static int uart16550_write(struct file *file, const char *user_buffer,
        size_t size, loff_t *offset)
{
	dprintk("[uart debug] uart16550_write()\n");
        int bytes_copied;
        uint32_t device_port;
        /*
         * TODO: Write the code that takes the data provided by the
         *      user from userspace and stores it in the kernel
         *      device outgoing buffer.
         * TODO: Populate bytes_copied with the number of bytes
         *      that fit in the outgoing buffer.
         */

        uart16550_hw_force_interrupt_reemit(device_port);

        return bytes_copied;
}

irqreturn_t interrupt_handler(int irq_no, void *data)
{
	dprintk("[uart debug] interrupt_handler()\n");
        int device_status;
        uint32_t device_port;
        /*
         * TODO: Write the code that handles a hardware interrupt.
         * TODO: Populate device_port with the port of the correct device.
         */

        device_status = uart16550_hw_get_device_status(device_port);

        while (uart16550_hw_device_can_send(device_status)) {
                uint8_t byte_value;
                /*
                 * TODO: Populate byte_value with the next value
                 *      from the kernel device outgoing buffer.
                 */
                uart16550_hw_write_to_device(device_port, byte_value);
                device_status = uart16550_hw_get_device_status(device_port);
        }

        while (uart16550_hw_device_has_data(device_status)) {
                uint8_t byte_value;
                byte_value = uart16550_hw_read_from_device(device_port);
                /*
                 * TODO: Store the read byte_value in the kernel device
                 *      incoming buffer.
                 */
                device_status = uart16550_hw_get_device_status(device_port);
        }

        return IRQ_HANDLED;
}

/* See: http://www.tldp.org/LDP/lkmpg/2.4/html/c577.htm */
static const struct file_operations uart_fops =
{
	.owner	= THIS_MODULE,
	.open	= uart16550_open,
	.release= uart16550_release,
	.write	= uart16550_write,
	.read	= uart16550_read,
	.unlocked_ioctl	= uart16550_ioctl
};

/* Makes sure the given parameters are legal */
static int bad_parameters(int major, int behavior) {
	return (major < 0) || (major > 1000000) || (behavior < 0x1) || (behavior > 0x3);
}

static void uart16550_cleanup(void)
{
	
	dprintk("[uart debug] iuart16550_cleanup()\n");
	
	if (bad_parameters(major, behavior)) {
		goto cleanup_done;
	}
	
        int have_com1 = 0;
        int have_com2 = 0;
        
        /*
         * TODO: Write driver cleanup code here.
         * TODO: have_com1 & have_com2 need to be set according to the
         *      module parameters.
         */
         
        /* UART16550_COM1_SELECTED is 0x1 */
	have_com1 = behavior & UART16550_COM1_SELECTED;
	
	/* UART16550_COM2_SELECTED is 0x2 */
	have_com2 = behavior & UART16550_COM2_SELECTED;
         
        if (have_com1) {
                /* Reset the hardware device for COM1 */
                uart16550_hw_cleanup_device(COM1_BASEPORT);
		/* Unregister character device */
		dev_t dev_no = MKDEV(major, 0);
		unregister_chrdev_region(dev_no, 1);
                /* Remove the sysfs info for /dev/com1 */
                device_destroy(uart16550_class, dev_no);
                /* Deallocate struct */
                cdev_del(uart_cdev_com1);
        }
        if (have_com2) {
                /* Reset the hardware device for COM2 */
                uart16550_hw_cleanup_device(COM2_BASEPORT);
		/* Unregister character device */
		dev_t dev_no = MKDEV(major, 1);
		unregister_chrdev_region(dev_no, 1);
                /* Remove the sysfs info for /dev/com2 */
                device_destroy(uart16550_class, dev_no);
                /* Deallocate struct */
                cdev_del(uart_cdev_com2);
        }

        /*
         * Cleanup the sysfs device class.
         */
        class_unregister(uart16550_class);
        class_destroy(uart16550_class);
        
        cleanup_done: /* Nothing */ ;
}

static int uart16550_init(void)
{
	dprintk("[uart debug] In uart16550_init() called with major=%d behavior=%#03x\n", major, behavior);
	
        int have_com1 = 0;
        int have_com2 = 0;
        /*
         * TODO: Write driver initialization code here.
         * TODO: have_com1 & have_com2 need to be set according to the
         *      module parameters.
         * TODO: Check return values of functions used. Fail gracefully.
         */

	if (bad_parameters(major, behavior)) {
		/* Invalid parameters */
		dprintk("[uart debug] Invalid parameters\n");
		goto fail_init;
	}
	
	/* UART16550_COM1_SELECTED is 0x1 */
	have_com1 = behavior & UART16550_COM1_SELECTED;
	
	/* UART16550_COM2_SELECTED is 0x2 */
	have_com2 = behavior & UART16550_COM2_SELECTED;
	

        /*
         * Setup a sysfs class & device to make /dev/com1 & /dev/com2 appear.
         */
        uart16550_class = class_create(THIS_MODULE, "uart16550");

        if (have_com1) {
        	dprintk("[uart debug] have_com1 = true\n");
                /* Setup the hardware device for COM1 */
                if (uart16550_hw_setup_device(COM1_BASEPORT, THIS_MODULE->name)) {
                	dprintk("[uart debug] An error occured on uart16550_hw_setup_device (com1)\n");
                	goto fail_init;
                }
                
                /* Register character device */
		dev_t dev_no = MKDEV(major, 0);
		if (register_chrdev_region(dev_no, 1,"com1")) {
			dprintk("[uart debug] An error occured on register_chrdev_region (com1)\n");
			goto fail_init;
		}
		
		/* Create the sysfs info for /dev/com1 */
                device_create(uart16550_class, NULL, dev_no, NULL, "com1");
                
                /* cdev thing */
                uart_cdev_com1 = cdev_alloc();
		if (NULL == uart_cdev_com1) {
			/* Allocation failed */
			dprintk("[uart debug] cdev_alloc() failed  (com1)\n");
			goto fail_init;
		}
		uart_cdev_com1->ops = &uart_fops;
		
		/* Note: after calling cdev_add, the device is "live" and
		*  its operations can be called by the kernel */
		if (cdev_add(uart_cdev_com1, dev_no, 1)) {
			printk ("[uart debug] cdev_add() failed (com1)\n");
			goto fail_init;
		}
        }
        if (have_com2) {
        	dprintk("[uart debug] have_com2 = true\n");
                /* Setup the hardware device for COM2 */
                
                if (uart16550_hw_setup_device(COM2_BASEPORT, THIS_MODULE->name)) {
                	dprintk("[uart debug] An error occured on uart16550_hw_setup_device (com2)\n");
                	goto fail_init;
                }
                
		/* Register character device */
		dev_t dev_no = MKDEV(major, 1);
		
		if (register_chrdev_region(dev_no, 1, "com2")) {
			dprintk("[uart debug] An error occured on register_chrdev_region (com2)\n");
			goto fail_init;
		}
		
                /* Create the sysfs info for /dev/com2 */
                device_create(uart16550_class, NULL, dev_no, NULL, "com2");
                
                /* cdev thing */
                uart_cdev_com2 = cdev_alloc();
		if (NULL == uart_cdev_com2) {
			/* Allocation failed */
			dprintk("[uart debug] cdev_alloc() failed  (com2)\n");
			goto fail_init;
		}
		uart_cdev_com2->ops = &uart_fops;
		
		if (cdev_add(uart_cdev_com2, dev_no, 1)) {
			printk ("[uart debug] cdev_add() failed (com2)\n");
			goto fail_init;
		}
        }
        return 0;
        
        fail_init:
        	uart16550_cleanup();
        	return -1;
}

module_init(uart16550_init)
module_exit(uart16550_cleanup)
