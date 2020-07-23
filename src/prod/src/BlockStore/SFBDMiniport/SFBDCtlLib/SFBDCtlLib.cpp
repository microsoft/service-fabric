// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "SFBDCtlLib.h"

// This is an example of an exported function.
SFBDCTLLIB_API void DisplayCommandLineHelp()
{
    TRACE_INFO("\nSupported commands: \n\n");

    TRACE_INFO("GetAdapterInfo - Returns details about the Service Fabric Block Device adapter.\n");
    TRACE_INFO("Arguments: none\n\n");
    TRACE_INFO("Example: sfbdctl GetAdapterInfo\n\n");

    TRACE_INFO("ListAllLu - Returns details about all the LUs managed by Service Fabric Block Device.\n");
    TRACE_INFO("Arguments: none\n\n");
    TRACE_INFO("Example: sfbdctl ListAllLu\n\n");

    TRACE_INFO("CreateLu - Create a logical unit at the next available Path/Target/Lun with the specified unique ID and disk size.\n");
    TRACE_INFO("Arguments:\n");
    TRACE_INFO("\t<ID>: an ID that will be used to uniquely identify the LUN across the network.\n");
    TRACE_INFO("\t<DiskSize> <DiskSizeType>: specifies the size of the disk in <DiskSizeType> units. Valid values for <DiskSizeType> are MB/GB/TB\n");
    TRACE_INFO("\t<FileSystem> : specifies the filesystem to format the LU in. Valid values are RAW/FAT/FAT32/NTFS/REFS\n\n");
    TRACE_INFO("\t<MountPoint> : optional argument that specifies the path, with a trailing \\, to mount the volume onto. Mounted folders are only supported on NTFS volumes.\n\n");
    TRACE_INFO("Example: sfbdctl CreateLu myuniquelunid 750 MB NTFS\n\n");
    TRACE_INFO("Example: sfbdctl CreateLu myuniquelunid 750 MB NTFS c:\\mountfolder\\ \n\n");

    TRACE_INFO("GetLuInfo - Gets details about the specified Lu.\n");
    TRACE_INFO("Arguments:\n");
    TRACE_INFO("\t<ID>: ID used in LU creation.\n");
    TRACE_INFO("Example: sfbdctl GetLuInfo myuniquelunid\n\n");

    TRACE_INFO("DeleteLu - Deletes the specified Lu.\n");
    TRACE_INFO("Arguments:\n");
    TRACE_INFO("\t<ID>: ID used in LU creation.\n");
    TRACE_INFO("Example: sfbdctl DeleteLu myuniquelunid\n\n");

    TRACE_INFO("DeleteLu - Deletes the specified Lu.\n");
    TRACE_INFO("Arguments:\n");
    TRACE_INFO("\t<ID>: ID used in LU creation.\n");
    TRACE_INFO("Example: sfbdctl DeleteLu myuniquelunid\n\n");

    TRACE_INFO("UnmountLu - Marks the specified Lu as unmounted so that it is considered offline.\n");
    TRACE_INFO("Arguments:\n");
    TRACE_INFO("\t<ID>: ID used in LU creation.\n");
    TRACE_INFO("Example: sfbdctl UnmountLu myuniquelunid\n\n");

    TRACE_INFO("MountLu - Marks the specified Lu as mounted so that it is considered online.\n");
    TRACE_INFO("Arguments:\n");
    TRACE_INFO("\t<ID>: ID used in LU creation.\n");
    TRACE_INFO("Example: sfbdctl MountLu myuniquelunid\n\n");

    TRACE_INFO("InstallDevice - Installs the Service Fabric Block Device Driver.\n");
    TRACE_INFO("Arguments:\n");
    TRACE_INFO("\t<INFPath>: Absolute path to the INF file for driver installation.\n");
    TRACE_INFO("Example: sfbdctl installdevice c:\\sfbd\\sfbdminiport.inf\n\n");

    TRACE_INFO("UninstallDevice - Uninstalls the Service Fabric Block Device Driver.\n");
    TRACE_INFO("Arguments:\n");
    TRACE_INFO("\t<INFPath>: Absolute path to the INF file for driver installation.\n");
    TRACE_INFO("Example: sfbdctl uninstalldevice c:\\sfbd\\sfbdminiport.inf\n\n");
}