/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include "aesdchar.h"
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/types.h>

int aesd_major = 0; // use dynamic major
int aesd_minor = 0;

MODULE_AUTHOR("Tommaso Labieni");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp) {
  struct aesd_dev *dev; /* device information */
  PDEBUG("open start");

  dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
  filp->private_data = dev; /* for other methods */

  return 0;
}

int aesd_release(struct inode *inode, struct file *filp) {
  PDEBUG("release");
  /**
   * TODO: handle release
   */
  return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                  loff_t *f_pos) {
  ssize_t retval = 0;
  PDEBUG("read %zu bytes with offset %lld", count, *f_pos);
  /**
   * TODO: handle read
   */
  return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                   loff_t *f_pos) {
  ssize_t retval = -ENOMEM;
  struct aesd_dev *dev = filp->private_data;
  size_t buf_size = count + dev->last_entry_size;

  PDEBUG("write %zu bytes with offset %lld", count, *f_pos);

  if (mutex_lock_interruptible(&dev->lock))
    return -ERESTARTSYS;

  if (!dev->entry) {
    PDEBUG("Allocate mem for new entry");
    dev->entry = kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
    if (!dev->entry)
      return -ERESTARTSYS;
  } else {
    PDEBUG("entry should be existing...");
  }

  PDEBUG("Allocating mem for buffptr. Last size is: %lu ; new size is: %lu",
         dev->last_entry_size, buf_size);

  dev->entry->buffptr = krealloc(dev->entry->buffptr, buf_size, GFP_KERNEL);
  if (!dev->entry->buffptr)
    goto out;
  dev->entry->size = buf_size;

  PDEBUG("Copying from user");

  if (copy_from_user((void *)(dev->entry->buffptr + dev->last_entry_size), buf,
                     count)) {
    retval = -EFAULT;
    goto out;
  }

  PDEBUG("Last Operations");

  *f_pos += count;
  retval = buf_size;

  /* Check if terminated with \n char */
  if (buf[count - 1] == '\n') {
    PDEBUG("user buf ended with terminating char.");
    /* Reset last entry size for new buf */
    dev->last_entry_size = 0;
    aesd_circular_buffer_add_entry(dev->buffer, dev->entry);
  } else {
    PDEBUG("user buf DID NOT ended with terminating char.");
    /* Update last entry size */
    dev->last_entry_size += count;
  }

out:
  mutex_unlock(&dev->lock);
  return retval;
}
struct file_operations aesd_fops = {
    .owner = THIS_MODULE,
    .read = aesd_read,
    .write = aesd_write,
    .open = aesd_open,
    .release = aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev) {
  int err, devno = MKDEV(aesd_major, aesd_minor);

  cdev_init(&dev->cdev, &aesd_fops);
  dev->cdev.owner = THIS_MODULE;
  dev->cdev.ops = &aesd_fops;
  err = cdev_add(&dev->cdev, devno, 1);
  if (err) {
    printk(KERN_ERR "Error %d adding aesd cdev", err);
  }
  return err;
}

int aesd_init_module(void) {
  dev_t dev = 0;
  int result;
  result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
  aesd_major = MAJOR(dev);
  if (result < 0) {
    printk(KERN_WARNING "Can't get major %d\n", aesd_major);
    return result;
  }
  memset(&aesd_device, 0, sizeof(struct aesd_dev));

  /**
   * TODO: initialize the AESD specific portion of the device
   */

  result = aesd_setup_cdev(&aesd_device);

  if (result) {
    unregister_chrdev_region(dev, 1);
  }

  PDEBUG("Allocating initial memory for buffer");
  /* Allocate memory for buffer */
  aesd_device.buffer = kmalloc(sizeof(struct aesd_circular_buffer), GFP_KERNEL);
  if (!aesd_device.buffer)
    return -ERESTARTSYS;

  mutex_init(&aesd_device.lock);

  /* Initialize device settings */
  aesd_device.entry = NULL;
  aesd_device.last_entry_size = 0;

  return result;
}

void aesd_cleanup_module(void) {
  dev_t devno = MKDEV(aesd_major, aesd_minor);

  cdev_del(&aesd_device.cdev);

  /**
   * TODO: cleanup AESD specific poritions here as necessary
   */

  unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
