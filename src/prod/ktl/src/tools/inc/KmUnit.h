/*++

Copyright (c) Microsoft Corporation

Module Name:

    KmUnit.h

Abstract:

    Contains function prototypes and includes other neccessary header files.

Environment:

    Kernel mode only.

--*/

#pragma once

#include <ntosp.h>
#include <wdf.h>

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <wdmsec.h>

#include <KmUnitDefs.h>
#include "KmCommon.h"
#include "Trace.h"                      


extern "C" {

//
// Device driver routine declarations.
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_UNLOAD                   KuEvtDriverUnload;
EVT_WDF_OBJECT_CONTEXT_CLEANUP          KuEvtDriverContextCleanup;
EVT_WDF_DEVICE_SHUTDOWN_NOTIFICATION    KuShutdown;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL      KuEvtIoDeviceControl;

}

//
// Don't use EVT_WDF_DRIVER_DEVICE_ADD for KuDeviceAdd even though 
// the signature is same because this is not an event called by the 
// framework.
//

NTSTATUS
KuDeviceAdd(
    __in WDFDRIVER          Driver,
    __in PWDFDEVICE_INIT    DeviceInit
    );
