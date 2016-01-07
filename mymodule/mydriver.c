/* **************** LDD:2.0 s_04/sample_driver.c **************** */
/*
 * The code herein is: Copyright Jerry Cooperstein, 2012
 *
 * This Copyright is retained for the purpose of protecting free
 * redistribution of source.
 *
 *     URL:    http://www.coopj.com
 *     email:  coop@coopj.com
 *
 * The primary maintainer for this code is Jerry Cooperstein
 * The CONTRIBUTORS file (distributed with this
 * file) lists those known to have contributed to the source.
 *
 * This code is distributed under Version 2 of the GNU General Public
 * License, which you should have received with the source.
 *
 */
/* 
Sample Character Driver 
@*/

#include <linux/module.h>	/* for modules */
#include <linux/fs.h>		/* file_operations */
#include <linux/uaccess.h>	/* copy_(to,from)_user */
#include <linux/init.h>		/* module_init, module_exit */
#include <linux/slab.h>		/* kmalloc */
#include <linux/cdev.h>		/* cdev utilities */
#include <linux/list.h>     /* LinkedList */
#include <linux/kernel.h>   /* printk and container_of */
#include <linux/device.h>   /* Dynamically create devices*/
#include <linux/mutex.h>    /* mutexes*/

/*Define initial/default values and structures*/
LIST_HEAD(mylinkedlist) ;

struct asp_mycdrv
{
    struct list_head list;
    struct cdev dev;
    char *ramdisk;
    //struct mutex mutex;
    struct semaphore sem;
    int devNo;
};

// declare device class
struct class *myDriverClass;
 
int my_major = 700;//default value
int my_minor = 0;
int nr_devs = 3;// as per specification

module_param(nr_devs,int,0);

/*********************************************************************************************/
#define MYDEV_NAME "mycdrv"

#define ramdisk_size (size_t) (16*PAGE_SIZE)



static int mycdrv_open(struct inode *inode, struct file *file)
{
    struct asp_mycdrv *tmp;
    tmp = container_of(inode->i_cdev, struct asp_mycdrv, dev);
    file->private_data = tmp;
    //down the semaphore 
    down_interruptible(&tmp->sem);
    //mutex_lock_interruptible(&tmp->mutex);
    if ( (file->f_flags & O_ACCMODE) == O_WRONLY)
    {
       memset(tmp->ramdisk, 0, ramdisk_size);
    }
	pr_info(" OPENING device: %s:\n\n", MYDEV_NAME);
	return 0;
}


static int mycdrv_release(struct inode *inode, struct file *file)
{
	struct asp_mycdrv *tmp = file->private_data;
	//release the semaphore
	up(&tmp->sem);
	//mutex_unlock(&tmp->mutex);
	pr_info(" CLOSING device: %s:\n\n", MYDEV_NAME);
	return 0;
}

static ssize_t
mycdrv_read(struct file *file, char __user * buf, size_t lbuf, loff_t * ppos)
{
	int nbytes;
	struct asp_mycdrv *tmp = file->private_data;
	
	if ((lbuf + *ppos) > ramdisk_size) {
		pr_info("trying to read past end of device,"
			"aborting because this is just a stub!\n");
		return 0;
	}
	nbytes = lbuf - copy_to_user(buf, tmp->ramdisk + *ppos, lbuf);
	*ppos += nbytes;
	pr_info("\n READING function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos);
	return nbytes;
}

static ssize_t
mycdrv_write(struct file *file, const char __user * buf, size_t lbuf,
	     loff_t * ppos)
{
	int nbytes;
	struct asp_mycdrv *tmp = file->private_data;
	if ((lbuf + *ppos) > ramdisk_size) {
		pr_info("trying to read past end of device,"
			"aborting because this is just a stub!\n");
		return 0;
	}
	nbytes = lbuf - copy_from_user(tmp->ramdisk + *ppos, buf, lbuf);
	*ppos += nbytes;
	pr_info("\n WRITING function, nbytes=%d, pos=%d\n", nbytes, (int)*ppos);
	return nbytes;
}

static const struct file_operations mycdrv_fops = {
	.owner = THIS_MODULE,
	.read = mycdrv_read,
	.write = mycdrv_write,
	.open = mycdrv_open,
	.release = mycdrv_release,
};

/*static int __init my_init(void)
{
	ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
	first = MKDEV(my_major, my_minor);
	register_chrdev_region(first, count, MYDEV_NAME);
	my_cdev = cdev_alloc();
	cdev_init(my_cdev, &mycdrv_fops);
	cdev_add(my_cdev, first, count);
	pr_info("\nSucceeded in registering character device %s\n", MYDEV_NAME);
	return 0;
}*/


/* Set up individual devices*/
static void setup_mydevice(struct asp_mycdrv *mydev, int index)
{
    int err = 0;
    int devno = MKDEV(my_major,my_minor + index);
    cdev_init(&mydev->dev,&mycdrv_fops);
    mydev->dev.owner = THIS_MODULE;
    mydev->dev.ops = &mycdrv_fops;
    err = cdev_add(&mydev->dev,devno,1);
    if (err)
    {
        printk(KERN_NOTICE "Error %d adding device%d", err, index);
    }
}

/*******************************************************************************************/
/* Initialization method for my char driver*/
static int __init mydriver_init(void)
{
    
    int result;
    int i = 0;//used for loops
    dev_t dev = 0;
    struct asp_mycdrv *tmp;
    // get major number dynamically from the kernel.
    result = alloc_chrdev_region(&dev, my_minor, nr_devs,"myCharDriver");
    my_major = MAJOR(dev);
    if (result < 0) 
    {
        printk(KERN_WARNING "myCharDriver: can't get major %d\n", my_major);
        return result;
    } 
    // create device class
    myDriverClass = class_create(THIS_MODULE,"my_character_drivers");
    //time to initialize each device
    for (i = 0;i< nr_devs;i++)
    {
        tmp = kmalloc(sizeof(struct asp_mycdrv), GFP_KERNEL);
        memset(tmp, 0, sizeof(struct asp_mycdrv));
        tmp->ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
        sema_init(&tmp->sem,1);
        //mutex_init(&tmp->mutex);
        //add node to the list
        INIT_LIST_HEAD(&tmp->list);
        list_add(&tmp->list, &mylinkedlist);
        //initialize individual devices
        setup_mydevice(tmp,i);
        //create this device
        device_create(myDriverClass,NULL,MKDEV(my_major,my_minor + i),NULL,"mycdrv%d",i);
        pr_info("\n Number of devices to be set up are %d, Completed loop %d",nr_devs,i);
        
    }
    
    return 0; 
}
/*********************************************************************************************/

/* Clean up stuff*/
static void __exit mydriver_exit(void)
{
    int i;
    dev_t devno = MKDEV(my_major, my_minor);
    struct asp_mycdrv *tmp;
    struct list_head *position,*rem;
    /*traverse the list and free resources*/
    list_for_each ( position , &mylinkedlist )
    {
        tmp = list_entry(position, struct asp_mycdrv, list);
        cdev_del(&tmp->dev);
        kfree(tmp->ramdisk);
    }
    unregister_chrdev_region(devno,nr_devs);
    /*Clean the linked list*/
    tmp = NULL;
    position = NULL;
    
    list_for_each_safe ( position ,rem, &mylinkedlist )
    {
        tmp = list_entry(position, struct asp_mycdrv, list);
        list_del(position);
        kfree(tmp);
    }
    /*delete the created devices*/
    for (i = 0;i< nr_devs;i++)
    {
        device_destroy(myDriverClass,MKDEV(my_major,my_minor + i));
    }
    class_destroy(myDriverClass);
}

/**********************************************************************************************/

/*static void __exit my_exit(void)
{
	cdev_del(my_cdev);
	unregister_chrdev_region(first, count);
	pr_info("\ndevice unregistered\n");
	kfree(ramdisk);
}*/

module_init(mydriver_init);
module_exit(mydriver_exit);

MODULE_AUTHOR("Jerry Cooperstein & Saumitra Aditya");
MODULE_LICENSE("GPL v2");
