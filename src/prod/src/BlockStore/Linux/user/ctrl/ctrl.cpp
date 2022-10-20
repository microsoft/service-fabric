// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <wchar.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <inttypes.h>
#include <sys/mount.h>
#include <sal.h>
#include "PAL.h"
#include <SFBDCtlLib.h>


static void show_help()
{
    printf("Create disk:\n");
    printf("\t./sfbdctl -c -n <device name> -s <size in MB> -i <blkstore service ip address string> -p <blkstore service port>\n");
    printf("Delete disk:\n");
    printf("\t./sfbdctl -d -n <device name> -i <blkstore service ip> -p <port>\n");
    printf("List disks locally:\n");
    printf("\t./sfbdctl -x \n");
    printf("List disks from blockstore service:\n");
    printf("\t./sfbdctl -l -i <blkstore service ip> -p <port>\n");

    //for rw
    printf("\t./sfbdctl -R -n <device name> -g <number of pages> -o <offset in pages> -i <blkstore service ip address string> -p <blkstore service port>\n");
    printf("\t./sfbdctl -W -n <device name> -g <number of pages> -o <offset in pages> -i <blkstore service ip address string> -p <blkstore service port>\n");

    printf("Explanation:\n");
    printf("\t-c : Creates disk with specified device name and size.\n");
    printf("\t-d : Deletes disk of specified device name.\n"); 
    printf("\t-x:  Lists all available disk from driver hash list.\n");
    printf("\t-l:  Lists all available disk from remote service.\n");
    printf("\t-t:  Type of disks, 0=ramdisk, 1=network\n");
    printf("Default value of ip is %s and port is %d\n", BLKSRV_IP, BLKSRV_PORT);
}

int main(int argc, char *argv[])
{
    int c;
    struct sfblkdisk_info *disk;
    int i;
    size_t size = 0;
    int index = -1;
    int cmd, ret;
    int count;
    int nr_max_disk_info = 100;
    int port = BLKSRV_PORT;
    int ip ;
    char *name = NULL;
    struct sfblkstore_svc remote;
    int type = 1;
    unsigned long long offset;
    int no_of_pages = 1;
    uint32_t *data;
    int fd;
    DWORD lastError;
    char device_file[SFBLKDEV_MAX_VOLUME_NAME_LEN];

    inet_pton(AF_INET, BLKSRV_IP, &ip);

    fd = (int)ConnectToDevice(&lastError);
    cmd = -1;
    while ( (c = getopt(argc, argv, "RWxlhcdt:n:s:r:p:o:g:i:")) != -1)
    {
        switch(c)
        {
            case 'c':
                if(cmd != -1){
                    printf("Invalid command. -c,-d,-x and -l options are mutually exclusive.");
                    goto exit;
                }
                cmd = 1; 
                break;
            case 'd':
                if(cmd != -1){
                    printf("Invalid command. -c,-d,-x and -l options are mutually exclusive.");
                    goto exit;
                }
                cmd = 2;
                break;
            case 'l':
                if(cmd != -1){
                    printf("Invalid command. -c,-d,-x and -l options are mutually exclusive.");
                    goto exit;
                }
                cmd = 3;
                break;
            case 'x':
                if(cmd != -1){
                    printf("Invalid command. -c,-d,-x and -l options are mutually exclusive.");
                    goto exit;
                }
                cmd = 4;
                break;
            case 'R':
                if(cmd != -1){
                    printf("Invalid command. -c,-d,-x and -l options are mutually exclusive.");
                    goto exit;
                }
                cmd = 5;
                break;
            case 'W':
                if(cmd != -1){
                    printf("Invalid command. -c,-d,-x and -l options are mutually exclusive.");
                    goto exit;
                }
                cmd = 6;
                break;
            case 'g':
                no_of_pages = atoll(optarg);
                break;
            case 'o':
                offset = atoll(optarg);
                break;
            case 't':
                type = atoi(optarg);
                break;
            case 's':
                size = MB(atoll(optarg));
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'i':
                inet_pton(AF_INET, optarg, &ip);
                break;
            case 'n':
                name = strdup(optarg);
                break;
            case 'h':
                show_help();
                goto noerror;
            default:
                printf("Invalid argument %c\n",c);
                show_help();
                goto exit;
        }
    }

    remote.ip = ip;
    remote.port = port;

    if(cmd == 1) {
        //Create device.
        if (size <= 0 || !name) {
            printf("Invalid name or size\n");
            goto exit;
        }
        ret = sfblkdev_create_disk((sfblkdev_type)type, name, size, &remote, device_file);

        if (ret < 0){
            printf("creating disk failed with error error %d\n", ret);
            goto exit;
        }
    } else if (cmd == 2) {
        //Delete Device
        if (!name) {
            printf("Invalid disk name\n");
            goto exit;
        }
        ret = sfblkdev_delete_disk((sfblkdev_type)type, name, &remote);
        if (ret < 0){
            printf("deleting disk %s failed with error %d\n", name, ret);
            goto exit;
        }
    } else if (cmd == 3) {
        //List all devices from block store service.
        disk = (struct sfblkdisk_info *)
                    malloc(sizeof(struct sfblkdisk_info )*nr_max_disk_info);
        if(disk == NULL)
        {
            goto exit;
        }

        count = sfblkdev_get_disks((sfblkdev_type)type, disk, nr_max_disk_info, &remote);
        printf("Found %d disks\n", count);
        for(i=0; i<count; i++)
        {
            printf("Disk%d, Name %s, Size %lld, Opened %d\n", i, 
                    disk[i].volume_name, disk[i].size, disk[i].open_count);
        }
        free(disk);
    } else if (cmd == 4) {
        //List all devices from driver.
        disk = (struct sfblkdisk_info *)
                    malloc(sizeof(struct sfblkdisk_info )*nr_max_disk_info);
        if(disk == NULL)
        {
            goto exit;
        }

        count = sfblkdev_get_disks_local((sfblkdev_type)type, disk, nr_max_disk_info);
        printf("Found %d disks\n", count);
        for(i=0; i<count; i++)
        {
            printf("Disk%d, Name %s, Size %lld, Opened %d\n", i, 
                    disk[i].volume_name, disk[i].size, disk[i].open_count);
        }
        free(disk);
    } else if (cmd == 5) {
        if (no_of_pages <= 0 || !name) {
            printf("Invalid name or size\n");
            goto exit;
        }
        data = (uint32_t *)malloc(no_of_pages*4096);
        if(data == NULL)
        {
            goto exit;
        }

        ret = sfblkdev_read_data((sfblkdev_type)type, name, data, no_of_pages, offset, &remote);
        if (ret < 0){
            printf("creating disk failed with error error %d\n", ret);
            free(data);
            goto exit;
        }
        for(i=0; i<no_of_pages*1024; i++){
            printf("[%#x] ", data[i]);
            if(i && i%8==0)
                printf("\n");
        }
        free(data);
        //printf("Created disk with name %s\n", name);
    } else if (cmd == 6){
        if (no_of_pages <= 0 || !name) {
            printf("Invalid name or size\n");
            goto exit;
        }
        data = (uint32_t *)malloc(no_of_pages*4096);
        if(data == NULL)
        {
            goto exit;
        }

        for(i=0; i<no_of_pages*1024; i++){
            data[i] = i;
        }
        ret = sfblkdev_write_data((sfblkdev_type)type, name, data, no_of_pages, offset, &remote);
        if (ret < 0){
            printf("creating disk failed with error error %d\n", ret);
            free(data);
            goto exit;
        }
        free(data);
    } else {
        printf("Invalid command!\n");
        show_help();
        goto exit;
    }
    //sfblkdev_lib_cleanup();
    if(name)
        free(name);
noerror:
    close(fd);
    return 0;

exit:
    printf("Exiting with error.\n");
    //sfblkdev_lib_cleanup();
    if(name)
        free(name);
    close(fd);
    return -1;
}
