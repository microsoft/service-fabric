// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <intrin.h>
#include <ktl.h>
#include <ktrace.h>
#include <KtlLogger.h>
#include <KtlPhysicalLog.h>

//
// This is an in-memory RvdLogStream implementation for KtlLogDestager testing.
// The destager only writes to the destaged RvdLogStream object but doesn't do anything else.
// So this implementation only needs to fake log write completions.
//

class FakeLogStreamForDestager : public RvdLogStream
{
    K_FORCE_SHARED(FakeLogStreamForDestager);

public:

    static NTSTATUS
    Create(
        __out RvdLogStream::SPtr& Result,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __in RvdLogStreamId& LogStreamId,
        __in ULONG MaxWriteLatency
        );

    virtual VOID
    QueryLogStreamType(__out RvdLogStreamType& LogStreamType) override
    {
        UNREFERENCED_PARAMETER(LogStreamType);
        KInvariant(NT_SUCCESS(STATUS_NOT_IMPLEMENTED));
    }

    virtual VOID
    QueryLogStreamId(__out RvdLogStreamId& LogStreamId) override
    {
        LogStreamId = _LogStreamId;
    }

    virtual NTSTATUS
    QueryRecordRange(
        __out_opt RvdLogAsn* const LowestAsn,
        __out_opt RvdLogAsn* const HighestAsn,
        __out_opt RvdLogAsn* const LogTruncationAsn) override
    {
        UNREFERENCED_PARAMETER(LowestAsn);
        UNREFERENCED_PARAMETER(HighestAsn);
        UNREFERENCED_PARAMETER(LogTruncationAsn);

        return STATUS_NOT_IMPLEMENTED;
    }

    // This is the only method we need to implement for testing the destager
    virtual NTSTATUS
    CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context) override;

    virtual NTSTATUS
    CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context) override
    {
        UNREFERENCED_PARAMETER(Context);

        return STATUS_NOT_IMPLEMENTED;
    }

    virtual NTSTATUS
    QueryRecord(
        __in RvdLogAsn RecordAsn,
        __out_opt ULONGLONG* const Version,
        __out_opt RecordDisposition* const Disposition,
        __out_opt ULONG* const IoBufferSize = nullptr,
        __out_opt ULONGLONG* const DebugInfo1 = nullptr) override
    {
        UNREFERENCED_PARAMETER(RecordAsn);
        UNREFERENCED_PARAMETER(Version);
        UNREFERENCED_PARAMETER(Disposition);
        UNREFERENCED_PARAMETER(IoBufferSize);
        UNREFERENCED_PARAMETER(DebugInfo1);

        return STATUS_NOT_IMPLEMENTED;
    }

    virtual NTSTATUS
    QueryRecords(
        __in RvdLogAsn LowestAsn,
        __in RvdLogAsn HighestAsn,
        __in KArray<RecordMetadata>& ResultsArray) override
    {
        UNREFERENCED_PARAMETER(LowestAsn);
        UNREFERENCED_PARAMETER(HighestAsn);
        UNREFERENCED_PARAMETER(ResultsArray);

        return STATUS_NOT_IMPLEMENTED;
    }

    virtual VOID
    Truncate(__in RvdLogAsn TruncationPoint, __in RvdLogAsn PreferredTruncationPoint) override
    {
        UNREFERENCED_PARAMETER(TruncationPoint);
        UNREFERENCED_PARAMETER(PreferredTruncationPoint);

        KInvariant(NT_SUCCESS(STATUS_NOT_IMPLEMENTED));
    }

    virtual NTSTATUS
    CreateUpdateReservationContext(__out AsyncReservationContext::SPtr& Context) override
	{
		UNREFERENCED_PARAMETER(Context);

		return STATUS_NOT_IMPLEMENTED;
	}
	
    virtual ULONGLONG QueryCurrentReservation() override
	{
		return 0;
	}
	
private:

    class AsyncWriteContextImp;

    FakeLogStreamForDestager(
        __in ULONG AllocationTag,
        __in RvdLogStreamId& LogStreamId,
        __in ULONG MaxWriteLatency
        );

    const RvdLogStreamId _LogStreamId;
    const ULONG _AllocationTag;

    // This fake log stream doesn't persist log records anywhere, either in memory or on disk, 
    // since they will never be read back. It simply delays log writes randomly to simulate the timing behavior.
    // This value specifies the upper bound of the delay in milliseconds
    const ULONG _MaxLogWriteLatency;
};

class FakeLogStreamForDestager::AsyncWriteContextImp : public RvdLogStream::AsyncWriteContext
{
    K_FORCE_SHARED(AsyncWriteContextImp);
public:

    AsyncWriteContextImp(
        __in ULONG MaxLogWriteLatency
        );

    virtual VOID
    StartWrite(
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version,
        __in const KBuffer::SPtr& MetaDataBuffer,
        __in const KIoBuffer::SPtr& IoBuffer,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

	virtual VOID
	StartReservedWrite(
		__in ULONGLONG ReserveToUse,
		__in RvdLogAsn RecordAsn,
		__in ULONGLONG Version,
		__in const KBuffer::SPtr& MetaDataBuffer,
		__in const KIoBuffer::SPtr& IoBuffer,
		__in_opt KAsyncContextBase* const ParentAsyncContext,
		__in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
	{
		UNREFERENCED_PARAMETER(ReserveToUse);
		UNREFERENCED_PARAMETER(RecordAsn);
		UNREFERENCED_PARAMETER(Version);
		UNREFERENCED_PARAMETER(MetaDataBuffer);
		UNREFERENCED_PARAMETER(IoBuffer);
		UNREFERENCED_PARAMETER(ParentAsyncContext);
		UNREFERENCED_PARAMETER(CallbackPtr);
	}
	
private:

    VOID OnStart() override;
    VOID OnReuse() override;

    const ULONG _MaxLogWriteLatency;
    KTimer::SPtr _DelayTimer;
};

struct LogRecordContainer;

class TestLogRecord : public KObject<TestLogRecord>, public KShared<TestLogRecord>
{
    K_FORCE_SHARED(TestLogRecord);

public:

    TestLogRecord(
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version,
        __in ULONG MetaDataBufferSize,
        __in ULONG IoBufferSize
        );

    RvdLogAsn GetAsn()
    {
        return _RecordAsn;
    }

    ULONGLONG GetVersion()
    {
        return _Version;
    }

    KBuffer::SPtr GetMetaDataBuffer()
    {
        return _MetaDataBuffer;
    }

    KIoBuffer::SPtr GetIoBuffer()
    {
        return _IoBuffer;
    }

    ULONG GetMemoryUsage()
    {
        return _MemoryUsage;
    }

    NTSTATUS GetWriteStatus()
    {
        return _WriteCompletionStatus;
    }

    KAsyncContextBase::CompletionCallback GetWriteCallback()
    {
        return KAsyncContextBase::CompletionCallback(this, &TestLogRecord::LogStreamWriteCallback);
    }

    static NTSTATUS
    CreateBuffer(
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version,
        __in ULONG MetaDataSize,
        __in ULONG IoBufferSize,
        __out KBuffer::SPtr& MetaDataBuffer,
        __out KIoBuffer::SPtr& IoBuffer
        );

    static BOOLEAN 
    Verify(
        __in RvdLogAsn RecordAsn,
        __in ULONGLONG Version,
        __in KBuffer::SPtr MetaDataBuffer,
        __in KIoBuffer::SPtr IoBuffer
        );

    KListEntry ListEntry;

private:

    struct FillPattern
    {
        RvdLogAsn RecordAsn;
        ULONGLONG Version;
    };

    VOID
    LogStreamWriteCallback(
        __in KAsyncContextBase* const Parent, 
        __in KAsyncContextBase& CompletingSubOp
        );

    RvdLogAsn _RecordAsn;
    ULONGLONG _Version;

    KBuffer::SPtr _MetaDataBuffer;
    KIoBuffer::SPtr _IoBuffer;

    ULONG _MemoryUsage;

    NTSTATUS _WriteCompletionStatus;
};

struct LogRecordContainer
{
    LogRecordContainer() : _RecordList(FIELD_OFFSET(TestLogRecord, ListEntry))
    {
        _TotalMemory = 0;
    }

    ~LogRecordContainer()
    {
    }

    VOID 
    AddLogRecord(
        __in TestLogRecord& Record
        );

    VOID 
    RemoveLogRecord(
        __in TestLogRecord& Record
        );

    VOID 
    CleanFlushedRecord(
        __in BOOLEAN CheckStatus,
        __in BOOLEAN ExpectSuccess
        );

    KNodeListShared<TestLogRecord> _RecordList;
    ULONG _TotalMemory;
};
