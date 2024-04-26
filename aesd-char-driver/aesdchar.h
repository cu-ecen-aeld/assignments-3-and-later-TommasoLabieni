/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */

#ifndef AESD_CHAR_DRIVER_AESDCHAR_H_
#define AESD_CHAR_DRIVER_AESDCHAR_H_

#include "aesd-circular-buffer.h"
#include <linux/cdev.h>
#include <linux/mutex.h>

#define AESD_DEBUG 1 // Remove comment on this line to enable debug

#undef PDEBUG /* undef it, just in case */
#ifdef AESD_DEBUG
#ifdef __KERNEL__
/* This one if debugging is on, and kernel space */
#define PDEBUG(fmt, args...) printk(KERN_DEBUG "aesdchar: " fmt, ##args)
#else
/* This one for user space */
#define PDEBUG(fmt, args...) fprintf(stderr, fmt, ##args)
#endif
#else
#define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

struct aesd_dev {
  struct aesd_buffer_entry tmp_entry;
  ssize_t last_entry_size;
  size_t size;
  struct aesd_circular_buffer *buffer; /* circular buffer */
  struct mutex lock;                   /* mutual exclusion semaphore     */
  struct cdev cdev;                    /* Char device structure      */
};

#endif /* AESD_CHAR_DRIVER_AESDCHAR_H_ */
