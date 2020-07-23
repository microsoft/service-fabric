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

int sfblkdev_entry(void)
{
    int err;

    //Initialize Control Device.
    err = initialize_sfblkdev_ctrl();
    if (err < 0)
        return err;

    //Initialize BlockStore device. 
    err = initialize_sfblkdev();
    if (err < 0) {
        uninitalize_sfblkdev_ctrl();
        return err;
    }
    return 0;
}

void sfblkdev_exit(void)
{
    //Cleanup BlockDevice.
    uninitalize_sfblkdev();

    //Cleanup control device.
    uninitalize_sfblkdev_ctrl();
    return;
}

MODULE_LICENSE("Dual MIT/GPL");
MODULE_VERSION("1.0");
module_init(sfblkdev_entry);
module_exit(sfblkdev_exit);
