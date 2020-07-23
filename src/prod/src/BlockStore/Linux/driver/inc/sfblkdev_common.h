// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under GPLv2 license.
#ifndef __SFBLKDEV_COMMON_H
#define __SFBLKDEV_COMMON_H

#define SFBLKDEV_MAX_VOLUME_NAME_LEN   50
#define SFBLKDEV_CTL_DEVICE_NAME "sfblkdevctrl"
#define SFBLKDEV_CREATE_DISK  _IOWR('B', 1, struct create_disk_param)
#define SFBLKDEV_DELETE_DISK  _IOWR('B', 2, struct delete_disk_param)
#define SFBLKDEV_GET_DISK     _IOWR('B', 3, struct get_disk_param)
#define SFBLKDEV_GET_DISK_LOCAL  _IOWR('B', 4, struct get_disk_param)
#define SFBLKDEV_READ_DATA  _IOWR('B', 5, struct rw_disk_param)
#define SFBLKDEV_WRITE_DATA  _IOWR('B', 6, struct rw_disk_param)
#define SFBLKDEV_REFRESH_DISK _IOWR('B', 7, struct refresh_disk_param)
#define SFBLKDEV_SHUTDOWN_DISK _IOWR('B', 8, struct shutdown_disk_param)

#define SFBLKDEV_MAX_DEV_NAME_LEN   32

#define BYTES_TO_MB(x)  ((x)/MB(1))
#define MB(x)   ((x)<<20)
#define GB(x)   ((x)<<30)
#define TB(x)   ((x)<<40)

struct sfblkdisk_info
{
    int open_count;
    unsigned long long size;
    char volume_name[SFBLKDEV_MAX_VOLUME_NAME_LEN];
    char device_name[SFBLKDEV_MAX_VOLUME_NAME_LEN];
};

struct sfblkstore_svc
{
    unsigned int ip;
    unsigned short port;
};

typedef enum 
{
    RAMDISK_SFBLKDEV_TYPE,
    NW_SFBLKDEV_TYPE,
    MAX_SFBLKDEV_TYPE 
}sfblkdev_type;

struct create_disk_param
{
    sfblkdev_type type;
    struct sfblkstore_svc remote;
    unsigned long long size;
    char volume_name[SFBLKDEV_MAX_VOLUME_NAME_LEN];
    //driver fills up this with the actual device file name.
    char device_name[SFBLKDEV_MAX_DEV_NAME_LEN];
};

struct delete_disk_param
{
    sfblkdev_type type;
    char volume_name[SFBLKDEV_MAX_VOLUME_NAME_LEN];
    struct sfblkstore_svc remote;
};

struct get_disk_param
{
    sfblkdev_type type;
    struct sfblkstore_svc remote;
    //driver fills up this with the valid disk count.
    int valid_count;
    //points to `count` struct sfblkdisk_info
    int count;
    struct sfblkdisk_info *disks;
};

struct rw_disk_param
{
    sfblkdev_type type;
    struct sfblkstore_svc remote;
    char volume_name[SFBLKDEV_MAX_VOLUME_NAME_LEN];
    uint64_t pos;
    int no_of_pages;    //number of pages.
    void *data; //user data, size should be no_of_pages*4096. 
};

struct shutdown_disk_param 
{
    sfblkdev_type type;
    struct sfblkstore_svc remote;
};

struct refresh_disk_param
{
    sfblkdev_type type;
    struct sfblkstore_svc remote;
};
#endif
