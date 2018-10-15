#include <linux/fs.h>  //get the functions that are related to device sriver coding 
#include <asm/uaccess.h> //enable to get data from userspace to kernel and vice versa
#include <linux/slab.h> 

#include <linux/init.h>
#include <linux/module.h>
#define BUFFER_SIZE 1024

int counterOpen = 0;
int counterClose = 0;
//device_buffer is a pointer
char device_buffer[BUFFER_SIZE]; 

//ssize_t is signed version, size_t is unsigned version integer
ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
    /* *buffer is the userspace buffer to where you are writing the data you want to be read from the device file*/
    /* length is the length of the userspace buffer*/
    /* offset will be set to current position of the opened file after read*/
    /* copy_to_user function: source is device_buffer and destination is the userspace buffer *buffer */
    
    printk(KERN_ALERT "Right now we are in read mode. \n");
   
    //The case is to check when the offset is beyond the end of buffer
    //No reason to read outside of buffer 
    if(*offset >= BUFFER_SIZE)
    {
       *offset = BUFFER_SIZE - 1; 
    }

    //The case when we are trying to read more bytes than contained in the buffer.
    //We will just read as many bytes as we can.
    if((*offset + length) >= BUFFER_SIZE)
    {
        length = (BUFFER_SIZE - *offset - 1);
    }
   
    //copy_to_user(destination, source, size) 
    copy_to_user(buffer, device_buffer, length);
    printk(KERN_ALERT "Reading the number of byte read: %d", length);
    return length;
}

//ssize_t is signed version, size_t is unsigned version integer
ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
    /* *buffer is the userspace buffer where you are writing the data you want to be written in the device file*/
    /* length is the length of the userspace buffer*/
    /* current position of the opened file*/
    /* copy_from_user function: destination is device_buffer and source is the userspace buffer *buffer */

    printk(KERN_ALERT "You are in Write function. \n");
    
    // increment offset to end of current buffer
    *offset = strlen(&device_buffer[0]);

    //The case when the offset is beyond the end of the file/buffer.
    //We don't want to write anything that is outside of our buffer!
    if(*offset >= BUFFER_SIZE)
    {
        *offset = BUFFER_SIZE;
    }
    //The case when we are trying to write more bytes than are contained in the buffer.
    //We will just write as many bytes as we can.
    if((*offset + length) >= BUFFER_SIZE){
        length = (BUFFER_SIZE - *offset);
    }
    //copy_from_user(destination, source, size)
    copy_from_user(device_buffer, buffer, length);

    printk(KERN_ALERT "You have wrote: %u", length);
    return length;
}

loff_t simple_char_driver_llseek (struct file *pfile, loff_t offset, int whence)
{
    switch(whence){
        case 0:  //set CURRENT
            pfile->f_pos= offset;
            break;

        case 1: //set increment 
            pfile->f_pos=pfile->f_pos+offset;
            break;

        case 2:
            pfile -> f_pos =device_buffer - offset;
            break;
        //default: /* can't happen */
        //    return -EINVAL;
    }
    return 0;
}

//to identify the device being opened is to look at the minor number stored in the inode structure. If you register your device with register_chrdev, you must use this technique
int simple_char_driver_open(struct inode *pinode, struct file *pfile)
{
    //ptrNode represents the actual physical file on the hard disk
    //ptrFile is the abstract open file that contains all the necessary file operations in the file_operations structure
    counterOpen ++;
    printk(KERN_ALERT "Simple Character Driver has been opened %d time", counterOpen);
    return 0;
}

int simple_char_driver_close(struct inode *pinode, struct file *pfile)
//Virtual File System lies in Kernel space just above character driver and low-level I/F. This VFS decodes the file type and transfers the file operations to appropriate device driver. Now for VFS to pass the device file operations onto the driver, it should have been informed about it. That is what we call as registering the file operations by driver with VFS. Basically it involves by using file_operations structure, which holds appropriate functions to be performed during usage.
{
    counterClose ++;
    printk(KERN_ALERT "Simple Character Driver has been closed %d times", counterClose);
    return 0;
}

struct file_operations simple_char_file_operation = {
   //Similar file under /lib/modules/4.10.0-33-generic/build/include/linux
   .owner = THIS_MODULE,
   .read = simple_char_driver_read,
   .write = simple_char_driver_write,
   .open = simple_char_driver_open, 
   .llseek = simple_char_driver_llseek,
   .release = simple_char_driver_close,
};

static int simple_char_driver_init(void)
{
    //Initialization functions should be declared static, since they are not meant to be visible outside the specific file; there is no hard rule about this, though, as no function is exported to the rest of the kernel unless explicitly requested
    printk(KERN_ALERT "Simple Character device is called! \n");
    //FYI register_chrdev and deregister_chrdev is prototyped under #include <linux/fs.h>
    //Within the driver, in order to link it with its corresponding /dev file in kernel space, the register_chrdev function is used
    register_chrdev(230, "simple_char_device", &simple_char_file_operation);
    //device_buffer = kmalloc(size, GFP_KERNEL);
    kmalloc(BUFFER_SIZE, GFP_KERNEL);
    return 0;
}
static void simple_char_driver_exit(void)
{
    //The cleanup function has no value to return, so it is declared void
    printk(KERN_ALERT "Simple Character device run away! \n");
    //Freeing Major number
    unregister_chrdev(230, "simple_char_device");
    kfree(device_buffer);
    //return 0;
}

module_init(simple_char_driver_init); //module_init to point to simple_char_driver
module_exit(simple_char_driver_exit); //module_exit to point to simple_char driver
