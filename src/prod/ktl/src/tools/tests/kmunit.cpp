/*++

Copyright (c) Microsoft Corporation

Module Name:

    KmUnit.cpp

Abstract:

    This file contains skeleton of the kernel-mode unit test driver.

Environment:

    Kernel mode only.

--*/
#pragma once
#include "KmUnit.h"
#ifndef KM_LIBRARY
#include "KmUnitTestCases.h"
#endif
#include <KmUnit.tmh>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, KuEvtDriverUnload)
#pragma alloc_text(PAGE, KuShutdown)
#endif

__drv_functionClass(DRIVER_INITIALIZE)
__drv_sameIRQL
NTSTATUS
DriverEntry(
    __in PDRIVER_OBJECT     DriverObject,
    __in PUNICODE_STRING    RegistryPath
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

    NTSTATUS.

--*/
{
    NTSTATUS                       status;
    WDF_DRIVER_CONFIG              config;
    WDFDRIVER                      hDriver;
    PWDFDEVICE_INIT                deviceInit = NULL;
    WDF_OBJECT_ATTRIBUTES          attributes;

    DbgPrintEx(0, 0xFFFFFFFF, "KmUnit DriverEntry\n");    

    WDF_DRIVER_CONFIG_INIT(
        &config,
        WDF_NO_EVENT_CALLBACK // This is a non-pnp driver.
        );

    //
    // Tell the framework that this is non-pnp driver so that it doesn't
    // set the default AddDevice routine.
    //
    config.DriverInitFlags |= WdfDriverInitNonPnpDriver;

    //
    // NonPnp driver must explicitly register an unload routine for
    // the driver to be unloaded.
    //
    config.EvtDriverUnload = KuEvtDriverUnload;

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = KuEvtDriverContextCleanup;

    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(DriverObject, RegistryPath, &attributes, 
        &config, &hDriver);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(0, 0xFFFFFFFF, "Ku: WdfDriverCreate failed with status 0x%x\n", status);        
        return status;
    }

    //
    // Since we are calling WPP_CLEANUP in the DriverContextCleanup
    // callback we should initialize WPP Tracing after WDFDRIVER
    // object is created to ensure that we cleanup WPP properly
    // if we return failure status from DriverEntry. This
    // eliminates the need to call WPP_CLEANUP in every path
    // of DriverEntry.
    //
    
    WPP_INIT_TRACING(DriverObject, RegistryPath);
    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "KmUnit driver started");

    //
    // In order to create a control device, we first need to allocate a
    // WDFDEVICE_INIT structure and set all properties.
    //
    
    deviceInit = WdfControlDeviceInitAllocate(hDriver, 
        &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R);
    if (deviceInit == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    //
    // Call KuDeviceAdd to create a deviceobject to represent our
    // software device.
    //
    
    status = KuDeviceAdd(hDriver, deviceInit);    
    return status;
}

NTSTATUS
KuDeviceAdd(
    __in WDFDRIVER          Driver,
    __in PWDFDEVICE_INIT    DeviceInit
    )
    
/*++

Routine Description:

    Called by the DriverEntry to create a control-device. This call is
    responsible for freeing the memory for DeviceInit.

Arguments:

    DriverObject - a pointer to the object that represents this device
    driver.

    DeviceInit - Pointer to a driver-allocated WDFDEVICE_INIT structure.

Return Value:

    STATUS_SUCCESS if initialized; an error otherwise.

--*/
{
    NTSTATUS                        status = STATUS_SUCCESS;
    WDF_IO_QUEUE_CONFIG             ioQueueConfig;
    WDFQUEUE                        queue;
    WDFDEVICE                       controlDevice;

    DECLARE_CONST_UNICODE_STRING(ntDeviceName, KU_NT_CONTROL_NAME) ;
    DECLARE_CONST_UNICODE_STRING(symbolicLinkName, KU_DOS_CONTROL_NAME) ;

    UNREFERENCED_PARAMETER(Driver);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "KuDeviceAdd DeviceInit %p\n", DeviceInit);
    
    //
    // Set exclusive to TRUE so that no more than one app can talk to the
    // control device at any time.
    //
    
    WdfDeviceInitSetExclusive(DeviceInit, TRUE);

    status = WdfDeviceInitAssignName(DeviceInit, &ntDeviceName);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "WdfDeviceInitAssignName failed %!STATUS!", status);
        goto End;
    }

    WdfControlDeviceInitSetShutdownNotification(DeviceInit,
                                                KuShutdown,
                                                WdfDeviceShutdown);
                                                
    status = WdfDeviceCreate(&DeviceInit,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &controlDevice);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "WdfDeviceCreate failed %!STATUS!", status);
        goto End;
    }
    
    //
    // Create a symbolic link for the control object so that usermode can open
    // the device.
    //

    status = WdfDeviceCreateSymbolicLink(controlDevice, &symbolicLinkName);
    if (!NT_SUCCESS(status)) {
    
        //
        // Control device will be deleted automatically by the framework.
        //
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "WdfDeviceCreateSymbolicLink failed %!STATUS!", status);
        goto End;
    }

    //
    // Configure a default queue. The request queue is sequential 
    // because we only run one test case at a time.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                                    WdfIoQueueDispatchSequential);

    ioQueueConfig.EvtIoDeviceControl = KuEvtIoDeviceControl;

    status = WdfIoQueueCreate(controlDevice,
                              &ioQueueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &queue // pointer to default queue
                              );
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_INIT, "WdfIoQueueCreate failed %!STATUS!", status);
        goto End;
    }
    
    //
    // Control devices must notify WDF when they are done initializing.   
    // I/O is rejected until this call is made.
    //
    WdfControlFinishInitializing(controlDevice);

End:
    //
    // If the device is created successfully, framework would clear the
    // DeviceInit value. Otherwise device create must have failed so we
    // should free the memory ourself.
    //
    if (DeviceInit != NULL) {
        ASSERT(!NT_SUCCESS(status));
        WdfDeviceInitFree(DeviceInit);
    }

    return status;
}

__drv_functionClass(EVT_WDF_OBJECT_CONTEXT_CLEANUP)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
KuEvtDriverContextCleanup(
    __in WDFOBJECT      Object
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
    UNREFERENCED_PARAMETER(Object);
    
    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "Entered KuEvtDriverContextCleanup\n");

    //
    // No need to free the control device object explicitly because it will
    // be deleted when the Driver object is deleted due to the default parent
    // child relationship between Driver and ControlDevice.
    //
    WPP_CLEANUP(WdfDriverWdmGetDriverObject(WdfGetDriver()));
}

__drv_functionClass(EVT_WDF_DEVICE_SHUTDOWN_NOTIFICATION)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
VOID
KuShutdown(
    __in WDFDEVICE Device
    )
    
/*++

Routine Description:

    Callback invoked when the machine is shutting down.  If you register for
    a last chance shutdown notification you cannot do the following:
    - Call any pageable routines
    - Access pageable memory
    - Perform any file I/O operations

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
    PAGED_CODE();    
    UNREFERENCED_PARAMETER(Device);
}

__drv_functionClass(EVT_WDF_DRIVER_UNLOAD)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
VOID
KuEvtDriverUnload(
    __in WDFDRIVER Driver
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
    PAGED_CODE();        
    UNREFERENCED_PARAMETER(Driver);

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_INIT, "Entered KuDriverUnload\n");
}

__drv_functionClass(EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
KuEvtIoDeviceControl(
    __in WDFQUEUE           Queue,
    __in WDFREQUEST         Request,
    __in size_t             OutputBufferLength,
    __in size_t             InputBufferLength,
    __in ULONG              IoControlCode
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

   None.

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PVOID               inBuf = NULL;
    PVOID               outBuf = NULL; 
    size_t              bufferSize;
    size_t              minBufferSize;
    PKU_TEST_START      startCmd;
    PCKU_TEST_ENTRY     testEntry;
    PKU_TEST_ID         testId;
    PKU_TEST_ID_ARRAY   testIdArray;    
    ULONG               i;
    BOOLEAN             completeRequest = TRUE;
    LPWSTR              params[KU_MAX_PARAMETERS];
    PTEST_PARAMS        parameters = NULL;
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);    
    UNREFERENCED_PARAMETER(InputBufferLength);        
    
    switch (IoControlCode)
    {
    case IOCTL_KU_CONTROL_QUERY_NUMBER_OF_TESTS:
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "Called IOCTL_KU_CONTROL_QUERY_NUMBER_OF_TESTS");

        minBufferSize = sizeof(ULONG);
        status = WdfRequestRetrieveOutputBuffer(Request, minBufferSize, &outBuf, &bufferSize);
        if(!NT_SUCCESS(status)) {
            break;
        }

        ASSERT(bufferSize == OutputBufferLength);
        *((PULONG)outBuf) = ARRAY_SIZE(gs_KuTestCases);

        WdfRequestSetInformation(Request, minBufferSize);
        break;
        
    case IOCTL_KU_CONTROL_QUERY_TESTS:
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "Called IOCTL_KU_CONTROL_QUERY_TESTS");
        
        minBufferSize = FIELD_OFFSET(KU_TEST_ID_ARRAY, TestIdArray) + 
            ARRAY_SIZE(gs_KuTestCases) * sizeof(KU_TEST_ID);
        status = WdfRequestRetrieveOutputBuffer(Request, minBufferSize, &outBuf, &bufferSize);
        if(!NT_SUCCESS(status)) {
            break;
        }

        ASSERT(bufferSize == OutputBufferLength);

        RtlZeroMemory(outBuf, bufferSize);

        testIdArray = (PKU_TEST_ID_ARRAY)outBuf;
        testIdArray->NumberOfTests = ARRAY_SIZE(gs_KuTestCases);
        
        for (i = 0; i < ARRAY_SIZE(gs_KuTestCases); ++i) {
            testId = &testIdArray->TestIdArray[i];            
            
            testId->Id = i;
            status = RtlStringCchCopyW(testId->Name, KU_TEST_NAME_LENGTH, gs_KuTestCases[i].TestCaseName);
            if(!NT_SUCCESS(status)) {
                break;
            }
            status = RtlStringCchCopyW(testId->Categories, KU_MAX_PATH, gs_KuTestCases[i].Categories);
            if(!NT_SUCCESS(status)) {
                break;
            }
            status = RtlStringCchCopyW(testId->Help, KU_MAX_PATH, gs_KuTestCases[i].Help);
            if(!NT_SUCCESS(status)) {
                break;
            }
        }

        if(NT_SUCCESS(status)) {            
            WdfRequestSetInformation(Request, minBufferSize);
        } 
        
        break;
        
    case IOCTL_KU_CONTROL_START_TEST:
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "Called IOCTL_KU_CONTROL_START_TEST");
        
        minBufferSize = FIELD_OFFSET(KU_TEST_START, Parameter);
        status = WdfRequestRetrieveInputBuffer(Request, minBufferSize, &inBuf, &bufferSize);
        if(!NT_SUCCESS(status)) {
            break;
        }

        ASSERT(bufferSize == InputBufferLength);
        
        startCmd = (PKU_TEST_START)inBuf;
        if (minBufferSize + startCmd->ParameterBytes > bufferSize) {
            status = STATUS_INVALID_PARAMETER;            
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL, 
                "Running test %d: Cmd buffer (%Iu bytes) is too small for the parameters (%d bytes). %!STATUS!", 
                startCmd->TestId, bufferSize, startCmd->ParameterBytes, status);
            break;
        }        
        
        if (startCmd->TestId >= ARRAY_SIZE(gs_KuTestCases)) {
            status = STATUS_NO_SUCH_FILE;
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL, 
                "Running test %d: No such test. Max test id = %d. %!STATUS!", 
                startCmd->TestId, ARRAY_SIZE(gs_KuTestCases) - 1, status);
            break;
        }

        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "Running test %d: %S", 
            startCmd->TestId, gs_KuTestCases[startCmd->TestId].TestCaseName);        

        testEntry = &gs_KuTestCases[startCmd->TestId];
        //testEntry->TestRoutine(Request);
        parameters = (PTEST_PARAMS)startCmd->Parameter;
        for(int j=0; j<KU_MAX_PARAMETERS; j++)
        {
            params[j] = parameters->Parameter[j];
            //TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "ERROR: unrecognized IOCTL %S", params[j]);
        }
		
		if (startCmd->BreakOnStart && KdRefreshDebuggerNotPresent() == FALSE)
		{
		//
		// The test loader requests �BreakOnStart� and a kernel debugger is active. Break into debugger.
		// This is only effective in chk build.
		//
			KdBreakPoint();
		}

        status = testEntry->TestRoutine(parameters->Count, params);

        //
        // Mark the request to be completed in the test case routine.         
        //
        //completeRequest = FALSE;    
        break;

    default:

        //
        // The specified I/O control code is unrecognized by this driver.
        //
        status = STATUS_INVALID_DEVICE_REQUEST;
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL, "ERROR: unrecognized IOCTL %x", IoControlCode);
        break;
    }
    
    if (completeRequest) {
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "Completing Request %p with status %!STATUS!",
                   Request, status);

        WdfRequestComplete(Request, status);    
    }
}


