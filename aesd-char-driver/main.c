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

//Variable for storing incomplete packet and its size
char *incomplete_packet;
size_t incomplete_packet_size = 0;

MODULE_AUTHOR("Kevin Tom");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;





int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    PDEBUG("open");
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
    ssize_t retval = 0, no_byte_to_copy = 0;
    size_t entry_offset_byte_rtn;
    struct aesd_buffer_entry *aesd_entry_ret;
    struct aesd_dev *driver_data = filp->private_data;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    retval = mutex_lock_interruptible(&driver_data->aesd_char_mut);
    if(retval != 0)
    {
        PDEBUG("Could not aquire mutex");
        return retval;
    }
    
    aesd_entry_ret = aesd_circular_buffer_find_entry_offset_for_fpos(&driver_data->circular_buffer, *f_pos, &entry_offset_byte_rtn);
    if(aesd_entry_ret == NULL)
    {
        *f_pos = 0;
        PDEBUG("Nothing was read");
        retval = 0;
        goto out; 
    }

    //Calcualting number of bytes to copy
    no_byte_to_copy = aesd_entry_ret->size - entry_offset_byte_rtn;
    if(count < no_byte_to_copy)
        no_byte_to_copy = count;
    
    *f_pos += no_byte_to_copy; //Updating file positin for next time

    //Copying to user space
    if( copy_to_user(buf, (aesd_entry_ret->buffptr + entry_offset_byte_rtn), no_byte_to_copy) )
    {
        retval = -EFAULT;
        PDEBUG("copy_from_user() failed");
        goto out;
    }

    //Assigning return value to number of bytes that was copied
    retval = no_byte_to_copy;


    out:
    mutex_unlock(&driver_data->aesd_char_mut); //Unlocking mutex
    return retval;
}







ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{

    ssize_t retval = -ENOMEM;
    char *user_krnl_buf = NULL, *packet = NULL, *c_buf_return;
    size_t  size_of_current_packet, start_of_current_packet = 0;
    int i=0;
    struct aesd_dev *driver_data = filp->private_data;
    
    //Variable used for writing into buffer
    struct aesd_buffer_entry buffer_entry;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);


    //filp->private_data.circular_buffer;
    retval = mutex_lock_interruptible(&driver_data->aesd_char_mut);
    if(retval != 0)
    {
        PDEBUG("Could not aquire mutex");
        return retval;
    }


    //allocating space for user data in kernel space
    user_krnl_buf = (char *)kmalloc(count,GFP_KERNEL);
    if(user_krnl_buf == NULL)
    {
        PDEBUG("kmalloc() failed");
        return retval;
    }


    //copying data from userspace to kernel space
    if (copy_from_user(user_krnl_buf, buf, count)) 
    {
        retval = -EFAULT;
        PDEBUG("copy_from_user() failed");
        goto out;
    }


    //Trying to find "\n"
    for(i=0;i<count;i++)
    {
        if(user_krnl_buf[i] == '\n')
        {

            //Calcualting size of the current packet
            size_of_current_packet = ( ((i - start_of_current_packet) + 1) * sizeof(user_krnl_buf[0]) );

            //Checking if incomplete packet is present
            if(incomplete_packet_size > 0)
            {
                //have to  realloc the global buffer
                incomplete_packet = (char *)krealloc(incomplete_packet,(incomplete_packet_size+size_of_current_packet),GFP_KERNEL);
                if(incomplete_packet == NULL)
                {
                    PDEBUG("krealloc() failed");
                    goto out;
                }
                
                //copying content into packet
                memcpy( (incomplete_packet + (incomplete_packet_size/sizeof(user_krnl_buf[0]))) , (user_krnl_buf + (start_of_current_packet*(sizeof(user_krnl_buf)))) ,size_of_current_packet);

                //Setting variables to write into the buffer
                buffer_entry.buffptr =(const char *)incomplete_packet;
                buffer_entry.size = incomplete_packet_size+size_of_current_packet;                 
            }
            else
            {

                packet = (char *)kmalloc(size_of_current_packet, GFP_KERNEL);
                if(packet == NULL)
                {
                    PDEBUG("kmalloc() failed");
                    goto out;
                }


                //copying content into packet
                memcpy(packet, (user_krnl_buf + start_of_current_packet) ,size_of_current_packet);

                //Setting variables to write into the buffer
                buffer_entry.buffptr = (const char *)packet;
                buffer_entry.size = size_of_current_packet;
            }

            //Add it to buffer
            c_buf_return = aesd_circular_buffer_add_entry(&driver_data->circular_buffer, &buffer_entry);
            if(c_buf_return != NULL)
            {
                kfree(c_buf_return);
                PDEBUG("Freed element from circular buffer");
            }
                


            //Updating start of next packet position
            start_of_current_packet = i+1;

            //No incomplete packet
            incomplete_packet_size = 0;
        }

        //Packet is incomplete
        if((i+1) == count && user_krnl_buf[i]!= '\n')
        {
            
            //Size of incomplete packet in bytes
            size_of_current_packet = ( ((i - start_of_current_packet) + 1) * sizeof(user_krnl_buf[0]) );

            //If previous packet was incomplete, realloc and add to it
            if(incomplete_packet_size > 0)
            {
                //have to  realloc
                incomplete_packet = (char *)krealloc(incomplete_packet,(incomplete_packet_size+size_of_current_packet),GFP_KERNEL);
                if(incomplete_packet == NULL)
                {
                    PDEBUG("krealloc() failed");
                    goto out;
                }
                memcpy( (incomplete_packet + (incomplete_packet_size/sizeof(user_krnl_buf[0]))) , (user_krnl_buf + (start_of_current_packet*(sizeof(user_krnl_buf)))) ,size_of_current_packet);
                incomplete_packet_size += size_of_current_packet;
            }
            else //if there was no data prevously
            {
                incomplete_packet = (char *)kmalloc(size_of_current_packet,GFP_KERNEL);
                if(incomplete_packet == NULL)
                {
                    PDEBUG("kmalloc() failed");
                    goto out;
                }
                memcpy( incomplete_packet , (user_krnl_buf + (start_of_current_packet*(sizeof(user_krnl_buf)))) ,size_of_current_packet);
                incomplete_packet_size = size_of_current_packet;
            }
        }

    }

    retval = count;

    out:
    mutex_unlock( &driver_data->aesd_char_mut );
    kfree(user_krnl_buf);
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
    
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));


    
    mutex_init(&aesd_device.aesd_char_mut);

    PDEBUG("MUTEX Intialized");
    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{

    int start, stop;
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    start = aesd_device.circular_buffer.out_offs;
    stop = aesd_device.circular_buffer.in_offs;

    cdev_del(&aesd_device.cdev);


    for ( ; start != stop  || aesd_device.circular_buffer.full ; start = (start+1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    {
        kfree(aesd_device.circular_buffer.entry[start].buffptr);
        aesd_device.circular_buffer.full = false;
    }

    mutex_destroy(&aesd_device.aesd_char_mut);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
