// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

typedef struct 
{
    TestIPC::TestIPCResult Results;
    PVOID Context;
    ULONGLONG AverageWriteLatencyInMs;
    ULONGLONG HighestWriteLatencyInMs;
	ULONGLONG TotalBytesWritten;
} LLWORKLOADSHAREDINFO;

NTSTATUS StartLLWorkload(    
    __in KtlLogManager::SPtr logManager,
    __in KGuid DiskId,
    __in KtlLogContainerId LogContainerId,
    __in ULONG TestDurationInSeconds,
    __in ULONG MaxRecordsWrittenPerCycle,
    __in ULONGLONG StreamSize,
    __in ULONG MaxWriteLatencyInMs,
    __in ULONG MaxAverageWriteLatencyInMs,
    __in ULONG DelayBetweenWritesInMs,
    __out LLWORKLOADSHAREDINFO* llWorkloadInfo
    );

