// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under GPLv2 license.
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/uio.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/preempt.h>
#include "sfblkdev.h"

static struct cdev ctrldev;
struct class *ctrldev_class;
static dev_t ctrldev_num;
struct sfblkdev_ctrl_ops *g_sfblkdev_ctrl_ops[MAX_SFBLKDEV_TYPE];

static int sfblkdev_ctrl_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int sfblkdev_ctrl_release(struct inode *inode, struct file *file)
{
    return 0;
}

/*blockstore control operations.*/
static long sfblkdev_ctrl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long err = 0;
    //int count;
    struct create_disk_param create_disk;
    struct delete_disk_param delete_disk;
    struct get_disk_param get_disk;
    struct rw_disk_param rw_disk;
    struct refresh_disk_param refresh_disk;
    struct shutdown_disk_param shutdown_disk;
 
    switch(cmd)
    {
        case SFBLKDEV_CREATE_DISK:
            err = copy_from_user(&create_disk, (void *)arg, sizeof(create_disk));
            if (err != 0)
            {
                TRACE_ERR("Failed to copy disk info for SFBLKDEV_CREATE_DISK target_copy_count:%lu actual_copy_count:%ld\n",
                           sizeof(create_disk), err);
                return err;
            }
            
            if(!g_sfblkdev_ctrl_ops[create_disk.type])
                return -EIO;

            if(!g_sfblkdev_ctrl_ops[create_disk.type]->create_disk)
                return -EIO;

            err = g_sfblkdev_ctrl_ops[create_disk.type]->create_disk(&create_disk, 0);
            copy_to_user((void *)arg, &create_disk, sizeof(create_disk));
            break;

        case SFBLKDEV_DELETE_DISK:
            err = copy_from_user(&delete_disk, (void *)arg, sizeof(delete_disk));
            if (err != 0)
            {
                TRACE_ERR("Failed to copy disk info for SFBLKDEV_DELETE_DISK target_copy_count:%lu actual_copy_count:%ld\n",
                          sizeof(delete_disk), err);
                return err;
            }

            if(!g_sfblkdev_ctrl_ops[delete_disk.type])
                return -EIO;

            if(!g_sfblkdev_ctrl_ops[delete_disk.type]->delete_disk)
                return -EIO;

            err = g_sfblkdev_ctrl_ops[delete_disk.type]->delete_disk(&delete_disk);
            copy_to_user((void *)arg, &delete_disk, sizeof(delete_disk));
            break;

        case SFBLKDEV_GET_DISK:
            err = copy_from_user(&get_disk, (void *)arg, sizeof(get_disk));
            if (err != 0)
            {
                TRACE_ERR("Failed to copy disk info for SFBLKDEV_GET_DISK target_copy_count:%lu actual_copy_count:%ld\n",
                           sizeof(get_disk), err);
                return err;
            }

            if(!g_sfblkdev_ctrl_ops[get_disk.type])
                return -EIO;

            if(!g_sfblkdev_ctrl_ops[get_disk.type]->get_disks)
                return -EIO;

            err = g_sfblkdev_ctrl_ops[get_disk.type]->get_disks(&get_disk);

            copy_to_user((void *)arg, &get_disk, sizeof(get_disk));
            break;

        case SFBLKDEV_GET_DISK_LOCAL:
            err = copy_from_user(&get_disk, (void *)arg, sizeof(get_disk));
            if (err != 0)
            {
                TRACE_ERR("Failed to copy disk info for SFBLKDEV_GET_DISK_LOCAL target_copy_count:%lu actual_copy_count:%ld\n",
                          sizeof(get_disk), err);
                return err;
            }
            if(!g_sfblkdev_ctrl_ops[get_disk.type])
                return -EIO;

            if(!g_sfblkdev_ctrl_ops[get_disk.type]->get_disks_local)
                return -EIO;

            err = g_sfblkdev_ctrl_ops[get_disk.type]->get_disks_local(&get_disk);

            copy_to_user((void *)arg, &get_disk, sizeof(get_disk));
            break;

        case SFBLKDEV_READ_DATA:
            err = copy_from_user(&rw_disk, (void *)arg, sizeof(rw_disk));
            if (err != 0)
            {
                TRACE_ERR("Failed to copy disk info for SFBLKDEV_READ_DATA target_copy_count:%lu actual_copy_count:%ld\n",
                          sizeof(rw_disk), err);
                return err;
            }
            if(!g_sfblkdev_ctrl_ops[rw_disk.type])
                return -EIO;

            if(!g_sfblkdev_ctrl_ops[rw_disk.type]->read_data)
                return -EIO;

            err = g_sfblkdev_ctrl_ops[rw_disk.type]->read_data(&rw_disk);

            copy_to_user((void *)arg, &rw_disk, sizeof(rw_disk));
            break;

        case SFBLKDEV_WRITE_DATA:
            err = copy_from_user(&rw_disk, (void *)arg, sizeof(rw_disk));
            if (err != 0)
            {
                TRACE_ERR("Failed to copy disk info for SFBLKDEV_WRITE_DATA target_copy_count:%lu actual_copy_count:%ld\n",
                          sizeof(rw_disk), err);
                return err;
            }
            if(!g_sfblkdev_ctrl_ops[rw_disk.type])
                return -EIO;

            if(!g_sfblkdev_ctrl_ops[rw_disk.type]->write_data)
                return -EIO;

            err = g_sfblkdev_ctrl_ops[rw_disk.type]->write_data(&rw_disk);

            copy_to_user((void *)arg, &rw_disk, sizeof(rw_disk));

            break;
        
        case SFBLKDEV_REFRESH_DISK:

            err = copy_from_user(&refresh_disk, (void *)arg, sizeof(refresh_disk));
            if (err != 0)
            {
                TRACE_ERR("Failed to copy disk info for SFBLKDEV_REFRESH_DISK target_copy_count:%lu actual_copy_count:%ld\n",
                          sizeof(refresh_disk), err);
                return err;
            }

            if(!g_sfblkdev_ctrl_ops[refresh_disk.type])
                return -EIO;

            if(!g_sfblkdev_ctrl_ops[refresh_disk.type]->refresh_disks)
                return -EIO;

            err = g_sfblkdev_ctrl_ops[refresh_disk.type]->refresh_disks(&refresh_disk);
            break;

        case SFBLKDEV_SHUTDOWN_DISK:

            err = copy_from_user(&shutdown_disk, (void *)arg, sizeof(shutdown_disk));
            if (err != 0)
            {
                TRACE_ERR("Failed to copy disk info for SFBLKDEV_SHUTDOWN_DISK target_copy_count:%lu actual_copy_count:%ld\n",
                          sizeof(shutdown_disk), err);
                return err;
            }

            if(!g_sfblkdev_ctrl_ops[shutdown_disk.type])
                return -EIO;

            if(!g_sfblkdev_ctrl_ops[shutdown_disk.type]->shutdown_disks)
                return -EIO;

            err = g_sfblkdev_ctrl_ops[shutdown_disk.type]->shutdown_disks(&shutdown_disk);
            break;
 
        default:
            TRACE_ERR("Received invalid IOCTL!"); 
            return -EINVAL;
    }
    return err;
}

int register_sfblkdev_ctrl_ops(struct sfblkdev_ctrl_ops *op)
{
    if(g_sfblkdev_ctrl_ops[op->type])
        return -1;
    g_sfblkdev_ctrl_ops[op->type] = op;
    return 0;
}

void unregister_sfblkdev_ctrl_ops(struct sfblkdev_ctrl_ops *op)
{
    g_sfblkdev_ctrl_ops[op->type] = NULL;
    return;
}

const struct file_operations sfblkdev_ctrl_fops = {
    .open           = sfblkdev_ctrl_open,
    .release        = sfblkdev_ctrl_release,
    .unlocked_ioctl = sfblkdev_ctrl_ioctl,
    .owner          = THIS_MODULE,
};

int initialize_sfblkdev_ctrl(void)
{
    struct cdev *cdev;
    struct class *cl;
    int num_minor = 1;
    int err = 0;
    struct device *dev;

    cdev = &ctrldev;

    err = alloc_chrdev_region(&ctrldev_num, 0, num_minor, SFBLKDEV_CTL_DEVICE_NAME);
    if (err < 0) {
        TRACE_ERR("alloc_chrdev_region failed with error %d", err);
        return err;
    }

    ctrldev_class = cl = class_create(THIS_MODULE, SFBLKDEV_CTL_DEVICE_NAME);
    if (IS_ERR(cl)) {
        err = (int) PTR_ERR(cl);
        TRACE_ERR("class_create failed with error %d", err);
        goto exit_class;
    }

    //Create device file.
    dev = device_create(cl, NULL, ctrldev_num, NULL, SFBLKDEV_CTL_DEVICE_NAME);
    if (IS_ERR(dev)) {
        err = (int) PTR_ERR(dev);
        TRACE_ERR("device_create failed with error %d", err);
        goto exit_device;
    }

    cdev_init(cdev, &sfblkdev_ctrl_fops);
    err = cdev_add(cdev, ctrldev_num, num_minor);
    if (err < 0) {
        TRACE_ERR("cdev_add failed with error %d", err);
        goto exit_device_add;
    }

    TRACE_INFO("Controller device registered with Major %d, Minor %d", MAJOR(ctrldev_num), MINOR(ctrldev_num));
    return 0;

exit_device_add:
    device_destroy(cl, ctrldev_num);
exit_device:
    class_destroy(cl);
exit_class:
    unregister_chrdev_region(ctrldev_num, num_minor);
    return err;
}

void uninitalize_sfblkdev_ctrl(void)
{
    int num_minor = 1;
    device_destroy(ctrldev_class, ctrldev_num);
    cdev_del(&ctrldev);
    class_destroy(ctrldev_class);
    unregister_chrdev_region(ctrldev_num, num_minor);
    return;
}
