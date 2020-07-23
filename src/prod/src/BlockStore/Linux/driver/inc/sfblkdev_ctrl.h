// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under GPLv2 license.
#ifndef _SFBLKDEV_CTRL_H
#define _SFBLKDEV_CTRL_H
#include "sfblkdev_common.h"
#include <linux/kern_levels.h>


//1 - Enable Info
//2 - Enable Noise
//Warn and Error are always enabled.

#define TRACE_LEVEL 1
#if TRACE_LEVEL >= 1
#define TRACE_INFO(a, b ...)      printk("[%s @ %d] "a"\n", __FUNCTION__, __LINE__, ##b)
#else
#define TRACE_INFO(a, b ...)      
#endif

#if TRACE_LEVEL >= 2
#define TRACE_NOISE(a, b ...)     printk("[%s@%d] "a"\n", __FUNCTION__, __LINE__, ##b)
#else
#define TRACE_NOISE(a, b ...)     
#endif

#define TRACE_ERR(a, b ...)     printk("[%s@%d] "a"\n", __FUNCTION__, __LINE__, ##b)
#define TRACE_WARN(a, b ...)   printk("[%s@%d] "a"\n", __FUNCTION__, __LINE__, ##b)

typedef uint16_t wchar_t;

struct sfblkdev_ctrl_ops
{
    sfblkdev_type type;
    int (*create_disk) (struct create_disk_param *param, int skip_reg);
    int (*delete_disk) (struct delete_disk_param *param);
    int (*get_disks)   (struct get_disk_param *param);

    //Network type store can implement this differently to return the 
    //disk list from the local hash list without going to service.
    int (*get_disks_local) (struct get_disk_param *param);    

    int (*read_data) (struct rw_disk_param *param);
    int (*write_data) (struct rw_disk_param *param);
    int (*refresh_disks) (struct refresh_disk_param *param);
    int (*shutdown_disks) (struct shutdown_disk_param *param);
};

extern int initialize_sfblkdev_ctrl(void);
extern void uninitalize_sfblkdev_ctrl(void);
extern int initialize_sfblkdev(void);
extern void uninitalize_sfblkdev(void);

int register_sfblkdev_ctrl_ops(struct sfblkdev_ctrl_ops *op);
void unregister_sfblkdev_ctrl_ops(struct sfblkdev_ctrl_ops *op);

#endif
