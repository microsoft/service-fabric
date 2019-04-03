// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under GPLv2 license.
#ifndef _SFBLKDEV_H
#define _SFBLKDEV_H
#include "sfblkdev_ctrl.h"
#include <linux/kfifo.h>
#define BLK_MAX_SEGMENT_LMT     (MB(1))
#define BLK_SECTOR_SIZE (512)
#define SFBLKDEV_NAME "sfblkdev"
#define MAX_LU_REGISTRATION 100
#define MAX_DEVICE_SMBOLICNAME_LEN  100
#define SFBLKDEV_RETRY_DELAY_IN_MS (5)

struct sfblkdisk_stats
{
    //counts successfully submitted transaction to work queue.
    atomic64_t submitted;

    //counts both failed and succeeded transactions.
    atomic64_t completed;
    
    //failed transactions.
    atomic64_t failed;

    //succeeded transactions.
    atomic64_t succeeded;

    //retry count
    atomic64_t send_retries;
    atomic64_t recv_retries;

    //total bytes read/wrtie
    atomic64_t total_bytes_read;
    atomic64_t total_bytes_written;
};

struct sfreq_state 
{
    struct delayed_work dwork;
    struct sfblkdisk *bdev;
    struct request *req;
    uint64_t tracking_id;
    unsigned long start_jiff;
    int retry_count;
};

struct sfblksvc_conn
{
    struct socket *sock;
    atomic64_t total_bytes_sent;
    atomic64_t total_bytes_rcvd;
};

enum sfblkdev_disk_state
{
    ready,
    marked_for_delete,
    deleted
};

struct sfblkdisk
{
    struct hlist_node list;
    struct list_head del_list;
    sfblkdev_type type;
    int major;
    int minor;
    dev_t devno;
    struct gendisk *disk;
    size_t size;
    spinlock_t qlock;
    spinlock_t lock;
    spinlock_t conn_lock;
    int open_count;
    enum sfblkdev_disk_state state;
    void *ramdisk;
    char device_name[SFBLKDEV_MAX_DEV_NAME_LEN];
    char volume_name[SFBLKDEV_MAX_VOLUME_NAME_LEN];
    int luid_name_len;
    struct sfblkdisk_stats stats;
    struct sfblkstore_svc remote;
    struct workqueue_struct *wq;
    wchar_t volume_name_uni[SFBLKDEV_MAX_VOLUME_NAME_LEN];
};

typedef enum
{
    SIZE_IN_MB,
    SIZE_IN_GB,
    SIZE_IN_TB
}disksizeunit;

typedef enum _blockcmd
{
    read=1,
    write,
    registerlu,
    unregisterlu,
    fetchregisterlu,
    umountlu,
    mountlu,
    operationcompleted = 0x827,
    operationfailed = 0xeee,
    invalidpayload = 0xfee,
    serverinvalidpayload = 0xfef
}blockcmd;

typedef struct _block_cmd_header
{
    blockcmd cmd;   //mode
    uint32_t payload_len_actual;    //sizeActualPayload
    uint64_t rw_offset; //offsetToRW
    uint32_t payload_len; //sizePayloadBuffer
    uint32_t luid_name_length; //lengthDeviceID
    uint64_t srb;  //addressSRB
    uint64_t tracking_id;
    void *luid_name;    //deviceID
    void *payload_data; //payloadData
}block_cmd_header;

#define SIZEOF_LUID_NAME (sizeof(uint16_t) * MAX_WCHAR_SIZE_LUID)
#define END_OF_PAYLOAD_MARKER   0xd0d0d0d0
#define SIZEOF_END_OF_PAYLOAD_MARKER    sizeof(uint32_t)
// Size of the payload that is exchanged between client (this driver) and the service
#define GET_PAYLOAD_HEADER_SIZE (sizeof(block_cmd_header) + SIZEOF_LUID_NAME + SIZEOF_END_OF_PAYLOAD_MARKER)

//fetchregisterlu
#define MAX_WCHAR_SIZE_LUID     50
#define MAX_LU_REGISTRATIONS    100
#define BLOCK_SIZE_MANAGEMENT_REQUEST   (16384)

typedef struct
{
    uint32_t luid_len;
    uint32_t disksize;
    disksizeunit unit;
    uint32_t mounted;
    uint16_t luid[MAX_WCHAR_SIZE_LUID];
} luid_registration_info, *plu_registration_info;

typedef struct
{
    uint32_t count_entries;
    luid_registration_info registration_info[MAX_LU_REGISTRATIONS]; 
} lu_registration_list, *plu_registration_list;

extern int convert_ascii_to_utf16(wchar_t *dest, const char *src, int len);
extern int convert_utf16_to_ascii(uint8_t *dest, wchar_t *src, int len);

#endif
