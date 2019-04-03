// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#pragma once

#include "common.h"
#include "trace.h"

// Function pointers for the helpers used by the service for DMA.
typedef bool (*FuncReadFromUserBuffer)(void* Srb, void* Buffer, uint32_t Length, uint32_t* DMAError);
typedef bool (*FuncReadFromSrbBuffer)(void* Srb, void* Buffer, uint32_t Length, uint32_t* DMAError);
// Function pointer for the helper used by the service for shutting down volumes associated with the service listening at the specified port.
typedef bool (*FuncShutdownVolumesForService)(DWORD dwServicePort);

// Function pointer for the helper used by the service to refresh it's LU List.
typedef bool (*FuncRefreshServiceLUList)(uint32_t servicePort);

#if defined(PLATFORM_UNIX)
// Function pointer for the helper used by the service to check if driver is loaded.
typedef bool (*FuncIsDriverAvailable)(void);
#endif

// Name of the Control Library that helps interface with the driver for SRB DMA.
#if defined(PLATFORM_UNIX)
#define CONTROL_LIB "libSFBDCtlLib.so"
#else
#define CONTROL_LIB "SFBDCtlLib.dll"
#endif

// Names of the various exports from the control library
#define EXPORT_READ_FROM_USER_BUFFER "ReadFromUserBuffer"
#define EXPORT_READ_FROM_SRB_BUFFER "ReadFromSRBBuffer"
#define EXPORT_REFRESH_SERVICE_LULIST "RefreshServiceLUList"
#define EXPORT_SHUTDOWN_VOLUMES_FOR_SERVICE "ShutdownVolumesForService"
#if defined(PLATFORM_UNIX)
#define EXPORT_IS_DRIVER_AVAILABLE "IsDriverAvailable"
#endif


class CDriverClient
{
public:
    CDriverClient() : hControlLib_(nullptr), pfReadFromSrbBuffer_(nullptr), pfReadFromUserBuffer_(nullptr), pfRefreshServiceLUList_(nullptr), fInitialized_(false) {}
    bool Initialize();
    bool CopyBlockToSRB(void* Srb, void* Buffer, uint32_t Length, uint32_t *DMAError);
    bool CopyBlockFromSRB(void* Srb, void* Buffer, uint32_t Length, uint32_t *DMAError);
    bool RefreshServiceLUList(uint32_t servicePort);
    bool ShutdownVolumesForService(uint32_t servicePort);
    
private:

    void *hControlLib_;
    FuncReadFromUserBuffer pfReadFromUserBuffer_;
    FuncReadFromSrbBuffer pfReadFromSrbBuffer_;
    FuncRefreshServiceLUList pfRefreshServiceLUList_;
    FuncShutdownVolumesForService pfShutdownVolumesForService_;

    bool fInitialized_;
};
