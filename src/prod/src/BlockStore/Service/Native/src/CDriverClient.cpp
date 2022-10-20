// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
// Interface to interact with the Driver Control Library

#include "stdafx.h"
#if defined(PLATFORM_UNIX)
#include "../dependencies/inc/libpal.h"
#endif
#include "../inc/CDriverClient.h"

using namespace std;
using namespace Common;

bool CDriverClient::Initialize()
{
#if defined(PLATFORM_UNIX)
    FuncIsDriverAvailable pfIsDriverAvailable;
#endif            

    hControlLib_ = ::load_library(CONTROL_LIB);
    if (hControlLib_ != NULL)
    {
#if defined(PLATFORM_UNIX)
        pfIsDriverAvailable = (FuncIsDriverAvailable) ::load_symbol(hControlLib_, EXPORT_IS_DRIVER_AVAILABLE);
        if(!pfIsDriverAvailable || !pfIsDriverAvailable()){
            // Driver/symbol is not available!
            ::unload_library(hControlLib_);
            TRACE_ERROR("Kernel driver is not loaded!");
            return false;
        }
#endif            

        // Successfully loaded the library - now, look for the exports
        pfReadFromUserBuffer_ = (FuncReadFromUserBuffer) ::load_symbol(hControlLib_, EXPORT_READ_FROM_USER_BUFFER);
        pfReadFromSrbBuffer_ = (FuncReadFromSrbBuffer) ::load_symbol(hControlLib_, EXPORT_READ_FROM_SRB_BUFFER);
        pfRefreshServiceLUList_ = (FuncRefreshServiceLUList) ::load_symbol(hControlLib_, EXPORT_REFRESH_SERVICE_LULIST);
        pfShutdownVolumesForService_ = (FuncShutdownVolumesForService) ::load_symbol(hControlLib_, EXPORT_SHUTDOWN_VOLUMES_FOR_SERVICE);

        if (pfReadFromUserBuffer_ && pfReadFromSrbBuffer_ && pfRefreshServiceLUList_ && pfShutdownVolumesForService_)
        {
           fInitialized_ = true;
        }
        else
        {
            // Unable to find one or more exports!
            ::unload_library(hControlLib_);

            hControlLib_ = nullptr;
            pfReadFromUserBuffer_ = nullptr;
            pfReadFromSrbBuffer_ = nullptr;
            pfRefreshServiceLUList_ = nullptr;
            pfShutdownVolumesForService_ = nullptr;

            fInitialized_ = false;
        }
    }

    return fInitialized_;
}

bool CDriverClient::CopyBlockToSRB(void* Srb, void* Buffer, uint32_t Length, uint32_t *DMAError)
{
    return (*pfReadFromUserBuffer_)(Srb, Buffer, Length, DMAError);
}

bool CDriverClient::CopyBlockFromSRB(void* Srb, void* Buffer, uint32_t Length, uint32_t *DMAError)
{
    return (*pfReadFromSrbBuffer_)(Srb, Buffer, Length, DMAError);
}

bool CDriverClient::RefreshServiceLUList(uint32_t servicePort)
{
    return (*pfRefreshServiceLUList_)(servicePort);
}

bool CDriverClient::ShutdownVolumesForService(uint32_t servicePort)
{
    return (*pfShutdownVolumesForService_)(servicePort);
}
