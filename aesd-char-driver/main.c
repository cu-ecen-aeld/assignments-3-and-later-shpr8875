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
{
    PDEBUG("open");
    
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);   
    filep->private_data = dev;
    
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
    size_t byte_read;
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

   
    size_t bytes_to_read = bytes_read; 
    if (bytes_to_read > count) 
    {
        bytes_to_read = count; 
    }


    // Transfer data byte by byte to the user space buffer using put_user
    for (i = 0; i < bytes_to_read; i++) 
    {
        if (put_user(data[i], buf + i)) 
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
    char *command_buf = NULL; 
    size_t new_size;

    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);

    
    if (count > AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) 
    {
        return -EINVAL; // Return error if too large
    }

    mutex_lock(&dev->lock); 

   // Making use of partialm cmd
    if (dev->partial_command) 
    {
        new_size = dev->partial_size + count; 
        command_buf = krealloc(dev->partial_command, new_size, GFP_KERNEL);
        if (!command_buf) {
            retval = -ENOMEM; // Memory allocation failed
            mutex_unlock(&dev->lock);
            return retval;
        }

        // copy new data into buffer
        if (copy_from_user(command_buf + dev->partial_size, buf, count)) 
        {
            kfree(command_buf);
            mutex_unlock(&dev->lock);
            return -EFAULT; 
        }
        dev->partial_command = command_buf; 
        dev->partial_size = new_size; 
    } 
    else 
    {
        // Allocate a new buffer for the command
        command_buf = kmalloc(count, GFP_KERNEL);
        if (!command_buf) 
        {
            retval = -ENOMEM;
            mutex_unlock(&dev->lock);
            return retval;
        }

        // Copy data from user space to kernel space
        if (copy_from_user(command_buf, buf, count)) 
        {
            kfree(command_buf); 
            mutex_unlock(&dev->lock);
            return -EFAULT; 
        }
        dev->partial_command = command_buf; 
        dev->partial_size = count; 
    }

     // Check if the command contains a newline character
    if (memchr(dev->partial_command, newline, dev->partial_size)) 
    {
       // Add the complete command to the circular buffer
        retval = aesd_circular_buffer_add_entry(&dev->buffer, dev->partial_command, dev->partial_size);
        if (retval < 0) 
        {
            kfree(dev->partial_command); 
            dev->partial_command = NULL; 
            dev->partial_size = 0; 
        } 
        else 
        {
            // If the buffer was full, free the oldest entry
            if (dev->buffer.full) 
            {
                kfree(dev->buffer.entry[dev->buffer.out_offs].buffptr);
            }
            // Reset the partial command state for future writes
            dev->partial_command = NULL;
            dev->partial_size = 0;
            retval = count; 
        }
    } 
    else 
    {
        retval = count; 
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
    kfree(aesd_device.partial_command);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
