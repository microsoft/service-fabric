// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*++

Module Name:

NPHelpers.c

Abstract:

This file includes NamedPipe transport support for Service Fabric BlockStore Device's interaction with the BlockStore Service.

Environment:

Kernel mode only.

--*/

#include "inc\SFBDVMiniport.h"
#include "inc\BlockStore.h"

#if defined(USE_NP_TRANSPORT)

// Attempts to connect to the named pipe of the service and return a valid handle if connected.
// Returns NULL on failure.
NTSTATUS ConnectTo(PUNICODE_STRING pipeName, PHANDLE phPipe)
{
    NTSTATUS            ntStatus;
    IO_STATUS_BLOCK     IoStatus = { 0 };
    OBJECT_ATTRIBUTES   ObjectAttributes;

    *phPipe = NULL;

    InitializeObjectAttributes(&ObjectAttributes,
        pipeName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    ntStatus = ZwCreateFile(phPipe,
        SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE, 
        &ObjectAttributes,
        &IoStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);

    if (NT_SUCCESS(ntStatus))
    {
        EventWriteNamePipeConnectionSuccess(NULL);
        KdPrint(("Successfully connected to named pipe.\n"));
    }
    else
    {
        EventWriteNamePipeConnectionFailed(NULL, ntStatus);
        KdPrint(("Unable to connect to named pipe due to (NT_STATUS) error: %08X.\n", ntStatus));
    }

    return ntStatus;
}

NTSTATUS SendBufferEx(HANDLE hPipe, PGATEWAY_PAYLOAD pPayload, PVOID pipeBuffer, ULONG pipeBufferLength)
{
    // Initialize the buffer for payload header
    UCHAR payloadHeader[GET_PAYLOAD_HEADER_SIZE];
    RtlZeroMemory(&payloadHeader, sizeof(payloadHeader));

    // Initialize and write the payload header
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    CopyPayloadHeaderToBuffer(pPayload, (PUCHAR)payloadHeader);
    NTSTATUS status = ZwWriteFile(hPipe, NULL, NULL, NULL, &ioStatusBlock, payloadHeader, sizeof(payloadHeader), NULL, NULL);
    if (NT_SUCCESS(status))
    {
        // We should have written the expected number of bytes.
        if (ioStatusBlock.Information != sizeof(payloadHeader))
        {
            NT_ASSERT(FALSE);
            status = GET_RW_ERROR_STATUS((pPayload->mode == Read));
        }
        else
        {
            // Write the actual payload to the pipe
            status = CopyPayloadToBuffer(pPayload, hPipe, pipeBuffer, pipeBufferLength);
        }
    }

    return status;
}

// Send the payload to the service via the specified pipe handle.
NTSTATUS SendBuffer(HANDLE hPipe, PGATEWAY_PAYLOAD pPayload)
{
    return SendBufferEx(hPipe, pPayload, NULL, 0);
}

// Receive the payload response via the specified pipe handle
NTSTATUS ReceiveBufferEx(HANDLE hPipe, PGATEWAY_PAYLOAD pPayload, PVOID pipeBuffer, ULONG pipeBufferLength)
{
    // Initialize the buffer for payload header
    UCHAR payloadHeader[GET_PAYLOAD_HEADER_SIZE];
    RtlZeroMemory(&payloadHeader, sizeof(payloadHeader));

    // Read the payload header
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    NTSTATUS status = ZwReadFile(hPipe, NULL, NULL, NULL, &ioStatusBlock, payloadHeader, sizeof(payloadHeader), NULL, NULL);
    if (NT_SUCCESS(status))
    {
        // We should have read the expected number of bytes.
        if (ioStatusBlock.Information != sizeof(payloadHeader))
        {
            NT_ASSERT(FALSE);
            status = GET_RW_ERROR_STATUS((pPayload->mode == Read));
        }
        else
        {
            status = CopyPayloadFromIOBuffer(pPayload, payloadHeader, hPipe, pipeBuffer, pipeBufferLength);
        }
    }

    return status;
}

// Receive the payload response via the specified pipe handle
NTSTATUS ReceiveBuffer(HANDLE hPipe, PGATEWAY_PAYLOAD pPayload)
{
    return ReceiveBufferEx(hPipe, pPayload, NULL, 0);
}

NTSTATUS CloseConnection(HANDLE hPipe)
{
    return ZwClose(hPipe);
}

#endif // defined(USE_NP_TRANSPORT)