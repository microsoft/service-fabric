// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*++

Module Name:

Common.h

Abstract:

This file includes data declarations for Service Fabric BlockStore Device's interaction with the BlockStore Service.

Environment:

Kernel mode only.

--*/

#pragma once
#ifndef _COMMON_H_
#define _COMMON_H_

typedef struct _SFBD_DEVICE_INFO {
    UCHAR               DeviceType;
    UCHAR               PathId;
    UCHAR               TargetId;
    UCHAR               Lun;

    // Is the DeviceInfo valid?
    LONG                Enabled;
    ULONG64             DiskSize;
    DiskSizeUnit        SizeUnit;
    ULONG               DiskSizeInSizeUnit;
    WCHAR               LUID[MAX_WCHAR_SIZE_LUID];
    PVOID               pLUExt;

    // Is the LU mounted?
    BOOLEAN             Mounted;

    // Service port associated with this LU
    ULONG               ServicePort;
} SFBD_DEVICE_INFO, *pSFBD_DEVICE_INFO;

typedef struct _SFBD_DEVICE_LIST {
    ULONG          DeviceCount;
    SFBD_DEVICE_INFO DeviceInfo[1];
} SFBD_DEVICE_LIST, *pSFBD_DEVICE_LIST;

#endif    // _COMMON_H_
