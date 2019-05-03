// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

//
// Global variables.
//

/*++

    Handles to the lease layer obtained using CreateFile.

--*/
HANDLE LeaseLayerDeviceAsync = INVALID_HANDLE_VALUE;
HANDLE LeaseLayerDeviceSync = INVALID_HANDLE_VALUE;
DWORD LeaseLayerUninitializedError = ERROR_INVALID_HANDLE;

/*++

    Handle to an I/O Thread Pool used for I/O completion callbacks.

--*/
PTP_IO ThreadPool = NULL;

/*++

    Used for concurrency control on the leasing application hash table.

--*/
Common::RwLock LeaseLayerLock;

//
// I/O thread pool callback.
//
extern VOID CALLBACK 
LeaseLayerIoCompletionCallback(
    __inout PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID Context,
    __inout_opt PVOID Overlapped,
    __in ULONG IoResult,
    __in ULONG_PTR NumberOfBytesTransferred,
    __inout PTP_IO Io
    );

BOOL
IsLeaseLayerInitialized(
    VOID
    )

/*++
 
Routine Description:
 
    Check to see if the user mode lease layer has been successfully initialized.

Return Value:
    
    TRUE if the handle to the device is not NULL.

--*/

{
    if (INVALID_HANDLE_VALUE != LeaseLayerDeviceAsync && INVALID_HANDLE_VALUE != LeaseLayerDeviceSync)
        return true;
    else if (ERROR_FILE_NOT_FOUND == LeaseLayerUninitializedError)
        SetLastError(ERROR_FILE_NOT_FOUND);
    else if (ERROR_ACCESS_DENIED == LeaseLayerUninitializedError)
        SetLastError(ERROR_ACCESS_DENIED);
    else
        SetLastError(ERROR_INVALID_HANDLE);
		
    return false;
}

VOID
InitializeLeaseLayer(
    VOID
    )

/*++

Routine description:

    Initializes the user mode lease layer. Called on process attach
    from DllMain. A handle will be open to the lease layer device.
    Also, an I/O thread pool will be created and associated with that
    handle. The thread pool will be used to complete callbacks whenever
    events from kernel need to be surfaced to the user mode.

Parameters Description:

    n/a

Return Value:
    
    VOID

--*/

{
    //
    // Open lease layer device async handle.
    //
    LeaseLayerDeviceAsync = CreateFile(
        L"\\\\.\\LeaseLayer",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
        NULL
        );

    if (INVALID_HANDLE_VALUE == LeaseLayerDeviceAsync) {

        LeaseLayerUninitializedError = GetLastError();

        //
        // Done.
        //
        return;
    }

    //
    // Open lease layer device sync handle.
    //
    LeaseLayerDeviceSync = CreateFile(
        L"\\\\.\\LeaseLayer",
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING,
        NULL
        );

    if (INVALID_HANDLE_VALUE == LeaseLayerDeviceSync) {

        LeaseLayerUninitializedError = GetLastError();

        //
        // Close async device handle.
        //
        CloseHandle(LeaseLayerDeviceAsync);

        LeaseLayerDeviceAsync = INVALID_HANDLE_VALUE;

        //
        // Done.
        //
        return;
    }

    //
    // Create I/O thread pool.
    //
    ThreadPool = CreateThreadpoolIo(
        LeaseLayerDeviceAsync,
        LeaseLayerIoCompletionCallback,
        NULL,
        NULL
        );

    if (NULL == ThreadPool) {

        //
        // Close sync and async device handles.
        //
        CloseHandle(LeaseLayerDeviceAsync);
        CloseHandle(LeaseLayerDeviceSync);

        LeaseLayerDeviceAsync = INVALID_HANDLE_VALUE;
        LeaseLayerDeviceSync = INVALID_HANDLE_VALUE;
    }
}

/*++

Routine description:

    Entry point into the lease laeyr user mode dynamic-link library.
    On process attach, a handle will be open to the lease layer device.
    Also, an I/O thread pool will be created and associated with that
    handle. The thread pool will be used to complete callbacks whenever
    events from kernel need to be surfaced to the user mode. On process
    detach all resources will be terminated and cleaned up.

--*/
BOOL APIENTRY 
DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved
    )
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        //
        // Delay initializing the user mode lease layer.
        //
        break;

    case DLL_THREAD_ATTACH:
        //
        // Nothing.
        //
        break;

    case DLL_THREAD_DETACH:
        //
        // Nothing.
        //
        break;

    case DLL_PROCESS_DETACH:
		
        if (IsLeaseLayerInitialized()) {

	        //
	        // Close sync and async device handles.
	        //
	        CloseHandle(LeaseLayerDeviceAsync);
	        CloseHandle(LeaseLayerDeviceSync);
        }
		
        break;
    }

    return TRUE;
}

