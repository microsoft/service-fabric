// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if KTL_USER_MODE
#error KtlLogger Driver must be build with KTL_USER_MODE=0
#endif

#include <ktl.h>
#include <ktrace.h>
#include <ktlevents.km.h>

#include <ntddk.h>
#include <wdf.h>

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <wdmsec.h> // for SDDLs
#include "public.h" // contains IOCTL definitions

#include "KtlLogShimKernel.h"

#define KTLLOG_TAG 'LLTK'

//
// Define function prototypes as C for those functions that are called
// directly from the framework.
//
extern "C"
{
//
// Device driver routine declarations.
//

DRIVER_INITIALIZE DriverEntry;

NTSTATUS
KtlLogEvtDeviceAdd(
    IN WDFDRIVER Driver,
    IN PWDFDEVICE_INIT DeviceInit,
    IN BOOLEAN EstablishSecurity,
    OUT WDFDEVICE* ControlDevice                   
    );

EVT_WDF_DRIVER_UNLOAD KtlLogEvtDriverUnload;

EVT_WDF_OBJECT_CONTEXT_CLEANUP KtlLogEvtDriverContextCleanup;
EVT_WDF_DEVICE_SHUTDOWN_NOTIFICATION KtlLogShutdown;

EVT_WDF_DEVICE_CONTEXT_CLEANUP KtlLogEvtDeviceContextCleanup;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL KtlLogIoDeviceControl;

EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE KtlLogEvtRequestCancelOnQueue;
EVT_WDF_REQUEST_CANCEL KtlLogEvtRequestCancel;

EVT_WDF_IO_IN_CALLER_CONTEXT KtlLogEvtDeviceIoInCallerContext;
EVT_WDF_DEVICE_FILE_CREATE KtlLogEvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE KtlLogEvtFileContextClose;
EVT_WDF_FILE_CLEANUP KtlLogEvtFileContextCleanup;

EVT_WDF_OBJECT_CONTEXT_CLEANUP KtlLogEvtRequestContextCleanup;

EVT_WDF_DEVICE_D0_EXIT KtlLogEvtDeviceD0Exit;

//
// Device object extention - the single device object will have this
// associated with it
//
typedef struct _CONTROL_DEVICE_EXTENSION
{
    KAllocator* KtlAllocator;
} CONTROL_DEVICE_EXTENSION, *PCONTROL_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONTROL_DEVICE_EXTENSION,
                                   ControlGetData)

//
// Following file context is used for tracking per handle information.
// Note that these contexts are NOT part of KTL and thus any resources
// including smart pointers must be manually cleaned up by the driver.
//
typedef struct _FILE_CONTEXT {

    FileObjectTable::SPtr FOT;
} FILE_CONTEXT, *PFILE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILE_CONTEXT, GetFileContext)

//
// Implementation within WDF device driver
//
class DevIoControlKm : public DevIoControlKernel
{
    K_FORCE_SHARED(DevIoControlKm);
    
    DevIoControlKm(
        __in ULONG AllocationTag
        );

    public:
        static NTSTATUS Create(
            __in WDFREQUEST Request,
            __out DevIoControlKernel::SPtr& Result,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        VOID CompleteRequest(
            __in NTSTATUS Status,
            __in ULONG OutBufferSize
            ) override ;

        ULONG GetInBufferSize() override 
        {
            return(_InBufferSize);
        }
        
        ULONG GetOutBufferSize() override 
        {
            return(_OutBufferSize);
        }
        
        PUCHAR GetBuffer() override 
        {
            return(_Buffer);
        }

        VOID Initialize(
            __in PUCHAR Buffer,
            __in ULONG InBufferSize,
            __in ULONG OutBufferSize
        ) override
        {
            UNREFERENCED_PARAMETER(Buffer);
            UNREFERENCED_PARAMETER(InBufferSize);
            UNREFERENCED_PARAMETER(OutBufferSize);
            
            //
            // Not used in kernel mode driver
            //
            KAssert(FALSE);
        }

        VOID
        SyncRequestCompletion(
            );

        VOID
        SyncRequestCancellation(
            __in RequestMarshallerKernel::SPtr Marshaller
            );      
        
    private:
        ULONG _AllocationTag;

        WDFREQUEST _Request;
        PUCHAR _Buffer;
        ULONG _InBufferSize;
        ULONG _OutBufferSize;

        //
        // These fields are used for synchronizing completion of the
        // request. Request is started while in the InIo callback but
        // cannot be completed until it has been pumped through the
        // WDF queue. These fields are used to force this synchronization.
        //
        // _CompletionLock is the spinlock that keeps these sane
        // _CancelCallbackShouldComplete is set if a race between the WDF
        //     cancel callback and the completion is detected and thus
        //     the completion defers to the WDF cancel callback
        // _HasCancelCallbackInvoked is TRUE when WDF invokes the
        //     request's cancel callback
        // _IsPassedQueue is TRUE when the request has been passed
        //     through the queue. If the request finishes after this is
        //     set to TRUE then the it can complete the request on its
        //     own.
        // _IsFinishedProcess is TRUE when the request has finished
        //     processing. 
        //     Once the request is pumped through the queue it will be
        //     completed.
        // _CompletionStatus has the completion status in the case that
        //     the request is completed when it is pumped out of the
        //     queue.
        // _CompletionSize has the completion size in the case that the
        //     the request is completed when it is pumped out of the
        //     queue.
        //
        KSpinLock _CompletionLock;
        BOOLEAN _CancelCallbackShouldComplete;
        BOOLEAN _HasCancelCallbackInvoked;
        BOOLEAN _IsPassedQueue;
        BOOLEAN _IsFinishedProcessing;
        NTSTATUS _CompletionStatus;
        ULONG _CompletionSize;      
};

//
// Following request context is associated with each request
//
typedef struct _REQUEST_CONTEXT {

    RequestMarshallerKernel::SPtr Marshaller;
    DevIoControlKm::SPtr DevIoCtl;
    
    WDFMEMORY InputMemoryBuffer;
    WDFMEMORY OutputMemoryBuffer;
    
} REQUEST_CONTEXT, *PREQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT, GetRequestContext)

#pragma warning(disable:4127)


NTSTATUS KtlLogInitializeKtl();

VOID KtlLogDeinitializeKtl();

NTSTATUS KtlLogEstablishSecurity(
    __in WDFDEVICE ControlDevice
    );

NTSTATUS
NTAPI
KtlLogSiloCreateNotify(
    __in PESILO ServerSilo
    );

VOID
NTAPI
KtlLogSiloTerminateNotify(
    __in PESILO ServerSilo
    );

NTSTATUS KtlLogCreateDevice(
    IN WDFDRIVER Driver,
    OUT WDFDEVICE* ControlDevice
    );

NTSTATUS KtlLogDeleteDevice(
    IN WDFDEVICE ControlDevice
    );

}
