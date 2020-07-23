// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*++

Module Name:

utils.c

Abstract:

This file includes helper functions for Service Fabric BlockStore Device's interaction with the BlockStore Service.

Environment:

Kernel mode only.

--*/

#include "inc\SFBDVMiniport.h"
#include "inc\BlockStore.h"


/**************************************************************************************************/ 
/*                                                                                                */ 
/* Note: DoStorageTraceETW may not be used here, since tracing won't have been set up.            */ 
/*                                                                                                */ 
/**************************************************************************************************/ 
VOID
SFBDQueryRegParameters(
                     IN PUNICODE_STRING pRegistryPath,
                     IN pSFBDRegistryInfo    pRegInfo
                    )
/*++

Routine Description:

    This routine is called from DriverEntry to get parameters from the registry.  If the registry query 
    fails, default values are used.

Return Value:

    None

--*/
{
    SFBDRegistryInfo defRegInfo;

    // Set default values.

    defRegInfo.BreakOnEntry       = DEFAULT_BREAK_ON_ENTRY;
    defRegInfo.DebugLevel         = DEFAULT_DEBUG_LEVEL;
    defRegInfo.LUNPerHBA          = DEFAULT_LUNPerHBA;

    RtlInitUnicodeString(&defRegInfo.VendorId, VENDOR_ID);
    RtlInitUnicodeString(&defRegInfo.ProductId, PRODUCT_ID);
    RtlInitUnicodeString(&defRegInfo.ProductRevision, PRODUCT_REV);

    // The initialization of lclRtlQueryRegTbl is put into a subordinate block so that the initialized Buffer members of Unicode strings
    // in defRegInfo will be used.

    {
        NTSTATUS                 status;

        #pragma warning(push)
        #pragma warning(disable : 4204)
        #pragma warning(disable : 4221)

        RTL_QUERY_REGISTRY_TABLE lclRtlQueryRegTbl[] = {
            // The Parameters entry causes the registry to be searched under that subkey for the subsequent set of entries.
            {NULL, RTL_QUERY_REGISTRY_SUBKEY | RTL_QUERY_REGISTRY_NOEXPAND, L"Parameters",       NULL,                         (ULONG_PTR)NULL, NULL,                              (ULONG_PTR)NULL},

            {NULL, RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND, L"BreakOnEntry",     &pRegInfo->BreakOnEntry,      REG_DWORD,       &defRegInfo.BreakOnEntry,          sizeof(ULONG)},
            {NULL, RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND, L"DebugLevel",       &pRegInfo->DebugLevel,        REG_DWORD,       &defRegInfo.DebugLevel,            sizeof(ULONG)},
            {NULL, RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND, L"VendorId",         &pRegInfo->VendorId,          REG_SZ,          defRegInfo.VendorId.Buffer,        0},
            {NULL, RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND, L"ProductId",        &pRegInfo->ProductId,         REG_SZ,          defRegInfo.ProductId.Buffer,       0},
            {NULL, RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND, L"ProductRevision",  &pRegInfo->ProductRevision,   REG_SZ,          defRegInfo.ProductRevision.Buffer, 0},
            {NULL, RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND, L"LUNPerHBA",        &pRegInfo->LUNPerHBA,         REG_DWORD,       &defRegInfo.LUNPerHBA,             sizeof(ULONG)},
           
            // The null entry denotes the end of the array.                                                                    
            {NULL, 0,                                                       NULL,                NULL,                         (ULONG_PTR)NULL, NULL,                              (ULONG_PTR)NULL},
        };

        #pragma warning(pop)

        status = RtlQueryRegistryValues(
                                        RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                        pRegistryPath->Buffer,
                                        lclRtlQueryRegTbl,
                                        NULL,
                                        NULL
                                       );

        if (!NT_SUCCESS(status)) {                    // A problem?
            pRegInfo->BreakOnEntry      = defRegInfo.BreakOnEntry;
            pRegInfo->DebugLevel        = defRegInfo.DebugLevel;
            RtlCopyUnicodeString(&pRegInfo->VendorId,  &defRegInfo.VendorId);
            RtlCopyUnicodeString(&pRegInfo->ProductId, &defRegInfo.ProductId);
            RtlCopyUnicodeString(&pRegInfo->ProductRevision, &defRegInfo.ProductRevision);
            pRegInfo->LUNPerHBA     = defRegInfo.LUNPerHBA;
        }
    }
}    

#pragma alloc_text (PAGE, GetCurrentHostname) //added to remove PAGED_CODE warning

VOID
GetCurrentHostname(
    _In_ PUNICODE_STRING Hostname
)

/*++

Routine Description:

This routine is called from the DriverEntry to query the hostname of the machine it is installed on.

Arguments:

Hostname    - Pointer to the buffer that will get the hostname

Return Value:

None

--*/

{

    RTL_QUERY_REGISTRY_TABLE rtlQueryRegTbl[2 + 1];  // Need 1 for NULL
    NTSTATUS                 Status;

    PAGED_CODE();

    RtlZeroMemory(rtlQueryRegTbl, sizeof(rtlQueryRegTbl));

    //
    // Setup the query table
    //

    rtlQueryRegTbl[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    rtlQueryRegTbl[0].Name = L"ComputerName";
    rtlQueryRegTbl[0].EntryContext = NULL;
    rtlQueryRegTbl[0].DefaultType = (ULONG_PTR)NULL;
    rtlQueryRegTbl[0].DefaultData = NULL;
    rtlQueryRegTbl[0].DefaultLength = (ULONG_PTR)NULL;

    //
    // Fetch the hostname
    //

    rtlQueryRegTbl[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    rtlQueryRegTbl[1].Name = L"ComputerName";
    rtlQueryRegTbl[1].EntryContext = Hostname;
    rtlQueryRegTbl[1].DefaultType = REG_SZ;
    rtlQueryRegTbl[1].DefaultData = NULL;
    rtlQueryRegTbl[1].DefaultLength = (ULONG_PTR)NULL;


    Status = RtlQueryRegistryValues(
        RTL_REGISTRY_CONTROL,
        L"ComputerName",
        rtlQueryRegTbl,
        NULL,
        NULL
    );
}

// Helper function to write the payload header to the specified buffer
VOID CopyPayloadHeaderToBuffer(PGATEWAY_PAYLOAD pPayload, PUCHAR pBuffer)
{
    PGATEWAY_PAYLOAD payloadOutgoing = (PGATEWAY_PAYLOAD)pBuffer;

    payloadOutgoing->mode = pPayload->mode;
    payloadOutgoing->offsetToRW = pPayload->offsetToRW;
    payloadOutgoing->sizePayloadBuffer = pPayload->sizePayloadBuffer;
    payloadOutgoing->lengthDeviceID = pPayload->lengthDeviceID;

#if defined(USE_DMA_FOR_SRB_IO)
    payloadOutgoing->addressSRB = pPayload->addressSRB;
#endif // defined(USE_DMA_FOR_SRB_IO)

    RtlCopyMemory(&(payloadOutgoing->deviceID), pPayload->deviceID, pPayload->lengthDeviceID);

    // Increment the offset at which the next write will occur
    PUCHAR pOutgoingPayloadData = (PUCHAR)&(payloadOutgoing->deviceID);
    pOutgoingPayloadData += payloadOutgoing->lengthDeviceID;

    // Write the EndOfPayloadMarker
    ULONG endOfPayloadMarker = END_OF_PAYLOAD_MARKER;
    RtlCopyMemory(pOutgoingPayloadData, &endOfPayloadMarker, sizeof(endOfPayloadMarker));
}

// Copies the actual payload to the specified buffer
NTSTATUS CopyPayloadToBuffer(PGATEWAY_PAYLOAD pPayload
#if !defined(USE_NP_TRANSPORT)
    , PUCHAR pBuffer
#else // defined(USE_NP_TRANSPORT)
    , HANDLE hPipe
    , PVOID pipeBuffer
    , ULONG pipeBufferLength
#endif // !defined(USE_NP_TRANSPORT)
)
{
    NTSTATUS status = STATUS_SUCCESS;

    // Use the actual payload length if specified. Otherwise, use the
    // payload buffer size.
    ULONG payloadLength = pPayload->sizeActualPayload;
    if (payloadLength == 0)
    {
        payloadLength = pPayload->sizePayloadBuffer;
    }

    if ((payloadLength > 0) && (pPayload->payloadData != NULL))
    {

#if !defined(USE_NP_TRANSPORT)
        RtlCopyMemory(pBuffer, pPayload->payloadData, payloadLength);
#else // defined(USE_NP_TRANSPORT)

        // Copy the payload to the pipeBuffer to be used for sending the payload if one is specified
        PVOID pSendBuffer = pPayload->payloadData;
        if ((pipeBuffer != NULL) && (pipeBufferLength > 0) && (pSendBuffer != pipeBuffer))
        {
            RtlZeroMemory(pipeBuffer, pipeBufferLength);
            RtlCopyMemory(pipeBuffer, pSendBuffer, payloadLength);

            // Update the params to reflect the buffer that will be sent across the pipe
            pSendBuffer = pipeBuffer;
            payloadLength = pipeBufferLength;
        }

        IO_STATUS_BLOCK ioStatusBlock = { 0 };
        ULONG remainingLengthToWrite = payloadLength;
        PUCHAR pWriteFrom = pSendBuffer;
        ULONG currentLengthToWrite = remainingLengthToWrite % MAX_BUFFER_SIZE_NAMEDPIPE;
        if (currentLengthToWrite == 0)
        {
            currentLengthToWrite = MAX_BUFFER_SIZE_NAMEDPIPE;
        }

        while (remainingLengthToWrite > 0)
        {
            status = ZwWriteFile(hPipe, NULL, NULL, NULL, &ioStatusBlock, pWriteFrom, currentLengthToWrite, NULL, NULL);
            if (NT_SUCCESS(status))
            {
                ULONG bytesWritten = (ULONG)ioStatusBlock.Information;
                if (currentLengthToWrite != bytesWritten)
                {
                    NT_ASSERT(FALSE);
                    status = GET_RW_ERROR_STATUS((pPayload->mode == Read));
                    break;
                }
                else
                {
                    remainingLengthToWrite -= bytesWritten;
                    currentLengthToWrite = MAX_BUFFER_SIZE_NAMEDPIPE;
                    pWriteFrom += bytesWritten;
                }
            }
            else
            {
                break;
            }
        }

#endif // !defined(USE_NP_TRANSPORT)

    }

    return status;
}

// Helper function to copy the IO buffer to user payload
NTSTATUS CopyPayloadFromIOBuffer(PGATEWAY_PAYLOAD pPayload, PVOID pIOBuffer
#if defined(USE_NP_TRANSPORT)
    , HANDLE hPipe
    , PVOID pipeBuffer
    , ULONG pipeBufferLength
#endif // defined(USE_NP_TRANSPORT)
)
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN IsReadOrWriteRequest = ((pPayload->mode == Read) || (pPayload->mode == Write));
    PGATEWAY_PAYLOAD payloadIncoming = (PGATEWAY_PAYLOAD)pIOBuffer;

    //
    // First, read the header
    //
    pPayload->mode = payloadIncoming->mode;
    pPayload->offsetToRW = payloadIncoming->offsetToRW;
    pPayload->sizePayloadBuffer = payloadIncoming->sizePayloadBuffer;
    pPayload->lengthDeviceID = payloadIncoming->lengthDeviceID;

#if defined(USE_DMA_FOR_SRB_IO)
    pPayload->addressSRB = payloadIncoming->addressSRB;
#endif // defined(USE_DMA_FOR_SRB_IO)

    PUCHAR pIncomingPayloadData = (PUCHAR)&(payloadIncoming->deviceID);

    // Copy the device ID over if it came in and a receiving buffer was specified.
    if (payloadIncoming->lengthDeviceID > 0)
    {
        if (pPayload->deviceID != NULL)
        {
            RtlCopyMemory(pPayload->deviceID, &(payloadIncoming->deviceID), payloadIncoming->lengthDeviceID);
        }

        pIncomingPayloadData += payloadIncoming->lengthDeviceID;
    }

    // Fetch the EndOfPayloadMarker
    ULONG endOfPayloadMarker = 0;
    RtlCopyMemory(&endOfPayloadMarker, pIncomingPayloadData, sizeof(endOfPayloadMarker));

    // Update the Mode to reflect invalid payload if we didn't see the marker.
    if (endOfPayloadMarker != END_OF_PAYLOAD_MARKER)
    {
        pPayload->mode = BLOCK_OPERATION_INVALID_PAYLOAD;
    }
    else
    {
        //
        // Second, get the response from the service.
        //
        BOOLEAN fCopyPayloadToReceipientBuffer = TRUE;
        
        // If we had any failure in non-RW request processing, then we have nothing to copy
        // to the actual target buffer where the payload was expected to be copied to.
        if (!IsReadOrWriteRequest && pPayload->mode != BLOCK_OPERATION_COMPLETED)
        {
            fCopyPayloadToReceipientBuffer = FALSE;
        }

#if defined(USE_SRB_MDL)
        // For Write, the response is only the header that we have captured above.
        // For Read, the data is in the MDL we chained before initiating the read request.
        // Thus, nothing to do here for either of them.
        if (!IsReadOrWriteRequest)
#endif // defined(USE_SRB_MDL)
        {
            // Copy the incoming data, if applicable, since payload looks intact.
            if ((pPayload->payloadData != NULL) && (payloadIncoming->sizePayloadBuffer > 0))
            {
#if !defined(USE_NP_TRANSPORT)
                if (fCopyPayloadToReceipientBuffer)
                {
                    PUCHAR pCopyPayloadFromAddress = pIOBuffer;
                    pCopyPayloadFromAddress += GET_PAYLOAD_HEADER_SIZE;
                    RtlCopyMemory(pPayload->payloadData, pCopyPayloadFromAddress, payloadIncoming->sizePayloadBuffer);
                }
#else // defined(USE_NP_TRANSPORT)

                PVOID pReceiveBuffer = pPayload->payloadData;
                ULONG receiveBufferLength = payloadIncoming->sizePayloadBuffer;
                BOOLEAN fCopyPayloadFromReceiveBufferToPayloadBuffer = FALSE;

                // Read the payload from the pipe into the explicitly specified buffer if requested.
                if ((pipeBuffer != NULL) && (pipeBufferLength > 0))
                {
                    // Use the specified buffer for reading from the pipe
                    pReceiveBuffer = pipeBuffer;
                    receiveBufferLength = pipeBufferLength;
                    RtlZeroMemory(pReceiveBuffer, pipeBufferLength);

                    // Set the flag indicating that we will need to copy the actual payload from the 
                    // explicit buffer to the user specified buffer.
                    fCopyPayloadFromReceiveBufferToPayloadBuffer = TRUE;
                }

                // Read the data off the named pipe in blocks of MAX_BUFFER_SIZE_NAMEDPIPE.
                ULONG remainingLengthToRead = receiveBufferLength;
                PUCHAR pReadTo = pReceiveBuffer;
                BOOLEAN fReadSuccessful = TRUE;
                ULONG currentReadLength = remainingLengthToRead % MAX_BUFFER_SIZE_NAMEDPIPE;
                if (currentReadLength == 0)
                {
                    currentReadLength = MAX_BUFFER_SIZE_NAMEDPIPE;
                }

                while (remainingLengthToRead > 0)
                {
                    IO_STATUS_BLOCK ioStatusBlock = { 0 };
                    status = ZwReadFile(hPipe, NULL, NULL, NULL, &ioStatusBlock, pReadTo, currentReadLength, NULL, NULL);
                    if (NT_SUCCESS(status))
                    {
                        ULONG bytesRead = (ULONG)ioStatusBlock.Information;
                        if (currentReadLength != bytesRead)
                        {
                            NT_ASSERT(FALSE);
                            status = GET_RW_ERROR_STATUS((pPayload->mode == Read));
                            fReadSuccessful = FALSE;
                            break;
                        }
                        else
                        {
                            remainingLengthToRead -= bytesRead;
                            pReadTo += bytesRead;
                            currentReadLength = MAX_BUFFER_SIZE_NAMEDPIPE;
                        }
                    }
                    else
                    {
                        fReadSuccessful = FALSE;
                        break;
                    }
                }

                if (fReadSuccessful && fCopyPayloadToReceipientBuffer)
                {
                    // Copy the received payload to the actual buffer where caller is expecting to find it.
                    if (fCopyPayloadFromReceiveBufferToPayloadBuffer)
                    {
                        RtlCopyMemory(pPayload->payloadData, pReceiveBuffer, payloadIncoming->sizePayloadBuffer);
                    }
                }
#endif // !defined(USE_NP_TRANSPORT)
            }
        }
    }

    return status;
}

NTSTATUS GetNTSTATUSForResponsePayload(BOOLEAN IsReadRequest, PGATEWAY_PAYLOAD pPayload)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (IsReadRequest)
    {
        if (pPayload->mode == BLOCK_OPERATION_COMPLETED)
        {
            if (!(pPayload->sizePayloadBuffer > 0))
            {
                // We couldn't find the requested block in the service. This is equivalent to accessing an initialized block
                // for the first time. Given that the outbuffer is initialized to zero, we will simply move past this block
                // to the next location, implying we read a block that was NULL initialization.
                //
                // In such a case, we have nothing to copy or free.
                Status = STATUS_SUCCESS;
            }
        }
        else if (pPayload->mode == BLOCK_OPERATION_INVALID_PAYLOAD)
        {
            // We got an invalid payload. Treat that
            // as disk operation failure.
            Status = GET_RW_ERROR_STATUS(IsReadRequest);
            NT_ASSERT(FALSE);
        }
        else if (pPayload->mode == BLOCK_OPERATION_INVALID_PAYLOAD_SERVER)
        {
            // Server got an invalid payload. Treat that
            // as disk operation failure.
            Status = GET_RW_ERROR_STATUS(IsReadRequest);
            NT_ASSERT(FALSE);
        }
        else
        {
            // We had an exception processing the block in the service. Treat that
            // as disk operation failure.
            Status = GET_RW_ERROR_STATUS(IsReadRequest);
            NT_ASSERT(pPayload->mode == BLOCK_OPERATION_FAILED);
        }
    }
    else
    {
        if (pPayload->mode == BLOCK_OPERATION_COMPLETED)
        {
            // All good - nothing to do.
        }
        else if (pPayload->mode == BLOCK_OPERATION_INVALID_PAYLOAD)
        {
            // We got an invalid payload. Treat that
            // as disk operation failure.
            Status = GET_RW_ERROR_STATUS(IsReadRequest);
            NT_ASSERT(FALSE);
        }
        else if (pPayload->mode == BLOCK_OPERATION_INVALID_PAYLOAD_SERVER)
        {
            // Server got an invalid payload. Treat that
            // as disk operation failure.
            Status = GET_RW_ERROR_STATUS(IsReadRequest);
            NT_ASSERT(FALSE);
        }
        else
        {
            // We had an exception processing the block in the service. Treat that
            // as disk operation failure.
            Status = GET_RW_ERROR_STATUS(IsReadRequest);
            NT_ASSERT(pPayload->mode == BLOCK_OPERATION_FAILED);
        }
    }

    return Status;
}

void
HandleInvariantFailure()
{
    KeBugCheck(RESOURCE_OWNER_POINTER_INVALID);
}
