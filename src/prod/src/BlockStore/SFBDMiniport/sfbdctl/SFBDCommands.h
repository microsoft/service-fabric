// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if !defined(DRIVER_BUILD)

#define EXTERN_C extern "C"
#define SFBDCTLLIB_API EXTERN_C __declspec(dllexport)

#define SFBD_SYMBOLIC_PATH  L"\\\\?\\root#servicefabricblockdevice#0000#{1fa5301a-7fb3-4a73-9b54-741f24e35f02}"

// Name of the device class - please keep this in sync with SFBDMiniport.inf's definition of the same.
#define SFBD_CLASS_NAME     L"ServiceFabricBlockDevice"

#endif // !defined(DRIVER_BUILD)

static const GUID GUID_ServiceFabricBlockDeviceInterface = // {1FA5301A-7FB3-4A73-9B54-741F24E35F02}
{ 0x1fa5301a, 0x7fb3, 0x4a73,{ 0x9b, 0x54, 0x74, 0x1f, 0x24, 0xe3, 0x5f, 0x2 } };

typedef enum
{
    // Used by the controller application
    GetAdapterInfo,
    CreateLU,
    GetInfoForLu,
    DeleteLu,
    UnmountLu,
    MountLu,
    GetLuCount,
    GetInfoForLuIndex,
    ListAllLu,
    ProvisionLu,

    // Used by the service
    ReadFromUsermodeBuffer,
    ReadFromSrbDataBuffer,
    RefreshLUList,
    ShutdownServiceVolumes,

    // Used by the controller application
    InstallDevice,
    UninstallDevice
} SFBDCommandCode;

struct SFBDCommands
{
    SFBDCommandCode commandCode;
    PWCHAR szCommand;
    int  commandArgs;
};