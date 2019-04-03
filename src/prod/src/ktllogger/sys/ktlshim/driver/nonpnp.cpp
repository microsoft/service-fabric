// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "logger.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry )
#pragma alloc_text( PAGE, KtlLogEvtDeviceAdd)
#pragma alloc_text( PAGE, KtlLogEvtDriverUnload)
#pragma alloc_text( PAGE, KtlLogEvtDriverContextCleanup)
#pragma alloc_text( PAGE, KtlLogEvtDeviceContextCleanup)
#pragma alloc_text( PAGE, KtlLogEvtDeviceIoInCallerContext)
#pragma alloc_text( PAGE, KtlLogEvtDeviceFileCreate)
#pragma alloc_text( PAGE, KtlLogIoDeviceControl)
#pragma alloc_text( PAGE, KtlLogSiloCreateNotify)
#pragma alloc_text( PAGE, KtlLogSiloTerminateNotify)
#endif // ALLOC_PRAGMA



BOOLEAN KtlLogKtlInitialized;
BOOLEAN OverlayManagerRegistered;
BOOLEAN SiloCallbacksRegistered;

//
// Define globals and function prototypes needed to call the container
// apis that allow driver to be container aware. Note that container
// apis are only available in RS1 or later
//
WDFDRIVER DriverHandle;
PSILO_MONITOR KtlLogSiloMonitor;

typedef NTKERNELAPI
_Must_inspect_result_
NTSTATUS
(*FUNCPsRegisterSiloMonitor)(
    _In_ PSILO_MONITOR_REGISTRATION Registration,
    _Outptr_ PSILO_MONITOR *ReturnedMonitor
    );
FUNCPsRegisterSiloMonitor FuncPsRegisterSiloMonitor;

typedef NTKERNELAPI
VOID
(*FUNCPsUnregisterSiloMonitor)(
    _In_ _Post_invalid_ PSILO_MONITOR Monitor
    );
FUNCPsUnregisterSiloMonitor FuncPsUnregisterSiloMonitor;

typedef NTKERNELAPI
_Must_inspect_result_
NTSTATUS
(*FUNCPsStartSiloMonitor)(
    _In_ PSILO_MONITOR Monitor
    );
FUNCPsStartSiloMonitor FuncPsStartSiloMonitor;

typedef NTKERNELAPI
PESILO
(*FUNCPsAttachSiloToCurrentThread)(
    _In_ PESILO Silo
    );
FUNCPsAttachSiloToCurrentThread FuncPsAttachSiloToCurrentThread;

typedef NTKERNELAPI
VOID
(*FUNCPsDetachSiloFromCurrentThread)(
    _In_ PESILO PreviousSilo
    );
FUNCPsDetachSiloFromCurrentThread FuncPsDetachSiloFromCurrentThread;

typedef NTKERNELAPI
_Check_return_
NTSTATUS
(*FUNCPsInsertSiloContext)(
    _In_ PESILO Silo,
    _In_ ULONG ContextSlot,
    _In_ PVOID SiloContext
   );
FUNCPsInsertSiloContext FuncPsInsertSiloContext;

typedef NTKERNELAPI
ULONG
(*FUNCPsGetSiloMonitorContextSlot)(
    _In_ PSILO_MONITOR Monitor
    );
FUNCPsGetSiloMonitorContextSlot FuncPsGetSiloMonitorContextSlot;

typedef
NTKERNELAPI
/* _Check_return_ */
NTSTATUS
(*FUNCPsGetSiloContext)(
    _In_ PESILO Silo,
    _In_ ULONG ContextSlot,
    _Outptr_result_nullonfailure_ PVOID *ReturnedSiloContext
    );
FUNCPsGetSiloContext FuncPsGetSiloContext;

typedef NTKERNELAPI
_Must_inspect_result_
NTSTATUS
(*FUNCPsCreateSiloContext)(
    _In_ PESILO Silo,
    _In_ ULONG Size,
    _In_ POOL_TYPE PoolType,
    _In_opt_ SILO_CONTEXT_CLEANUP_CALLBACK ContextCleanupCallback,
    _Outptr_result_bytebuffer_(Size) PVOID *ReturnedSiloContext
    );
FUNCPsCreateSiloContext FuncPsCreateSiloContext;

typedef NTKERNELAPI
VOID
(*FUNCPsDereferenceSiloContext)(
    _In_ PVOID SiloContext
    );
FUNCPsDereferenceSiloContext FuncPsDereferenceSiloContext;

typedef NTKERNELAPI
NTSTATUS
(*FUNCPsRemoveSiloContext)(
    _In_ PVOID SiloContext,
    _In_ ULONG ContextSlot,
    _Outptr_result_nullonfailure_ PVOID *ReturnedSiloContext
    );
FUNCPsRemoveSiloContext FuncPsRemoveSiloContext;


NTSTATUS
DriverEntry(
    IN OUT PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING      RegistryPath
    )
/*++

Routine Description:
    This routine is called by the Operating System to initialize the driver.

    It creates the device object, fills in the dispatch entry points and
    completes the initialization.

Arguments:
    DriverObject - a pointer to the object that represents this device
    driver.

    RegistryPath - a pointer to our Services key in the registry.

Return Value:
    STATUS_SUCCESS if initialized; an error otherwise.

--*/
{
    NTSTATUS                       status;
    WDF_DRIVER_CONFIG              config;
    WDFDRIVER                      hDriver;
    WDF_OBJECT_ATTRIBUTES          attributes;
    PWDFDEVICE_INIT                pInit = NULL;

    KtlLogKtlInitialized = FALSE;
    OverlayManagerRegistered = FALSE;
    SiloCallbacksRegistered = FALSE;

    //
    // Find addresses for silo apis. We need to do this since the silo
    // apis are in windows 10/RS1 and this driver needs to work
    // downlevel to Windows 7
    //
    

    UNICODE_STRING textPsRegisterSiloMonitor;
    RtlInitUnicodeString(&textPsRegisterSiloMonitor, L"PsRegisterSiloMonitor");
    FuncPsRegisterSiloMonitor = (FUNCPsRegisterSiloMonitor)MmGetSystemRoutineAddress(&textPsRegisterSiloMonitor);
    if (FuncPsRegisterSiloMonitor != NULL)
    {

        UNICODE_STRING textPsUnregisterSiloMonitor;
        RtlInitUnicodeString(&textPsUnregisterSiloMonitor, L"PsUnregisterSiloMonitor");
        FuncPsUnregisterSiloMonitor = (FUNCPsUnregisterSiloMonitor)MmGetSystemRoutineAddress(&textPsUnregisterSiloMonitor);
        if (FuncPsUnregisterSiloMonitor == NULL)
        {
            return(STATUS_REVISION_MISMATCH);
        }

        UNICODE_STRING textPsStartSiloMonitor;
        RtlInitUnicodeString(&textPsStartSiloMonitor, L"PsStartSiloMonitor");
        FuncPsStartSiloMonitor = (FUNCPsStartSiloMonitor)MmGetSystemRoutineAddress(&textPsStartSiloMonitor);
        if (FuncPsStartSiloMonitor == NULL)
        {
            return(STATUS_REVISION_MISMATCH);
        }

        UNICODE_STRING textPsAttachSiloToCurrentThread;
        RtlInitUnicodeString(&textPsAttachSiloToCurrentThread, L"PsAttachSiloToCurrentThread");
        FuncPsAttachSiloToCurrentThread = (FUNCPsAttachSiloToCurrentThread)MmGetSystemRoutineAddress(&textPsAttachSiloToCurrentThread);
        if (FuncPsAttachSiloToCurrentThread == NULL)
        {
            return(STATUS_REVISION_MISMATCH);
        }

        UNICODE_STRING textPsDetachSiloFromCurrentThread;
        RtlInitUnicodeString(&textPsDetachSiloFromCurrentThread, L"PsDetachSiloFromCurrentThread");
        FuncPsDetachSiloFromCurrentThread = (FUNCPsDetachSiloFromCurrentThread)MmGetSystemRoutineAddress(&textPsDetachSiloFromCurrentThread);
        if (FuncPsDetachSiloFromCurrentThread == NULL)
        {
            return(STATUS_REVISION_MISMATCH);
        }

        UNICODE_STRING textPsInsertSiloContext;
        RtlInitUnicodeString(&textPsInsertSiloContext, L"PsInsertSiloContext");
        FuncPsInsertSiloContext = (FUNCPsInsertSiloContext)MmGetSystemRoutineAddress(&textPsInsertSiloContext);
        if (FuncPsInsertSiloContext == NULL)
        {
            return(STATUS_REVISION_MISMATCH);
        }

        UNICODE_STRING textPsGetSiloMonitorContextSlot;
        RtlInitUnicodeString(&textPsGetSiloMonitorContextSlot, L"PsGetSiloMonitorContextSlot");
        FuncPsGetSiloMonitorContextSlot = (FUNCPsGetSiloMonitorContextSlot)MmGetSystemRoutineAddress(&textPsGetSiloMonitorContextSlot);
        if (FuncPsGetSiloMonitorContextSlot == NULL)
        {
            return(STATUS_REVISION_MISMATCH);
        }

        UNICODE_STRING textPsGetSiloContext;
        RtlInitUnicodeString(&textPsGetSiloContext, L"PsGetSiloContext");
        FuncPsGetSiloContext = (FUNCPsGetSiloContext)MmGetSystemRoutineAddress(&textPsGetSiloContext);
        if (FuncPsGetSiloContext == NULL)
        {
            return(STATUS_REVISION_MISMATCH);
        }       

        UNICODE_STRING textPsCreateSiloContext;
        RtlInitUnicodeString(&textPsCreateSiloContext, L"PsCreateSiloContext");
        FuncPsCreateSiloContext = (FUNCPsCreateSiloContext)MmGetSystemRoutineAddress(&textPsCreateSiloContext);
        if (FuncPsCreateSiloContext == NULL)
        {
            return(STATUS_REVISION_MISMATCH);
        }       

        UNICODE_STRING textPsDereferenceSiloContext;
        RtlInitUnicodeString(&textPsDereferenceSiloContext, L"PsDereferenceSiloContext");
        FuncPsDereferenceSiloContext = (FUNCPsDereferenceSiloContext)MmGetSystemRoutineAddress(&textPsDereferenceSiloContext);
        if (FuncPsDereferenceSiloContext == NULL)
        {
            return(STATUS_REVISION_MISMATCH);
        }
        
        UNICODE_STRING textPsRemoveSiloContext;
        RtlInitUnicodeString(&textPsRemoveSiloContext, L"PsRemoveSiloContext");
        FuncPsRemoveSiloContext = (FUNCPsRemoveSiloContext)MmGetSystemRoutineAddress(&textPsRemoveSiloContext);
        if (FuncPsRemoveSiloContext == NULL)
        {
            return(STATUS_REVISION_MISMATCH);
        }               
    }

    
    WDF_DRIVER_CONFIG_INIT(
        &config,
        WDF_NO_EVENT_CALLBACK // This is a non-pnp driver.
        );

    //
    // Set the pool tags associated with this driver. This tag will
    // be assigned to all driver's pool allocations.
    //
    config.DriverPoolTag = KTLLOG_TAG;

    // Tell the framework that this is non-pnp driver so that it doesn't
    // set the default AddDevice routine.
    config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
    
    //
    // NonPnp driver must explicitly register an unload routine for
    // the driver to be unloaded.
    //
    config.EvtDriverUnload = KtlLogEvtDriverUnload;

    //
    // Register a cleanup callback so that we can cleanup any resources
    // allocated from within DriverEntry. The DriverContextCleanup is
    // called during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = KtlLogEvtDriverContextCleanup;

    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             &hDriver);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    DriverHandle = hDriver;

    //
    // Be sure to do any cleanups on error as DriverUnload may not be
    // called
    //
    KFinally([&]()
    {
        if (! NT_SUCCESS(status))
        {
            if (OverlayManagerRegistered)
            {
                NTSTATUS status2;
                
                status2 = FileObjectTable::StopAndUnregisterOverlayManager(KtlSystem::GlobalNonPagedAllocator());
                KInvariant(NT_SUCCESS(status2));
                OverlayManagerRegistered = FALSE;
            }

            if (KtlLogKtlInitialized)
            {
                KtlLogDeinitializeKtl();
                KtlLogKtlInitialized = FALSE;
            }

            if (SiloCallbacksRegistered)
            {
                FuncPsUnregisterSiloMonitor(KtlLogSiloMonitor);
                SiloCallbacksRegistered = FALSE;
            }
        }
    });

    status = KtlLogInitializeKtl();
    if (! NT_SUCCESS(status))
    {
        return status;
    }   
    KtlLogKtlInitialized = TRUE;
        
    //
    // Create and register the overlay manager. This is unregistered in KtlLogEvtDriverContextCleanup
    //
    status = FileObjectTable::CreateAndRegisterOverlayManager(KtlSystem::GlobalNonPagedAllocator(),
                                                              KTLLOG_TAG);

    if (! NT_SUCCESS(status))
    {
        return(status); 
    }
    OverlayManagerRegistered = TRUE;
    
    
    //
    // In order to create a control device, we first need to allocate a
    // WDFDEVICE_INIT structure and set all properties.
    //
    pInit = WdfControlDeviceInitAllocate(
                            hDriver,
                            &SDDL_DEVOBJ_SYS_ALL_ADM_ALL
                            );

    if (pInit == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    //
    // Call NonPnpDeviceAdd to create a deviceobject to represent our
    // software device.
    //
    WDFDEVICE controlDevice;
    status = KtlLogEvtDeviceAdd(hDriver, pInit, TRUE, &controlDevice);
    if (! NT_SUCCESS(status))
    {
        //
        // pInit is cleaned up in EvtDeviceAdd
        //
        return status;
    }


    //
    // If container aware apis are available then call them. Even if
    // there are no containers used, calling doesn't hurt. Note that if
    // there are already containers created, the container CreateNotify
    // callback will be invoked in the context of the
    // PsStartSiloMonitor() call. 
    //
    if (FuncPsRegisterSiloMonitor != NULL)
    {
        //
        // Register for silo notifications
        //
        SILO_MONITOR_REGISTRATION Registration;

        RtlZeroMemory(&Registration, sizeof(Registration));
        Registration.Version = SILO_MONITOR_REGISTRATION_VERSION;
        Registration.DriverObjectName = &DriverObject->DriverName;
        Registration.CreateCallback = KtlLogSiloCreateNotify;
        Registration.TerminateCallback = KtlLogSiloTerminateNotify;
        Registration.MonitorExistingSilos = TRUE;  // Reattach to any existing silos

        status = FuncPsRegisterSiloMonitor(&Registration, &KtlLogSiloMonitor);

        if (NT_SUCCESS(status))
        {
            SiloCallbacksRegistered = TRUE;
            status = FuncPsStartSiloMonitor(KtlLogSiloMonitor);

            if (! NT_SUCCESS(status))
            {
                //
                // This can return STATUS_NOT_SUPPORTED in the case where
                // there are other silos already running. In this case we
                // fail driver load. If there are already silos running and
                // we are reloading then we should expect that this machine
                // will want to run more silos after the driver load.
                //
                return(status);
            }
        }
    }
    
    return STATUS_SUCCESS;
}

//
// Silo Support
// Callbacks on Create/Terminate
//

NTSTATUS
NTAPI
KtlLogSiloCreateNotify(
    __in PESILO ServerSilo
    )
/*++

Routine Description:

    This routine is the silo creation callback

Arguments:

    ServerSilo - Silo associated with this call

Return Value:

    Status of call.

--*/
{
    WDFDEVICE controlDevice;
    NTSTATUS status = STATUS_SUCCESS;
    PESILO previousSilo;
    PVOID siloContext;  

    PAGED_CODE();

    previousSilo = FuncPsAttachSiloToCurrentThread(ServerSilo);
    status = KtlLogCreateDevice(DriverHandle, &controlDevice);
    if (NT_SUCCESS(status))
    {
        KFinally([&]()
        {
            if (! NT_SUCCESS(status))
            {
                KtlLogDeleteDevice(controlDevice);
            }
        });

        status = FuncPsCreateSiloContext(ServerSilo,
                                            sizeof(WDFDEVICE),
                                            PagedPool,
                                            NULL,
                                            &siloContext);
        if (NT_SUCCESS(status))
        {
            *((WDFDEVICE*)siloContext) = controlDevice;
            status = FuncPsInsertSiloContext(
                ServerSilo,
                FuncPsGetSiloMonitorContextSlot(KtlLogSiloMonitor),
                siloContext);
        }

    }
    FuncPsDetachSiloFromCurrentThread(previousSilo);

    return status;
}

VOID
NTAPI
KtlLogSiloTerminateNotify(
    __in PESILO ServerSilo
    )
/*++

Routine Description:

    This routine is the silo termination callback

Arguments:

    ServerSilo - Silo associated with this call

Return Value:

    None.

--*/
{
    NTSTATUS status;
    PVOID siloContext;  
    WDFDEVICE controlDevice;
    PESILO previousSilo;

    PAGED_CODE();
    
    status = FuncPsGetSiloContext(ServerSilo,
                              (*FuncPsGetSiloMonitorContextSlot)(KtlLogSiloMonitor),
                              &siloContext);
    if (NT_SUCCESS(status))
    {
        controlDevice = *((WDFDEVICE*)siloContext);
        if (controlDevice != NULL) {

            previousSilo = FuncPsAttachSiloToCurrentThread(ServerSilo);
            KtlLogDeleteDevice(controlDevice);
            FuncPsDetachSiloFromCurrentThread(previousSilo);
        }

        status = FuncPsRemoveSiloContext(ServerSilo,
                                         (*FuncPsGetSiloMonitorContextSlot)(KtlLogSiloMonitor),
                                         &siloContext);

        //
        // Dereference twice - once for the GetSiloContext above and
        // once for creation and once for the context returned from
        // Remove.
        //
        FuncPsDereferenceSiloContext(siloContext);
        FuncPsDereferenceSiloContext(siloContext);
        FuncPsDereferenceSiloContext(siloContext);
                                                
    }
}

NTSTATUS KtlLogCreateDevice(
    IN WDFDRIVER Driver,
    OUT WDFDEVICE* ControlDevice
    )
{
    NTSTATUS status;
    PWDFDEVICE_INIT                pInit = NULL;
    
    //
    // In order to create a control device, we first need to allocate a
    // WDFDEVICE_INIT structure and set all properties.
    //

    //
    // Allow all access to SYSTEM, Administrators and Authenticated
    // Users
    //
    UNICODE_STRING containerSDDL;
    RtlInitUnicodeString(&containerSDDL, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;AU)");
    pInit = WdfControlDeviceInitAllocate(
                            Driver,
                            &SDDL_DEVOBJ_SYS_ALL_ADM_ALL
                            );

    if (pInit == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    //
    // Call NonPnpDeviceAdd to create a deviceobject to represent our
    // software device.
    //
    status = KtlLogEvtDeviceAdd(Driver, pInit, FALSE, ControlDevice);
    if (! NT_SUCCESS(status))
    {
        //
        // pInit is cleaned up in EvtDeviceAdd
        //
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS KtlLogDeleteDevice(
    IN WDFDEVICE ControlDevice
    )
{
    WdfObjectDelete((WDFOBJECT)ControlDevice);
    return(STATUS_SUCCESS);
}

NTSTATUS
KtlLogEvtDeviceAdd(
    IN WDFDRIVER Driver,
    IN PWDFDEVICE_INIT DeviceInit,
    IN BOOLEAN EstablishSecurity,
    OUT WDFDEVICE* ControlDevice                   
    )
/*++

Routine Description:

    Called by the DriverEntry to create a control-device. This call is
    responsible for freeing the memory for DeviceInit.

Arguments:

    DriverObject - a pointer to the object that represents this device
    driver.

    RegistryPath has the registry path for the driver

    DeviceInit - Pointer to a driver-allocated WDFDEVICE_INIT structure.

Return Value:

    STATUS_SUCCESS if initialized; an error otherwise.

--*/
{
    NTSTATUS                        status;
    WDF_OBJECT_ATTRIBUTES           attributes;
    WDF_OBJECT_ATTRIBUTES           requestAttributes;
    WDF_IO_QUEUE_CONFIG             ioQueueConfig;
    WDF_FILEOBJECT_CONFIG           fileConfig;
    WDFQUEUE                        queue;
    WDFDEVICE                       controlDevice;
    PCONTROL_DEVICE_EXTENSION       devExt;

    DECLARE_CONST_UNICODE_STRING(ntDeviceName, NTDEVICE_NAME_STRING) ;
    DECLARE_CONST_UNICODE_STRING(symbolicLinkName, SYMBOLIC_NAME_STRING) ;
    
    UNREFERENCED_PARAMETER( Driver );

    PAGED_CODE();

    KFinally([&]()
    {
        //
        // If the device is created successfully, framework would clear the
        // DeviceInit value. Otherwise device create must have failed so we
        // should free the memory ourself.
        //
        if (DeviceInit != NULL) {
            WdfDeviceInitFree(DeviceInit);
        }       
    });
    
    //
    // Set exclusive to FALSE so that many app can talk to the
    // control device at any time.
    //
    WdfDeviceInitSetExclusive(DeviceInit, FALSE);

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    //
    // Register for machine shutdown notification
    //
    WdfControlDeviceInitSetShutdownNotification(DeviceInit,
                                                KtlLogShutdown,
                                                WdfDeviceShutdown);
    
    //
    // Initialize WDF_FILEOBJECT_CONFIG_INIT struct to tell the
    // framework whether you are interested in handling Create, Close and
    // Cleanup requests that gets generated when an application or another
    // kernel component opens an handle to the device. If you don't register
    // the framework default behaviour would be to complete these requests
    // with STATUS_SUCCESS. A driver might be interested in registering these
    // events if it wants to do security validation and also wants to maintain
    // per handle (fileobject) context.
    //

    // Add FILE_OBJECT_EXTENSION as the context to the file object.
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, FILE_CONTEXT);
    
    WDF_FILEOBJECT_CONFIG_INIT(
                        &fileConfig,
                        KtlLogEvtDeviceFileCreate,
                        KtlLogEvtFileContextClose,
                        WDF_NO_EVENT_CALLBACK
                        );

    //
    // Since our driver is a legacy driver and should not have any
    // filter on top or below, the FsContext is available for our usage
    //
    fileConfig.FileObjectClass = WdfFileObjectWdfCanUseFsContext;

    WdfDeviceInitSetFileObjectConfig(DeviceInit,
                                     &fileConfig,
                                     &attributes);

    //
    // In order to support marshalling KIoBuffer we need to register for the
    // EvtDeviceIoInProcessContext callback so that we can handle the request
    // in the calling threads context.
    //
    WdfDeviceInitSetIoInCallerContextCallback(DeviceInit,
                                              KtlLogEvtDeviceIoInCallerContext);

    //
    // Specify the size of device context
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,
                                            CONTROL_DEVICE_EXTENSION);
    attributes.EvtCleanupCallback = KtlLogEvtDeviceContextCleanup;  

    status = WdfDeviceInitAssignName(DeviceInit, &ntDeviceName);

    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    //
    // Now specify the size of request extension which the framework
    // will automatically include with each request
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&requestAttributes, REQUEST_CONTEXT);
    requestAttributes.EvtCleanupCallback = KtlLogEvtRequestContextCleanup;  
    WdfDeviceInitSetRequestAttributes(DeviceInit, &requestAttributes);  
    
    status = WdfDeviceCreate(&DeviceInit,
                             &attributes,
                             &controlDevice);
    if (!NT_SUCCESS(status)) {
        return(status);
    }

    devExt = ControlGetData(controlDevice);
    if (devExt != NULL)
    {        
        //
        // Grab the allocator
        //
        devExt->KtlAllocator = &KtlSystem::GlobalNonPagedAllocator();        
    }   
    
    //
    // Create a symbolic link for the control object so that usermode can open
    // the device.
    //

    status = WdfDeviceCreateSymbolicLink(controlDevice,
                                         &symbolicLinkName);

    if (!NT_SUCCESS(status) && (status != STATUS_OBJECT_NAME_COLLISION))
    {
        //
        // Control device will be deleted automatically by the framework.
        //
        return(status);
    }

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //  
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                                           WdfIoQueueDispatchParallel ); 

    ioQueueConfig.EvtIoDeviceControl = KtlLogIoDeviceControl;
    ioQueueConfig.EvtIoCanceledOnQueue = KtlLogEvtRequestCancelOnQueue;
    
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    //
    // By default, Static Driver Verifier (SDV) displays a warning if it 
    // doesn't find the EvtIoStop callback on a power-managed queue. 
    // The 'assume' below causes SDV to suppress this warning. If the driver 
    // has not explicitly set PowerManaged to WdfFalse, the framework creates
    // power-managed queues when the device is not a filter driver.  Normally 
    // the EvtIoStop is required for power-managed queues, but for this driver
    // it is not needed b/c the driver doesn't hold on to the requests or 
    // forward them to other drivers. This driver completes the requests 
    // directly in the queue's handlers. If the EvtIoStop callback is not 
    // implemented, the framework waits for all driver-owned requests to be
    // done before moving in the Dx/sleep states or before removing the 
    // device, which is the correct behavior for this type of driver.
    // If the requests were taking an indeterminate amount of time to complete,
    // or if the driver forwarded the requests to a lower driver/another stack,
    // the queue should have an EvtIoStop/EvtIoResume.
    //
    __analysis_assume(ioQueueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(controlDevice,
                              &ioQueueConfig,
                              &attributes,
                              &queue // pointer to default queue
                              );
    __analysis_assume(ioQueueConfig.EvtIoStop == 0);
    if (!NT_SUCCESS(status))
    {
        //
        // Control device will be deleted automatically by the framework.
        //
        return(status);
    }

    //
    // Control devices must notify WDF when they are done initializing.   I/O is
    // rejected until this call is made.
    //
    WdfControlFinishInitializing(controlDevice);

    if (EstablishSecurity)
    {
        //
        // Establish security descriptor for device object. There is a case
        // where the security descriptor registry key is not yet setup and
        // so we shouldn't fail driver load.
        //
        NTSTATUS statusDontCare;
        statusDontCare = KtlLogEstablishSecurity(controlDevice);
    }

    *ControlDevice = controlDevice;
    return status;

}

NTSTATUS KtlLogEstablishSecurity(
    __in WDFDEVICE ControlDevice
    )
{
    NTSTATUS status;
    WDFDRIVER driver;
    WDFKEY key;
    WDFMEMORY memory;
    size_t size;
    DECLARE_CONST_UNICODE_STRING(securityDescriptorName, L"SecurityDescriptor");
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;
    PDEVICE_OBJECT deviceObject;
    HANDLE fileHandle;

    driver = WdfDeviceGetDriver(ControlDevice);
    if (driver == NULL)
    {
        return(STATUS_NOT_FOUND);
    }

    deviceObject = WdfDeviceWdmGetDeviceObject(ControlDevice);
    if (deviceObject == NULL)
    {
        return(STATUS_NOT_FOUND);       
    }
        
    status = WdfDriverOpenParametersRegistryKey(driver,
                                                KEY_READ,
                                                NULL,         // KeyAttributes
                                                &key);

    if (NT_SUCCESS(status))
    {
        status = WdfRegistryQueryMemory(key,
                                        &securityDescriptorName,
                                        PagedPool,
                                        NULL,       // MemoryAttributes
                                        &memory,
                                        NULL);      // ValueType
        if (NT_SUCCESS(status))
        {
            securityDescriptor = WdfMemoryGetBuffer(memory,
                                                    &size);
            if (securityDescriptor != NULL)
            {
                status = ObOpenObjectByPointer(deviceObject,
                                                   OBJ_KERNEL_HANDLE,
                                                   NULL,
                                                   WRITE_DAC,
                                                   0,
                                                   KernelMode,
                                                   &fileHandle);

                if (NT_SUCCESS(status))
                {
                    status = ZwSetSecurityObject(fileHandle, DACL_SECURITY_INFORMATION, securityDescriptor);
                    
                    ZwClose(fileHandle);
                }
            }

            WdfObjectDelete(memory);
        }
                                        
        WdfRegistryClose(key);
    }

    return(status);
}


VOID KtlLogEvtDeviceContextCleanup (
    IN WDFOBJECT Object
    )
{
    UNREFERENCED_PARAMETER(Object);
    return;
}

VOID
KtlLogEvtDeviceFileCreate(
    IN WDFDEVICE            Device,
    IN WDFREQUEST           Request,
    IN WDFFILEOBJECT        FileObject
    )
/*++

Routine Description:

    The framework calls a driver's EvtDeviceFileCreate callback
    when it receives an IRP_MJ_CREATE request.
    The system sends this request when a user application opens the
    device to perform an I/O operation, such as reading or writing a file.
    This callback is called synchronously, in the context of the thread
    that created the IRP_MJ_CREATE request.

Arguments:

    Device - Handle to a framework device object.
    FileObject - Pointer to fileobject that represents the open handle.
    CreateParams - Parameters of IO_STACK_LOCATION for create

Return Value:

   NT status code

--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PCONTROL_DEVICE_EXTENSION   devExt;
    PFILE_CONTEXT fileContext;

    PAGED_CODE ();

    devExt = ControlGetData(Device);

    //
    // Create a file handle context and initialize it with what is
    // needed
    //
    fileContext = GetFileContext(FileObject);
    ASSERT(fileContext != NULL);

    status = FileObjectTable::Create(*devExt->KtlAllocator,
                                     KTLLOG_TAG,
                                     fileContext->FOT);
    if (NT_SUCCESS(status))
    {
        //
        // Take refcount on the FOT, it will be released when
        // the handle is closed. 
        //
        fileContext->FOT->AddRef();
    }
    
    WdfRequestComplete(Request, status);

    return;
}


VOID
KtlLogEvtFileContextClose (
    IN WDFFILEOBJECT    FileObject
    )

/*++

Routine Description:

   EvtFileClose is called when all the references to the FileObject for
   the device are closed. All request have been completed.

    NOTE: this may be called at raised IRQL
    
Arguments:

    FileObject - Pointer to fileobject that represents the open handle.

Return Value:

   VOID

--*/
{
    // TODO: REMOVE WHEN DEBUGGED
    static BOOLEAN EmptyFOT = TRUE;
    // TODO: REMOVE WHEN DEBUGGED
    
    PFILE_CONTEXT fileContext;

    fileContext = GetFileContext(FileObject);
    ASSERT(fileContext != NULL);

    
    //
    // Release refcount on file object table for this handle
    //
    FileObjectTable* fOT = fileContext->FOT.RawPtr();
    fileContext->FOT = nullptr;

    if (fOT != NULL)
    {
        //
        // Cleanup all resources associated with this handle
        //
        RequestMarshallerKernel::Cleanup(*fOT);
    } else {
    // TODO: REMOVE WHEN DEBUGGED
        EmptyFOT = TRUE;
    // TODO: REMOVE WHEN DEBUGGED       
    }
}

VOID KtlLogEvtRequestContextCleanup (
    IN WDFOBJECT Object
    )
{
    PREQUEST_CONTEXT reqContext;
    reqContext = GetRequestContext(Object);

    //
    // NOTE: This may be called at raised IRQL
    //

    if (reqContext->Marshaller)
    {
        //
        // Free Marshaller and devioctl when cleaning up proper
        // IoControl requests
        //
        reqContext->Marshaller->Reset();
        reqContext->Marshaller = nullptr;
        reqContext->DevIoCtl = nullptr;
    }
}

VOID KtlLogEvtRequestCancel(
    __in WDFREQUEST Request
    )
{
    PREQUEST_CONTEXT reqContext;
    reqContext = GetRequestContext(Request);

    reqContext->DevIoCtl->SyncRequestCancellation(reqContext->Marshaller);
}  

VOID
KtlLogEvtRequestCancelOnQueue(
    IN WDFQUEUE  Queue,
    IN WDFREQUEST  Request
    )

{
    UNREFERENCED_PARAMETER(Queue);
    
    PREQUEST_CONTEXT reqContext;
    
    reqContext = GetRequestContext(Request);
    ASSERT(reqContext != NULL);

    //
    // The driver has taken ownership of the request from WDF and is
    // required to complete it. Set the flag that indicates the request
    // has passed the queue.
    //  
    KtlLogEvtRequestCancel(Request);
    reqContext->DevIoCtl->SyncRequestCompletion();
}

VOID
KtlLogEvtDeviceIoInCallerContext(
    IN WDFDEVICE  Device,
    IN WDFREQUEST Request
    )
/*++
Routine Description:

    This I/O in-process callback is called in the calling threads context/address
    space before the request is subjected to any framework locking or queueing
    scheme based on the device pnp/power or locking attributes set by the
    driver. The process context of the calling app is guaranteed as long as
    this driver is a top-level driver and no other filter driver is attached
    to it.

    This callback is only required if you are handling method-neither IOCTLs,
    or want to process requests in the context of the calling process.

    Driver developers should avoid defining neither IOCTLs and access user
    buffers, and use much safer I/O tranfer methods such as buffered I/O
    or direct I/O.

Arguments:

    Device - Handle to a framework device object.

    Request - Handle to a framework request object. Framework calls
              PreProcess callback only for Read/Write/ioctls and internal
              ioctl requests.

Return Value:

    VOID

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PCONTROL_DEVICE_EXTENSION  devExt;
    PREQUEST_CONTEXT reqContext = NULL;
    WDF_REQUEST_PARAMETERS params;
    PFILE_CONTEXT fileContext;
    WDFFILEOBJECT fileObject;
    FileObjectTable::SPtr fOT;
    DevIoControlKernel::SPtr devIoCtl;

    PAGED_CODE();

    devExt = ControlGetData(Device);
    
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request,  &params );

    //
    // Do basic verification to see if this is out IOCTL and if not
    // then pass it forward to the device queue. Full validation will
    // happen there
    //
    if ((params.Type != WdfRequestTypeDeviceControl) ||
        ((params.Type == WdfRequestTypeDeviceControl) &&
         (params.Parameters.DeviceIoControl.IoControlCode != KTLLOGGER_DEVICE_IOCTL_X)))
    {
        status = WdfDeviceEnqueueRequest(Device, Request);
        if( !NT_SUCCESS(status) )
        {
            goto End;
        }
        return;
    }

    reqContext = GetRequestContext(Request);
    ASSERT(reqContext != NULL); 

    status = RequestMarshallerKernel::Create(reqContext->Marshaller,
                                             *devExt->KtlAllocator,
                                             KTLLOG_TAG);

    if (!NT_SUCCESS(status))
    {
        goto End;
    }

    status = DevIoControlKm::Create(Request,
                                    devIoCtl,                                    
                                    *devExt->KtlAllocator,
                                    KTLLOG_TAG);

    if (!NT_SUCCESS(status))
    {
        goto End;
    }

    reqContext->DevIoCtl = down_cast<DevIoControlKm, DevIoControlKernel>(devIoCtl);
    
    //
    // Grab out file context
    //
    fileObject = WdfRequestGetFileObject(Request);
    if (fileObject == NULL)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto End;
    }

    fileContext = GetFileContext(fileObject);
    ASSERT(fileContext != NULL);

    //
    // Unmarshall the parameters from the buffer and for any KIoBuffers 
    // from the request, probe and lock them
    //
    fOT = fileContext->FOT;

    if (fOT)
    {
        BOOLEAN completeRequest;
        ULONG outBufferSize;
        
        status = reqContext->Marshaller->MarshallRequest((PVOID)Request,
                                                         fOT,
                                                         up_cast<DevIoControlKernel>(reqContext->DevIoCtl),
                                                         &completeRequest,
                                                         &outBufferSize);
        if (completeRequest)
        {
            WdfRequestSetInformation(Request, outBufferSize);           
            goto End;
        }

        status = reqContext->Marshaller->PerformRequest(fOT,
                                                        up_cast<DevIoControlKernel>(reqContext->DevIoCtl));

        if (status != STATUS_PENDING)
        {
            if (! NT_SUCCESS(status))
            {
                WdfRequestSetInformation(Request, 0);
            }
            WdfRequestComplete( Request, status);
        }

    } else {
        status = STATUS_INVALID_DEVICE_REQUEST;
        goto End;
    }

    //
    // Send the request back to the framework so that it can be
    // delivered to the EvtIoDeviceControl() through the queue
    //
    status = WdfDeviceEnqueueRequest(Device, Request);
    if( ! NT_SUCCESS(status) )
    {
        goto End;
    }
    
    return;
    
End:

    WdfRequestComplete(Request, status);
    return;
}

VOID
KtlLogIoDeviceControl(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           OutputBufferLength,
    IN size_t           InputBufferLength,
    IN ULONG            IoControlCode
    )
/*++
Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.

Return Value:

   VOID

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( Queue );
    UNREFERENCED_PARAMETER( InputBufferLength );
    UNREFERENCED_PARAMETER( OutputBufferLength );

    PAGED_CODE();

    //
    // Determine which I/O control code was specified.
    //
    switch (IoControlCode)
    {
        case KTLLOGGER_DEVICE_IOCTL_X:
        {
            //
            // Grab the request and file contexts
            //
            PREQUEST_CONTEXT reqContext;
            
            reqContext = GetRequestContext(Request);
            ASSERT(reqContext != NULL);

            //
            // See if the request has been cancelled and setup the
            // cancellation routine. It is unlikely to return STATUS_CANCELLED
            // here since the request was just dispatched from the
            // queue. If the request was cancelled while on the queue
            // then the KtlLogEvtRequestCancelOnQueue would have been
            // invoked and the request not dispatched here. At this
            // point we do not know if the request has finished or not.
            // If it has finished then it is safe to ignore cancel
            // since the request will be completed. If not finished we
            // need to call the cancel routine to close this window.
            //
            status = WdfRequestMarkCancelableEx(Request,
                                                KtlLogEvtRequestCancel);

            if (status == STATUS_CANCELLED)
            {
                KtlLogEvtRequestCancel(Request);
            }
            
            //
            // See if request has already finished. Don't touch it
            // after this - either it was completed in the call or it
            // will be completed on another thread.
            //
            reqContext->DevIoCtl->SyncRequestCompletion();
            status = STATUS_PENDING;
            
            break;
        }
            
        default:
        {       
            //
            // The specified I/O control code is unrecognized by this driver.
            //
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
    }

    if (status != STATUS_PENDING)
    {
        if (! NT_SUCCESS(status))
        {
            WdfRequestSetInformation(Request, 0);
        }
        WdfRequestComplete( Request, status);
    }
}


VOID
KtlLogShutdown(
    WDFDEVICE Device
    )
/*++

Routine Description:
    Callback invoked when the machine is shutting down.  If you register for
    a last chance shutdown notification you cannot do the following:
    o Call any pageable routines
    o Access pageable memory
    o Perform any file I/O operations

    If you register for a normal shutdown notification, all of these are
    available to you.

    This function implementation does nothing, but if you had any outstanding
    file handles open, this is where you would close them.

Arguments:
    Device - The device which registered the notification during init

Return Value:
    None

  --*/

{
    UNREFERENCED_PARAMETER(Device);
    return;
}


VOID
KtlLogEvtDriverUnload(
    IN WDFDRIVER Driver
    )
/*++
Routine Description:

   Called by the I/O subsystem just before unloading the driver.
   You can free the resources created in the DriverEntry either
   in this routine or in the EvtDriverContextCleanup callback.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

Return Value:

    NTSTATUS

--*/
{
    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    return;
}

VOID
KtlLogEvtDriverContextCleanup(
    IN WDFOBJECT Driver
    )
/*++
Routine Description:

   Called when the driver object is deleted during driver unload.
   You can free all the resources created in DriverEntry that are
   not automatically freed by the framework.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

Return Value:

    NTSTATUS

--*/
{
    UNREFERENCED_PARAMETER(Driver);

    NTSTATUS status;
    
    PAGED_CODE();

    if (OverlayManagerRegistered)
    {
        status = FileObjectTable::StopAndUnregisterOverlayManager(KtlSystem::GlobalNonPagedAllocator());
        KInvariant(NT_SUCCESS(status));
        OverlayManagerRegistered = FALSE;
    }
    
    if (KtlLogKtlInitialized)
    {
        //
        // Shutdown KTL
        //
        KtlLogDeinitializeKtl();
        KtlLogKtlInitialized = FALSE;
    }

    if (SiloCallbacksRegistered)
    {
        FuncPsUnregisterSiloMonitor(KtlLogSiloMonitor);
        SiloCallbacksRegistered = FALSE;
    }   
}
