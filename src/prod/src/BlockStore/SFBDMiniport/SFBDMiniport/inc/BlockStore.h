#pragma once
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*++

Module Name:

BlockStore.h

Abstract:

This file includes data declarations for Service Fabric BlockStore Device's interaction with the BlockStore Service.

Environment:

Kernel mode only.

--*/

#ifndef _BLOCKSTORE_H_
#define _BLOCKSTORE_H_

#if !defined(USE_BLOCKSTORE_SERVICE_ON_NODE)

#if !defined(USE_NP_TRANSPORT)
#define SERVER_FQDN                         L"EnterServerFQDNHere"
#else // defined(USE_NP_TRANSPORT) 
#define SERVER_FQDN                         L"\\DosDevices\\unc\\EnterServerFQDNHere\\Pipe\\SFBlockStorePipe"
#endif // defined(USE_NP_TRANSPORT)

#else // defined(USE_BLOCKSTORE_SERVICE_ON_NODE)

#if !defined(USE_NP_TRANSPORT)
#define SERVER_FQDN                         L"localhost"
#else // defined(USE_NP_TRANSPORT) 
#define SERVER_FQDN                         L"\\DosDevices\\Pipe\\SFBlockStorePipe"
#endif // defined(USE_NP_TRANSPORT)

#endif // !defined(USE_BLOCKSTORE_SERVICE_ON_NODE)

// Marker to indicate end of payload that is exchanged between the driver and the service
#define END_OF_PAYLOAD_MARKER           0xd0d0d0d0
#define SIZEOF_END_OF_PAYLOAD_MARKER    sizeof(ULONG)

#define TIMEOUT_IN_SECONDS_PROCESS_SRB 15
#define TIMEOUT_IN_SECONDS_SERVICE_CONNECT 5

// Macro to compute relative timeout in seconds for use in KeWaitFor* operations
#define RELATIVE_TIMEOUT_IN_SECONDS(x) (-x * 10 * 1000 * 1000)

// Data structure used by the application to send/receive data.
typedef struct _GATEWAY_PAYLOAD {
    ULONG       mode;               // 0 - Test, 1 - Read, 2 - Write
    ULONG       sizeActualPayload;  // Size of the buffer containing the payload data (as it can be different [and greater] from lengthRW).
    ULONG64     offsetToRW;         // Offset to R/W from/to.
    ULONG       sizePayloadBuffer;  // Length of payload that the service will read when it receives request.
    ULONG       lengthDeviceID;     // Length of the device ID
    ULONG64     addressSRB;         // Address of the SRB corresponding to the request

    // ** THIS IS ALWAYS THE LAST ENTRY OF THE HEADER SENT TO THE SERVICE. Any additional fields must be added (and aligned) before this. ** 
    // ** Also, make sure the fields are 8 byte aligned to ensure compiler does not insert any padding. **
    PVOID       deviceID;           // LU ID for the volume. 
    PVOID       payloadData;        // Pointer to the single-byte buffer that contains the data to be R/W from the device.
} GATEWAY_PAYLOAD, *PGATEWAY_PAYLOAD;

// Type that holds the user-buffer (and associated kernel buffer structures) used for socket I/O
typedef struct {
    PVOID   payloadNPBuffer;
    ULONG   payloadNPBufferSize;
    WSK_BUF SocketBuffer;
} USER_BUFFER, *PUSER_BUFFER;

#define GET_RW_ERROR_STATUS(IsReadRequest)  (IsReadRequest?NTSTATUS_FROM_WIN32(ERROR_READ_FAULT): NTSTATUS_FROM_WIN32(ERROR_WRITE_FAULT))

// Size of LU ID in bytes
#define SIZEOF_LUID_BYTES (sizeof(WCHAR) * MAX_WCHAR_SIZE_LUID)

// Size of the payload that is exchanged between client (this driver) and the service
#define GET_PAYLOAD_HEADER_SIZE (sizeof(GATEWAY_PAYLOAD) + SIZEOF_LUID_BYTES + SIZEOF_END_OF_PAYLOAD_MARKER) 

// Validate the size against the constant that is used in the service as well.
C_ASSERT(GET_PAYLOAD_HEADER_SIZE == 152);

// Size of buffer that named pipe can send over the network
#define MAX_BUFFER_SIZE_NAMEDPIPE (64 * 1024)

#define BLOCK_OPERATION_COMPLETED               0x827
#define BLOCK_OPERATION_FAILED                  0xeee
#define BLOCK_OPERATION_INVALID_PAYLOAD         0xfee
#define BLOCK_OPERATION_INVALID_PAYLOAD_SERVER  0xfef

#define USER_BUFFER_POOL_TAG ((ULONG)'bksw')
#define USER_BUFFER_POOL_TAG_REQUEST ((ULONG)'rksw')

#define STATUS_BLOCK_NOT_FOUND NTSTATUS_FROM_WIN32(ERROR_INVALID_BLOCK)

typedef enum 
{
    Read = 1,
    Write,
    RegisterLU,
    UnregisterLU,
    FetchRegisteredLUList,
    UnmountLU,
    MountLU
} BlockMode;

#define IS_LU_OPERATION(mode) ((mode == RegisterLU) || (mode == UnregisterLU) || (mode == UnmountLU) || (mode == MountLU))

// Name of the Reliable dictionary in which LU registrations are maintained.
#define LU_REGISTER_DEVICE_ID L"LURegistrar"

// Any changes to this structure should also be reflected in
// "FetchRegisteredLUList" in the service.
typedef struct
{
    ULONG lenLUID;

    // TODO: Merge the next 3 fields into one.
    ULONG DiskSize;
    ULONG DiskSizeUnit;
    ULONG Mounted;
    WCHAR wszLUID[MAX_WCHAR_SIZE_LUID];
} LU_REGISTRATION_INFO, *PLU_REGISTRATION_INFO;

typedef struct
{
    ULONG countEntries;
    LU_REGISTRATION_INFO registrationInfo[MAX_LU_REGISTRATIONS]; 
} LU_REGISTRATION_LIST, *PLU_REGISTRATION_LIST;

extern const WSK_CLIENT_DISPATCH g_WskAppDispatch;

// General helper functions

VOID CopyPayloadHeaderToBuffer(PGATEWAY_PAYLOAD pPayload, PUCHAR pBuffer);

NTSTATUS CopyPayloadToBuffer(PGATEWAY_PAYLOAD pPayload
#if !defined(USE_NP_TRANSPORT)
    , PUCHAR pBuffer
#else // defined(USE_NP_TRANSPORT)
    , HANDLE hPipe
    , PVOID pipeBuffer
    , ULONG pipeBufferLength
#endif // !defined(USE_NP_TRANSPORT)
);

NTSTATUS CopyPayloadFromIOBuffer(PGATEWAY_PAYLOAD pPayload, PVOID pIOBuffer
#if defined(USE_NP_TRANSPORT)
    , HANDLE hPipe
    , PVOID pipeBuffer 
    , ULONG pipeBufferLength
#endif // defined(USE_NP_TRANSPORT)
);

NTSTATUS GetNTSTATUSForResponsePayload(BOOLEAN IsReadRequest, PGATEWAY_PAYLOAD pPayload);

#if !defined(USE_NP_TRANSPORT)

typedef struct _RW_REQUEST
{
    // Buffer used for Socket I/O
    USER_BUFFER SocketBuffer;
    BOOLEAN IsSocketBufferAllocated;

    // Irp used for Socket I/O
    PIRP pIrp;
    BOOLEAN IsIrpAllocated;

    // Offset to R/W from
    ULONG64 offset;

    // Length to R/W from the offset
    ULONG lengthToRW;

    // Reference to the Socket being used for communication
    volatile PWSK_SOCKET *ppSocket;

    // Type of Request
    BOOLEAN IsReadRequest;

    // Reference to the LU ID
    PWCHAR pLUID;

    // Offset for which the OS requested the SRB for.
    ULONG64 baseOffset;

    // Event to synchronize subrequests
    PKEVENT pSubrequestsEventsync;

    // Count of active subrequests in progress. When we hit zero on this,
    // eventSyncSubrequests is signalled.
    volatile PLONG pSubrequestsActiveCount;

    // Consolidated status for subrequest processing.
    volatile PNTSTATUS pSubrequestsStatus;

    // Can we close the socket?
    BOOLEAN CanCloseSocket;

    // Should we check for payload consistency?
    BOOLEAN CheckForPayloadConsistency;

#if defined(USE_SRB_MDL)
    // SRB DataBuffer MDL
    PMDL pSrbDataBufferMdl;
#else // !defined(USE_SRB_MDL) 

    // Reference to the memory to R/W to/from
    PVOID pRWBufferAddress;
#endif // defined(USE_SRB_MDL)

#if defined(USE_DMA_FOR_SRB_IO)
    // Reference to the SRB corresponding to the request.
    PSCSI_REQUEST_BLOCK pSrb;
#endif // defined(USE_DMA_FOR_SRB_IO)

} RW_REQUEST, *PRW_REQUEST;

VOID SetEndOfPayloadMarker(
    IN PUSER_BUFFER UserBuffer
);

NTSTATUS
AllocAndInitSyncIrp(
    IN OUT PIRP* IrpForInit,
    IN PKEVENT SyncEvent,
    IN BOOLEAN fReuseIrp
);

NTSTATUS
AllocAndInitIrp(
    IN OUT PIRP* IrpForInit,
    IN BOOLEAN fReuseIrp,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID ContextCompletionRoutine
);

VOID FreeRWRequest(PRW_REQUEST pRWRequest);

PWSK_SOCKET ConnectTo(
    IN pHW_HBA_EXT pHBAExt,
    IN PUNICODE_STRING server,
    IN ULONG servicePort
);

VOID
GetCurrentHostname(
    _In_ PUNICODE_STRING Hostname
);

NTSTATUS ResolveNameToIp(
    IN pHW_HBA_EXT pHBAExt
);

NTSTATUS CacheSecurityContext(
    IN pHW_HBA_EXT pHBAExt
);

NTSTATUS InitSockets(
    IN pHW_HBA_EXT pHBAExt
);

VOID ShutdownSockets(
    IN pHW_HBA_EXT pHBAExt
);

NTSTATUS AllocateSocketBuffer(
    IN OUT PUSER_BUFFER UserBuffer,
    IN ULONG Size
);

NTSTATUS FreeSocketBuffer(
    IN OUT PUSER_BUFFER UserBuffer
);

VOID ResetSocketBuffer(
    IN PUSER_BUFFER UserBuffer
);

NTSTATUS CloseSocket(
    IN PWSK_SOCKET socket
);

NTSTATUS DisconnectSocket(
    IN PWSK_SOCKET socket
);

NTSTATUS SendBuffer(
    IN PWSK_SOCKET socket,
    IN PUSER_BUFFER pSocketBuffer,
    IN PIRP pIrp,
    IN PKEVENT pSyncEvent,
    IN PGATEWAY_PAYLOAD payload
);

NTSTATUS ReceiveBuffer(
    IN PWSK_SOCKET socket,
    IN PUSER_BUFFER pSocketBuffer,
    IN PIRP pIrp,
    IN PKEVENT pSyncEvent,
    IN PGATEWAY_PAYLOAD payload
);

IO_COMPLETION_ROUTINE SendRequestCompletionRoutine;
IO_COMPLETION_ROUTINE ReceiveResponseCompletionRoutine;

NTSTATUS SendRequestAsync(
    IN PRW_REQUEST pRWRequest,
    IN PGATEWAY_PAYLOAD payload
);

NTSTATUS ReceiveResponseAsync(
    IN PRW_REQUEST pRWRequest
);

NTSTATUS ReceiveBlockResponseFromService(PRW_REQUEST pRWRequest);

NTSTATUS ResolveFQDNToIpV4Address(
    IN pHW_HBA_EXT pHBAExt,
    IN PUNICODE_STRING fqdnToResolve,
    IN OUT PSOCKADDR resolvedAddress
);

NTSTATUS SendBlockRequestToService(PRW_REQUEST pRWRequest);

IO_COMPLETION_ROUTINE WorkCompletionRoutine;
IO_WORKITEM_ROUTINE SFBDWorkerfunction;
KSTART_ROUTINE InitWSKOnThread;

#else // defined(USE_NP_TRANSPORT)

NTSTATUS SendBuffer(HANDLE hPipe, PGATEWAY_PAYLOAD pPayload);
NTSTATUS SendBufferEx(HANDLE hPipe, PGATEWAY_PAYLOAD pPayload, PVOID pipeBuffer, ULONG pipeBufferLength);
NTSTATUS ReceiveBuffer(HANDLE hPipe, PGATEWAY_PAYLOAD pPayload);
NTSTATUS ReceiveBufferEx(HANDLE hPipe, PGATEWAY_PAYLOAD pPayload, PVOID pipeBuffer, ULONG pipeBufferLength);
NTSTATUS ConnectTo(PUNICODE_STRING pipeName, PHANDLE phPipe);
NTSTATUS CloseConnection(HANDLE hPipe);

#endif // !defined(USE_NP_TRANSPORT)

// BlockService helpers
NTSTATUS ProcessRWRequests(
    IN ULONG64 offset,
    IN PSCSI_REQUEST_BLOCK pSrb);

NTSTATUS ProcessManagementRequest(
    IN BlockMode registrationMode,
    IN pHW_HBA_EXT pHBAExt,
    IN PWCHAR pLUUniqueID,
    IN ULONG  DiskSize,
    IN UCHAR  DiskSizeUnit,
    IN ULONG  Mounted,
    IN PVOID  pLURegistrationListBuffer,
    IN ULONG  servicePort
);

#endif    // _BLOCKSTORE_H_

