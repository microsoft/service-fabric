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
#include <iostream>
#include "PAL.h"
#include <SFBDCtlLib.h>
#include <SFBDCtlLibTrace.h>
#include "../../../../Common/Common.h"

#include <iostream>
#include <cmath>

#define SFBLKDEV_CTL_DEV_PATH   "/dev/sfblkdevctrl"
#define LOOPBACK_ADDRESS "127.0.0.1"
static int g_fd = -1;

static void __attribute__ ((constructor)) sfblkdev_lib_init(void);
static void __attribute__ ((destructor)) sfblkdev_lib_cleanup(void);

int open_device()
{
    int fd;
    fd = open(SFBLKDEV_CTL_DEV_PATH, O_RDWR);
    if(fd == -1)
    {
        TRACE_ERROR("Device file " << SFBLKDEV_CTL_DEV_PATH << " open failed with error " << errno);
        return -errno;
    }
    return fd;
}

int close_device(int fd)
{
    close(fd);
    return 0;
}

bool ReadFromUserBuffer (void *Srb, void *Buffer, uint32_t Length, 
                                uint32_t *DmaError)
{
    return false;
}

bool ReadFromSRBBuffer (void *Srb, void *Buffer, uint32_t Length, 
                                uint32_t *DmaError)
{
    return false;
}

HANDLE ConnectToDevice(PDWORD pLastError)
{
    int fd;

    fd = open_device();
    if (fd < 0){
        *pLastError = fd;
        return NULL;
    }

    return (HANDLE)fd;
}

bool CloseLinuxHandle(HANDLE driver)
{
    close_device((int)driver);
    return true;
}

bool IsDriverAvailable(void)
{
    if(g_fd < 0)
        return false;
    return true;
}

bool already_mounted(const char *mount_point)
{
    char fs_cmd[250];
    bool return_value = true;

    char *mount_point_copy = strdup(mount_point);
    if (mount_point_copy == NULL) return false;

    mount_point_copy[strlen(mount_point_copy)-1] = '\0';

    // mount point will not be null here.
    sprintf(fs_cmd, "grep -q %s /proc/mounts", mount_point_copy);
    int systemReturnValue = system(fs_cmd);
    if(WIFEXITED(systemReturnValue)) {
        if(WEXITSTATUS(systemReturnValue)) {
            return_value = false;
        }
    } else {
        // abnormal termination
        return_value = false;
    }

    free(mount_point_copy);
    return return_value;
}


bool ProvisionLUN(HANDLE hDevice,
                  PWCHAR pLUID,
                  PWCHAR pSize,
                  PWCHAR pSizeType, 
                  LPWSTR pFileSystem,
                  PWCHAR pMountPoint,
                  DWORD dwServicePort)
{
    struct sfblkstore_svc remote;
    size_t size;
    char *fs_type;
    char fs_cmd[64];
    char device_file[64] = "/dev/";
    char drv_file[32];
    int mnt_point_len = wcslen(pMountPoint) + 1;
    int ret;
    int volume_return_code = 0, umount_return_code = 0;

    //setup remote.
    inet_pton(AF_INET, LOOPBACK_ADDRESS, &remote.ip);
    remote.port = dwServicePort;

    //convert size in bytes.
    size = (size_t)wcstod(pSize, nullptr);
    if(wcsncmp(pSizeType, L"MB", 2) == 0) {
        size = size<<20;
    }else if(wcsncmp(pSizeType, L"GB", 2) == 0) {
        size = size<<30;
    }else if(wcsncmp(pSizeType, L"TB", 2) == 0) {
        size = size<<40;
    }else {
        return false;
    }

    //check device length.
    if(wcsnlen(pLUID, SFBLKDEV_MAX_VOLUME_NAME_LEN) == SFBLKDEV_MAX_VOLUME_NAME_LEN)
        return false;

    //Validate filesystem type.
    if (wcscasecmp(pFileSystem, L"RAW") == 0)
    {
        // Nothing to do.
        fs_type = NULL;
    }
    else if (wcscasecmp(pFileSystem, L"FAT") == 0)
    {
        fs_type = "fat";
    }
    else if (wcscasecmp(pFileSystem, L"NTFS") == 0)
    {
        fs_type = "ntfs";
    }
    else if (wcscasecmp(pFileSystem, L"ext3") == 0)
    {
        fs_type = "ext3";
    }
    else if (wcscasecmp(pFileSystem, L"ext4") == 0)
    {
        fs_type = "ext4";
    }
    else
    {
        TRACE_ERROR("Invalid Filesystem type specified:" << pFileSystem);
        return false;
    }

    wstring wLuid(pLUID);
    string luid;
    Common::StringUtility::UnicodeToAnsi(wLuid, luid);
    if(luid.empty())
    {
        TRACE_ERROR("SFBDCtlLib: LUID conversion failed");
        return false;
    }

    //mount volume.
    //convert widechar mountpoint to ascii.
    wstring wMountPoint(pMountPoint);
    string mountPoint;
    Common::StringUtility::UnicodeToAnsi(wMountPoint, mountPoint);
    if(mountPoint.empty())
    {
        TRACE_ERROR("SFBDCtlLib: pMountPoint conversion failed");
        return false;
    }

    TRACE_INFO("SFBDCtlLib: volume:" << luid
              << " size:" << size
              << " mount_point:" << mountPoint);

    if(already_mounted(mountPoint.c_str()))
    {
        TRACE_INFO("SFBDCtlLib: mountpoint already mounted:" << mountPoint);
        return true;
    }

    //create volume.
    volume_return_code = sfblkdev_create_disk(NW_SFBLKDEV_TYPE, luid.c_str(), size,
                        &remote, drv_file);
    if(volume_return_code != 0 && volume_return_code != -EEXIST) {
        TRACE_ERROR("SFBDCtlLib: driver couldn't create disk.");
        return false;
    }

    TRACE_INFO("SFBDCtlLib: create disk successful device:" << drv_file);

    strcat(device_file, drv_file);

#if 0
    //Unmount device.
    TRACE_INFO("SFBDCtlLib: unmounting device file " << device_file);
    umount_return_code = umount(device_file);
    if(umount_return_code != 0)
    {
        TRACE_WARNING("SFBDCtlLib: umount failed ret:" << umount_return_code << " err:" << errno);
    }
#endif

    if(fs_type) {
        // Do not reformat existing drive
        if(volume_return_code != -EEXIST)
        {
            //create filesystem.
            sprintf(fs_cmd, "mkfs.%s %s", fs_type, device_file);
            TRACE_INFO("SFBDCtlLib: Creating filesystem:" << fs_cmd);
    
            int systemReturnValue = system(fs_cmd);
            if(WIFEXITED(systemReturnValue)) {  // normal exit
                if(WEXITSTATUS(systemReturnValue)) {
                    // normally indicates an error
                    TRACE_ERROR("SFBDCtlLib: mkfs failed:" << WEXITSTATUS(systemReturnValue));
                    sfblkdev_delete_disk(NW_SFBLKDEV_TYPE, luid.c_str(), &remote);
                    return false;
                }
            } else {
                // abnormal termination, e.g. process terminated by signal
                TRACE_ERROR("SFBDCtlLib: mkfs process terminated failed:" << WIFEXITED(systemReturnValue));
                sfblkdev_delete_disk(NW_SFBLKDEV_TYPE, luid.c_str(), &remote);
                return false;
            }
            TRACE_INFO("SFBDCtlLib: mkfs succeeded " << systemReturnValue);
        }

        TRACE_INFO("SFBDCtlLib: Mount Point:" << mountPoint
               << " device_file:" << device_file
               << " fs_type:" << fs_type);

        //mount volume.
        ret = mount(device_file, mountPoint.c_str(), fs_type, 0, nullptr);
        if(ret){
            //mount failed!
            TRACE_ERROR("disk mount failed with error " << errno);
            if(volume_return_code != -EEXIST)
            {
                TRACE_WARNING("deleting volume " << luid.c_str() << " device file "<< device_file);
                sfblkdev_delete_disk(NW_SFBLKDEV_TYPE, luid.c_str(), &remote);
            }
            return false; 
        }
        TRACE_INFO("SFBDCtlLib: mount completed with ret " << ret);
    }
    return true;
}

bool ShutdownVolumesForService(DWORD dwServicePort)
{
    struct shutdown_disk_param shutdown_disk;
    int ret;
    bool result = false;
   
    inet_pton(AF_INET, LOOPBACK_ADDRESS, &shutdown_disk.remote.ip);
    shutdown_disk.remote.port = dwServicePort;
    shutdown_disk.type = NW_SFBLKDEV_TYPE; 
    
    ret = ioctl(g_fd, SFBLKDEV_SHUTDOWN_DISK, &shutdown_disk);
    if (ret) {
        TRACE_ERROR("ioctl shutdown_disk failed with error:" << errno);
        goto exit;
    }
    result = true;

exit:
    return result;
}

bool RefreshServiceLUList(uint32_t servicePort)
{
    struct refresh_disk_param refresh_disk;
    int ret;
    bool result = false;

    inet_pton(AF_INET, LOOPBACK_ADDRESS, &refresh_disk.remote.ip);
    refresh_disk.remote.port = servicePort;
    refresh_disk.type = NW_SFBLKDEV_TYPE;
    
    ret = ioctl(g_fd, SFBLKDEV_REFRESH_DISK, &refresh_disk);
    if (ret) {
        TRACE_ERROR("ioctl refresh_disk failed with error:" << errno);
        goto exit;
    }
    result = true;

exit:
    return result;
}

int sfblkdev_create_disk(sfblkdev_type type,
                         const char *volume_name, 
                         size_t size,
                         struct sfblkstore_svc *remote,
                         char *device_file)
{
    struct create_disk_param create_disk;
    int ret = -EINVAL;
    int len;

    if (!volume_name || !remote) {
        TRACE_ERROR("Invalid device id or remote ip");
        goto exit;
    }

    TRACE_INFO("SFBDCtlLib: Creating disk:" << volume_name << ", fd:" << g_fd << ", size:" << size);

    memset(&create_disk, 0, sizeof(create_disk));

    len = strnlen(volume_name, SFBLKDEV_MAX_VOLUME_NAME_LEN);
    if(len  >= SFBLKDEV_MAX_VOLUME_NAME_LEN)
        goto exit;
    strcpy(create_disk.volume_name, volume_name);

    create_disk.size = size;
    //update remote block store service.
    create_disk.remote = *remote;
    create_disk.type = type;

    ret = ioctl(g_fd, SFBLKDEV_CREATE_DISK, &create_disk);
    if (ret) {
        // Error
        ret = -errno;
        if(ret == -EEXIST) {
            TRACE_INFO("SFBDCtlLib: Volume already exists.");   
        }else{
            TRACE_ERROR("SFBDCtlLib: Volume creation failed with error" << ret);
            goto exit;
        }
    }
    strcpy(device_file, create_disk.device_name);
    TRACE_INFO("SFBDCtlLib: Volume:" << volume_name << " DeviceFile:" << create_disk.device_name);

exit:
    return ret;
}

int sfblkdev_delete_disk(sfblkdev_type type, const char *volume_name, struct sfblkstore_svc *remote)
{
    struct delete_disk_param delete_disk;
    int ret;
    int len;

    ret = -ENODEV;
    if(g_fd <= 0)
        goto exit;

    //Delete Device
    ret = -EINVAL;
    if (!volume_name) {
        TRACE_ERROR("Invalid device id");
        goto exit;
    }

    len = strnlen(volume_name, SFBLKDEV_MAX_VOLUME_NAME_LEN);
    if(len  >= SFBLKDEV_MAX_VOLUME_NAME_LEN)
        goto exit;

    strcpy(delete_disk.volume_name, volume_name);
    delete_disk.type = type;
    delete_disk.remote = *remote;

    //call ioctl.
    ret = ioctl(g_fd, SFBLKDEV_DELETE_DISK, &delete_disk);
    if (ret) {
        TRACE_ERROR("SFBLKDEV_DELETE_DISK ret:" << ret << " errno:" << errno);
        ret = -errno;
        goto exit;
    }
    return 0;

exit:
    return ret;
}

int sfblkdev_get_disks_local(sfblkdev_type type, struct sfblkdisk_info *user_disk_info, int count)
{
    struct get_disk_param get_disk;
    struct sfblkdisk_info *disk;
    int i;
    size_t size = 0;
    int index = -1;
    int cmd, ret;

    ret = -ENODEV;
    if(g_fd <= 0)
        goto exit;
    
    disk = (struct sfblkdisk_info *) 
                malloc(sizeof(struct sfblkdisk_info) * count);
    get_disk.disks = disk;
    get_disk.count = count;
    get_disk.type = type;
    //update remote block store service.
    ret = ioctl(g_fd, SFBLKDEV_GET_DISK_LOCAL, &get_disk);
    if (ret) {
        TRACE_ERROR("SFBLKDEV_GET_DISK_LOCAL ret:" << ret << " errno:" << errno);
        ret = -errno;
        goto exit;
    }

    for(i=0; i<get_disk.valid_count; i++)
    {
        memcpy(&user_disk_info[i], &disk[i], sizeof(struct sfblkdisk_info));
    }
    free(disk);

    //return count of copied disks.
    return get_disk.valid_count;

exit:
    return ret;
}

int sfblkdev_get_disks(sfblkdev_type type, struct sfblkdisk_info *user_disk_info, int count, 
                            struct sfblkstore_svc *remote)
{
    struct get_disk_param get_disk;
    int i;
    size_t size = 0;
    int index = -1;
    int cmd, ret;
    int available_disk;

    ret = -ENODEV;
    if(g_fd <= 0)
        goto exit;
    
    ret = -EINVAL;
    if(!remote)
        goto exit;

    get_disk.disks = user_disk_info;
    //update remote block store service.
    get_disk.remote = *remote;
    get_disk.count = count;
    get_disk.type = type;
    ret = ioctl(g_fd, SFBLKDEV_GET_DISK, &get_disk);
    if (ret) {
        TRACE_ERROR("SFBLKDEV_GET_DISK ret:" << ret << " errno:" << errno);
        ret = -errno;
        goto exit;
    }
    return get_disk.valid_count;

exit:
    return ret;
}

int sfblkdev_read_data(sfblkdev_type type, const char *volume_name, void *data, int pages, uint64_t offset,
                            struct sfblkstore_svc *remote)
{
    struct rw_disk_param rw_data;
    int ret;
    int len;

    ret = -ENODEV;
    if(g_fd <= 0)
        goto exit;

    ret = -EINVAL;
    if (!volume_name || !remote) {
        TRACE_ERROR("Invalid device id or remote ip");
        goto exit;
    }

    len = strnlen(volume_name, SFBLKDEV_MAX_VOLUME_NAME_LEN);
    if(len  >= SFBLKDEV_MAX_VOLUME_NAME_LEN)
        goto exit;
    strcpy(rw_data.volume_name, volume_name);

    rw_data.type = type;
    rw_data.no_of_pages = pages;
    rw_data.pos = offset;
    //update remote block store service.
    rw_data.remote = *remote;
    rw_data.data = data;

    ret = ioctl(g_fd, SFBLKDEV_READ_DATA, &rw_data);
    if (ret) {
        TRACE_ERROR("SFBLKDEV_READ_DATA ret " << ret << " errno" << errno);
        ret = -errno;
        goto exit;
    }
    TRACE_INFO("Read successful");
    return 0;

exit:
    return ret;
}

int sfblkdev_write_data(sfblkdev_type type, const char *volume_name, void *data, int pages, uint64_t offset,
                            struct sfblkstore_svc *remote)
{
    struct rw_disk_param rw_data;
    int ret;
    int len;

    ret = -ENODEV;
    if(g_fd <= 0)
        goto exit;

    ret = -EINVAL;
    if (!volume_name || !remote) {
        TRACE_ERROR("Invalid device id or remote ip");
        goto exit;
    }

    len = strnlen(volume_name, SFBLKDEV_MAX_VOLUME_NAME_LEN);
    if(len  >= SFBLKDEV_MAX_VOLUME_NAME_LEN)
        goto exit;
    strcpy(rw_data.volume_name, volume_name);

    rw_data.type = type;
    rw_data.no_of_pages = pages;
    rw_data.pos = offset;
    //update remote block store service.
    rw_data.remote = *remote;
    rw_data.data = data;

    ret = ioctl(g_fd, SFBLKDEV_WRITE_DATA, &rw_data);
    if (ret) {
        TRACE_ERROR("SFBLKDEV_WRITE_DATA ret:" << ret << " errno:" << errno);
        ret = -errno;
        goto exit;
    }
    TRACE_INFO("Write successful");
    return 0;

exit:
    return ret;
}

static void sfblkdev_lib_init(void)
{
    //Open device.
    g_fd = open_device();
    return;
} 

static void sfblkdev_lib_cleanup(void)
{
    if(g_fd < 0)
        return;
    close_device(g_fd);
}
