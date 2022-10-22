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
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/gfp.h>

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Kevin Tom");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;


int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");

    struct aesd_dev *dev;
    dev = container_of(inode->i_cdev,struct aesd_dev, cdev);
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
    size_t *entry_offset_byte_rtn=NULL;
    size_t byte_postion = 0;
    struct aesd_buffer_entry *aesd_entry_ret;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    mutex_lock(&aesd_device.aesd_char_mut);
    aesd_entry_ret = aesd_circular_buffer_find_entry_offset_for_fpos(&aesd_device.circular_buffer, (*f_pos), entry_offset_byte_rtn);
    if(aesd_buffer_entry == NULL)
    {
        mutex_unlock(&aesd_device.aesd_char_mut); 
        PDEBUG("Nothing was read");
        return 0;  
    }
    else
    {
        //byte_postion = aesd_entry_ret->size - entry_offset_byte_rtn

        if(copy_to_user(buf, aesd_entry_ret->buffptr, count))
        {

        }
    }

    mutex_unlock(&aesd_device.aesd_char_mut);


    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    void *user_krnl_buf = NULL;
    int i=0;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);


    //filp->private_data.circular_buffer;
    mutex_lock(&filp->private_data.aesd_char_mut);


    //allocating space for user data in kernel space
    user_krnl_buf = kmalloc(count,GFP_KERNEL);
    if (copy_from_user(user_krnl_buf, buf, count)) 
    {
        retval = -EFAULT;
        goto out;
    }

    for(i=0;i<count;i++)
    {
        if(user_krnl_buf[i] == "\n")
        {
            //Write to buffer
            
        }
        else
        {
            //Increment and go ahead
            //also check if no '\n' was recieved 

        }
    }









    out:
        mutex_unlock(&aesd_device.aesd_char_mut);



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

    //Requesting for dynamic major number -> aesdchar is the driver name
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    
    //
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */

    //init_MUTEX(&aesd_dev.aesd_char_mut);
    mutex_init(&aesd_dev.aesd_char_mut);
    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    mutex_destroy(&aesd_dev.aesd_char_mut);
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
