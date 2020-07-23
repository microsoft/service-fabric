// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef __BLKSTORE_API_H
#define __BLKSTORE_API_H
#include "sfblkdev_common.h"

#define BLKSRV_IP   "10.171.44.65"
#define BLKSRV_PORT 40000

#ifdef __cplusplus
extern "C" {
#endif

bool ReadFromUserBuffer (void *Srb, void *Buffer, uint32_t Length, uint32_t *DmaError);
bool ReadFromSRBBuffer (void *Srb, void *Buffer, uint32_t Length, uint32_t *DmaError);
HANDLE ConnectToDevice(PDWORD pLastError);
bool CloseLinuxHandle(HANDLE driver);
bool ProvisionLUN(HANDLE Unused, PWCHAR pLUID, PWCHAR pSize, PWCHAR pSizeType, 
                    LPWSTR pFileSystem, PWCHAR pMountPoint, DWORD dwServicePort);
bool ShutdownVolumesForService(DWORD dwServicePort);
bool RefreshServiceLUList(uint32_t servicePort);
bool IsDriverAvailable(void);

int sfblkdev_create_disk(sfblkdev_type type, const char *disk_id,
                            size_t size, struct sfblkstore_svc *remote, char *device_file);
int sfblkdev_delete_disk(sfblkdev_type type, const char *disk_id, struct sfblkstore_svc *remote);
int sfblkdev_get_disks(sfblkdev_type type, struct sfblkdisk_info *user_disk_info, int count, 
                            struct sfblkstore_svc *remote);
int sfblkdev_get_disks_local(sfblkdev_type type, struct sfblkdisk_info *user_disk_info, int count);
int sfblkdev_read_data(sfblkdev_type type, const char *disk_id, void *data, int pages, uint64_t offset,
                            struct sfblkstore_svc *remote);
int sfblkdev_write_data(sfblkdev_type type, const char *disk_id, void *data, int pages, uint64_t offset,
                            struct sfblkstore_svc *remote);
#ifdef __cplusplus
}
#endif

#endif
