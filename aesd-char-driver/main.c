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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include <linux/slab.h> 
#include <linux/uio.h> 
#include "aesd-circular-buffer.h"


int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Shweta Prasad"); 
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{   struct aesd_dev *dev;
    PDEBUG("open");
    
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);   
    filp->private_data = dev;
    
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *data;
    size_t bytes_read;
    size_t bytes_to_read = bytes_read; 
    size_t i;

    
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    mutex_lock(&dev->lock);

    // Retrieve the pointer to data from the circular buffer starting at *f_pos
    data = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &bytes_read);
    if (!data) 
    {
        mutex_unlock(&dev->lock);
        return 0; // No data available
    }

   
    if (bytes_to_read > count) 
    {
        bytes_to_read = count; 
    }


    // Transfer data byte by byte to the user space buffer using put_user
    for (i = 0; i < bytes_to_read; i++) 
    {
        if (put_user(data->buffptr[i], buf + i)) 
        {
            retval = -EFAULT;
            mutex_unlock(&dev->lock);
            return retval;
        }
    }

    *f_pos += bytes_to_read; // Update the file position
    retval = bytes_to_read;   // Return the number of bytes read

    mutex_unlock(&dev->lock);
    return retval;

}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev = filp->private_data;
    const char newline = '\n';  
    size_t new_size = dev->entry.size;
    ssize_t bytes_not_write;
    
    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);

    
    if (count > AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) 
    {
        return -EINVAL; // Return error if too large
    }

    mutex_lock(&dev->lock); 
  
    // Allocate memory for the new data or resize if necessary
    if (new_size == 0) 
    {
        dev->entry.buffptr = kzalloc(count, GFP_KERNEL);
    } else 
    {
        dev->entry.buffptr = krealloc(dev->entry.buffptr, new_size + count, GFP_KERNEL);
    }

    if (!dev->entry.buffptr) 
    {
        mutex_unlock(&dev->lock);
        return retval;
    }
   
    bytes_not_write = copy_from_user((void*)&dev->entry.buffptr[new_size], buf, count);
    retval = count - bytes_not_write;
    dev->entry.size += retval;

    *f_pos += retval;  // Update file position

    if (strchr((char*)dev->entry.buffptr, newline)) 
    {
        aesd_circular_buffer_add_entry(&dev->buffer, &dev->entry);
        dev->entry.buffptr = NULL; 
        dev->entry.size = 0;
    }

    mutex_unlock(&dev->lock); // Unlock the mutex
    return retval; 
}

struct file_operations aesd_fops = 
{
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) 
    {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) 
    {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) 
    {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    cdev_del(&aesd_device.cdev);
    int index;
    struct aesd_buffer_entry *entry;

   AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, index) 
    {
        kfree(entry->buffptr);
    }
   // kfree(aesd_device.entry.buffptr);
    mutex_destroy(&aesd_device.lock);

    // Free partial command memory if allocated
   // kfree(aesd_device.partial_command);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);