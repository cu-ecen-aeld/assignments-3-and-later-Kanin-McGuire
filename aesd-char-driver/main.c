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
#include "linux/slab.h"
#include "linux/string.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Kanin McGuire"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    filp->private_data = container_of( inode->i_cdev, struct aesd_dev, cdev );
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    filp->private_data = NULL;
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *aesdDev = filp->private_data;
    size_t entry_offset_byte;
    size_t available_bytes;
    size_t bytes_to_copy;
    struct aesd_buffer_entry *entry;
    PDEBUG( "read %zu bytes with offset %lld",count,*f_pos );

    // Lock the device to prevent concurrent access
    if ( mutex_lock_interruptible( &aesdDev->mutexLock ) )
    {
        // Unable to lock mutex
        retval = -ERESTARTSYS;
    }

    // Find the entry offset for the read position
    entry = aesd_circular_buffer_find_entry_offset_for_fpos( &aesdDev->buff, *f_pos, &entry_offset_byte );


    if ( entry != NULL )
    {
        // Calculate the number of bytes available for reading
        available_bytes = entry->size - entry_offset_byte;
        if ( available_bytes > 0 )
        {
            // Copy data to user space
            bytes_to_copy = ( available_bytes > count ) ? count : available_bytes;

            if ( !copy_to_user( buf, entry->buffptr + entry_offset_byte, bytes_to_copy ))
            {
                // Update file position
                *f_pos += bytes_to_copy;
                retval = bytes_to_copy;
            }
            else
            {
                // Error copying data to user space
                retval = -EFAULT;
                goto errorHandler;
            }
        }
        else
        {
            // No more data available
            retval = 0;
            goto errorHandler;
        }
    }
    else
    {
        // No data available here
        retval = 0;
        goto errorHandler;
    }

errorHandler:
    // Unlock the device
    mutex_unlock( &aesdDev->mutexLock );

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *aesdDev = NULL;
    const char* entryPtr = NULL;
    // PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    PDEBUG("write %zu bytes with offset %lld: %.*s", count, *f_pos, (int)count, buf);

    aesdDev = filp->private_data;

    // Allocate memory for the write command
    aesdDev->buffEntry.buffptr = aesdDev->buffEntry.size ? ( char * )krealloc( aesdDev->buffEntry.buffptr, count + aesdDev->buffEntry.size, GFP_KERNEL ) : ( char * )kmalloc( count, GFP_KERNEL );

    if ( aesdDev->buffEntry.buffptr == NULL )
    {
        // Error handling
        return -ENOMEM;
    }

    if ( mutex_lock_interruptible( &aesd_device.mutexLock ) )
    {
        // Unable to lock mutex
        retval = -ERESTARTSYS;
    }

    // Copy write command from user space
    if ( copy_from_user( (void *)( aesdDev->buffEntry.buffptr + aesdDev->buffEntry.size ), buf, count ) == 0 )
    {
        aesdDev->buffEntry.size += count;
        if ( memchr( aesdDev->buffEntry.buffptr, '\n', aesdDev->buffEntry.size ) != NULL )
        {
            // Add the entry to the circular buffer
            entryPtr = aesd_circular_buffer_add_entry( &aesdDev->buff, &aesdDev->buffEntry );

            if ( entryPtr != NULL )
            {
                kfree( entryPtr );
                entryPtr = NULL;
            }

            aesdDev->buffEntry.buffptr = NULL;
            aesdDev->buffEntry.size = 0;
        }
    }
    else
    {
        kfree( aesdDev->buffEntry.buffptr );
        // Error copying from user space
        retval = -EFAULT;
        goto errorHandler;
    }

    // Unlock the device
    mutex_unlock( &aesd_device.mutexLock );

    retval = count; // Return the number of bytes written

    return retval;

errorHandler:
    // Free memory allocated for buffer entry
    kfree( aesdDev->buffEntry.buffptr );

    // Unlock the device
    mutex_unlock( &aesd_device.mutexLock );

    return retval;
}

struct file_operations aesd_fops = {
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
    if (err) {
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
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    mutex_init( &aesd_device.mutexLock );

    aesd_circular_buffer_init( &( aesd_device.buff ) );

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    struct aesd_buffer_entry *entryPtr;
    int index = 0;
    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    mutex_destroy( &aesd_device.mutexLock );

    AESD_CIRCULAR_BUFFER_FOREACH( entryPtr, &aesd_device.buff, index )
    {
        if ( entryPtr->buffptr )
        {
            kfree( entryPtr->buffptr );
            entryPtr->buffptr = NULL;
            entryPtr->size = 0;
        }
    }

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
