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

  return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                  loff_t *f_pos) {
  struct aesd_dev *dev = filp->private_data;
  ssize_t retval = 0;
  size_t cur_off = *f_pos;
  size_t entry_offset_byte_rtn;
  struct aesd_buffer_entry *entry = NULL;

  PDEBUG("read %zu bytes with offset %lld", count, *f_pos);

  PDEBUG("****** PRINTING BUFFER ******");
  print_buffer(dev->buffer);

  if (mutex_lock_interruptible(&dev->lock))
    return -ERESTARTSYS;

  PDEBUG("Searching for entry with cur_off = %lu", cur_off);
  entry = aesd_circular_buffer_find_entry_offset_for_fpos(
      dev->buffer, cur_off, &entry_offset_byte_rtn);

  if (entry == NULL) {
    PDEBUG("Null entry! Exiting");
    goto out;
  }

  PDEBUG("Found entry %s with entry_offset = %lu", entry->buffptr,
         entry_offset_byte_rtn);

  PDEBUG(" \
  f_pos: %llu \n \
  count: %lu \n \
  entry->size: %lu \
  ",
         *f_pos, count, entry->size);

  if (count > entry->size)
    count = entry->size;

  PDEBUG(" \
  CHANGED \n \
  f_pos: %llu \n \
  count: %lu \n \
  entry->size: %lu \
  ",
         *f_pos, count, entry->size);

  if (copy_to_user(buf, entry->buffptr, count)) {
    retval = -EFAULT;
    goto out;
  }

  *f_pos += count;
  retval = count;

out:
  mutex_unlock(&dev->lock);
  return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                   loff_t *f_pos) {
  struct aesd_dev *dev = filp->private_data;
  size_t buf_size = count + dev->last_entry_size;
  ssize_t retval = -ENOMEM;
  struct aesd_buffer_entry *new_entry = NULL;

  PDEBUG("write %zu bytes with offset %lld", count, *f_pos);

  if (mutex_lock_interruptible(&dev->lock))
    return -ERESTARTSYS;

  if (dev->tmp_entry.size == 0) {
    PDEBUG("Entry not existing");
  } else {
    PDEBUG("Entry should be existing...");
  }

  PDEBUG("Allocating mem for buffptr. Last size is: %lu ; new size is: %lu",
         dev->last_entry_size, buf_size);

  dev->tmp_entry.buffptr =
      krealloc(dev->tmp_entry.buffptr, buf_size, GFP_KERNEL);
  if (!dev->tmp_entry.buffptr)
    goto out;
  dev->tmp_entry.size = buf_size;

  PDEBUG("Copying from user");

  if (copy_from_user((void *)(dev->tmp_entry.buffptr + dev->last_entry_size),
                     buf, count)) {
    retval = -EFAULT;
    goto out;
  }

  PDEBUG("Last Operations");

  *f_pos += count;
  retval = count;

  /* Check if terminated with \n char */
  if (buf[count - 1] == '\n') {
    dev->tmp_entry.buffptr =
        krealloc(dev->tmp_entry.buffptr, (buf_size + 1), GFP_KERNEL);

    memset((void *)(dev->tmp_entry.buffptr + buf_size), '\0', 1);

    PDEBUG("user buf ended with terminating char. Entry is: %s",
           dev->tmp_entry.buffptr);

    /* Allocate memory for new entry */
    new_entry = kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
    if (!new_entry)
      goto out;
    new_entry->buffptr = kmalloc((buf_size + 1), GFP_KERNEL);

    /* Set new entry value */
    new_entry->buffptr = memcpy((void *)new_entry->buffptr,
                                dev->tmp_entry.buffptr, (buf_size + 1));
    new_entry->size = (buf_size + 1);

    /* Reset tmp entry vars for new buf */
    kfree(dev->tmp_entry.buffptr);
    memset(&(aesd_device.tmp_entry), 0, sizeof(struct aesd_buffer_entry));
    dev->last_entry_size = 0;

    /* If buffer is full ,free memory of last written element */
    if (dev->buffer->full) {
      PDEBUG("Buffer is full. Freeing memory");
      kfree(dev->buffer->entry[dev->buffer->in_offs].buffptr);
      dev->buffer->entry[dev->buffer->in_offs].size = 0;
    }

    aesd_circular_buffer_add_entry(dev->buffer, new_entry);
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
  uint8_t index;
  struct aesd_buffer_entry *entry;

  result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
  aesd_major = MAJOR(dev);
  if (result < 0) {
    printk(KERN_WARNING "Can't get major %d\n", aesd_major);
    return result;
  }
  memset(&aesd_device, 0, sizeof(struct aesd_dev));

  result = aesd_setup_cdev(&aesd_device);

  if (result) {
    unregister_chrdev_region(dev, 1);
  }

  PDEBUG("Allocating initial memory for buffer");

  /* Allocate memory for buffer */
  aesd_device.buffer = kmalloc(sizeof(struct aesd_circular_buffer), GFP_KERNEL);
  if (!aesd_device.buffer)
    return -ERESTARTSYS;

  cdev_del(&aesd_device.cdev);

  /* Init buffer entries */
  AESD_CIRCULAR_BUFFER_FOREACH(entry, aesd_device.buffer, index) {
    entry->size = 0;
    memset(entry, 0, sizeof(struct aesd_buffer_entry));
  }

  mutex_init(&aesd_device.lock);

  /* Initialize device settings */
  memset(&(aesd_device.tmp_entry), 0, sizeof(struct aesd_buffer_entry));
  aesd_device.last_entry_size = 0;

  return result;
}

void aesd_cleanup_module(void) {
  dev_t devno = MKDEV(aesd_major, aesd_minor);
  uint8_t index;
  struct aesd_buffer_entry *entry;

  cdev_del(&aesd_device.cdev);

  /* Free buffer entries */
  AESD_CIRCULAR_BUFFER_FOREACH(entry, aesd_device.buffer, index) {
    if (entry->size > 0) {
      kfree(entry->buffptr);
      kfree(entry);
    }
  }

  /* Free buffer memory */
  kfree(aesd_device.buffer);

  unregister_chrdev_region(devno, 1);
}

module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
