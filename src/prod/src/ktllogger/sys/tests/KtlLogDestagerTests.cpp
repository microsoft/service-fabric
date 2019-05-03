// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <KtlLogger.h>
#include "KtlLogDestagerTests.h"

#include <..\KtlShim\KtlLogDestagerImp.h>

#if (CONSOLE_TEST) || (DISPLAY_ON_CONSOLE)
#undef KDbgPrintf
#define KDbgPrintf(...)     printf(__VA_ARGS__)
#endif

#pragma region FakeLogStreamForDestager implementation

NTSTATUS
FakeLogStreamForDestager::Create(
    __out RvdLogStream::SPtr& Result,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in RvdLogStreamId& LogStreamId,
    __in ULONG MaxWriteLatency
    )
{
    Result = _new(AllocationTag, Allocator) FakeLogStreamForDestager(AllocationTag, LogStreamId, MaxWriteLatency);
    if (!Result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

FakeLogStreamForDestager::FakeLogStreamForDestager(
    __in ULONG AllocationTag,
    __in RvdLogStreamId& LogStreamId,
    __in ULONG MaxWriteLatency
    ) : 
    _AllocationTag(AllocationTag),
    _LogStreamId(LogStreamId),
    _MaxLogWriteLatency(MaxWriteLatency)
{
}

FakeLogStreamForDestager::~FakeLogStreamForDestager()
{
}

NTSTATUS
FakeLogStreamForDestager::CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context)
{
    // Generate a random latency value for this write
    Context = _new(_AllocationTag, GetThisAllocator()) AsyncWriteContextImp(_MaxLogWriteLatency);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return Context->Status();
}

FakeLogStreamForDestager::AsyncWriteContextImp::AsyncWriteContextImp(
    __in ULONG MaxLogWriteLatency
    ) : _MaxLogWriteLatency(MaxLogWriteLatency)
{
    NTSTATUS status = KTimer::Create(_DelayTimer, GetThisAllocator(), KTL_TAG_TEST);
    KInvariant(NT_SUCCESS(status));
}

FakeLogStreamForDestager::AsyncWriteContextImp::~AsyncWriteContextImp()
{
}

VOID
FakeLogStreamForDestager::AsyncWriteContextImp::StartWrite(
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) 
{
    UNREFERENCED_PARAMETER(RecordAsn);
    UNREFERENCED_PARAMETER(Version);
    UNREFERENCED_PARAMETER(MetaDataBuffer);
    UNREFERENCED_PARAMETER(IoBuffer);

    Start(ParentAsyncContext, CallbackPtr);        
}

VOID
FakeLogStreamForDestager::AsyncWriteContextImp::OnStart()
{
    // Generate a random latency value for this write.
    // Arm the timer to simulate timing aspect of this log write.
    ULONG latency = (ULONG)KNt::GetPerformanceTime() % _MaxLogWriteLatency;
    _DelayTimer->StartTimer(latency, this, nullptr);

    // This async is the parent of the timer. So its completion will be held back until the timer completes.
    Complete(STATUS_SUCCESS);
}

VOID
FakeLogStreamForDestager::AsyncWriteContextImp::OnReuse()
{
    _DelayTimer->Reuse();
}

#pragma endregion

VOID 
PopulateTestRecord(
    __in LogRecordContainer& RecordContainer,
    __in ULONG StartingAsn,
    __in ULONG Count,
    __in KtlLogDestager& Destager
    )
{
    NTSTATUS status;

    for (ULONG i = 0; i < Count; i++)
    {
        ULONG metaDataBufferSize = (i + 1) * 100;
        ULONG ioBufferSize = 4096;

        RvdLogAsn recordAsn = StartingAsn + i * 2;  // Leave holes in Asn space so that we can test ReadContaining

        TestLogRecord::SPtr logRecord = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) TestLogRecord(recordAsn, 1, metaDataBufferSize, ioBufferSize);

        RecordContainer.AddLogRecord(*logRecord);

        KtlLogDestager::AsyncWriteContext::SPtr writeContext;
        status = Destager.CreateAsyncWriteContext(writeContext);
        KInvariant(NT_SUCCESS(status));

        writeContext->StartWrite(
            logRecord->GetAsn(), logRecord->GetVersion(), logRecord->GetMetaDataBuffer(), logRecord->GetIoBuffer(), 
            nullptr,    // ResourceCounter
            nullptr,    // Parent
            logRecord->GetWriteCallback()
            );
    }
}

NTSTATUS
LogDestagerBasicTest(__in int argc, __in_ecount(argc) WCHAR* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;

    // create a fake log stream for testing
    KGuid logStreamId;
    logStreamId.CreateNew();

    RvdLogStream::SPtr logStream;
    const ULONG maxWriteLatency = 500;

    status = FakeLogStreamForDestager::Create(logStream, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST, RvdLogStreamId(logStreamId), maxWriteLatency);

    // create a destager
    KtlLogDestager::SPtr destager;
    status = KtlLogDestagerImp::Create(destager, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST, *logStream);
    KInvariant(NT_SUCCESS(status));

    LogRecordContainer recordContainer;
    ULONG recordCount = 0;

    KtlLogDestager::ResourceCounter resourceCounter;

    // Basic enqueue test
    {
        // Put some log records into the destager
        recordCount = 32;
        PopulateTestRecord(recordContainer, 2, recordCount, *destager);

        // Wait for a little while to let the destager queue state settled.
        // There are async context activities happening in the background.
        KNt::Sleep(2000);

        // We have not trigger any flush yet. Therefore all log writes should still be queued.
        KInvariant(recordContainer._RecordList.Count() == recordCount);

        destager->QueryResourceCounter(resourceCounter);

        KInvariant(resourceCounter.LocalMemoryUsage == recordContainer._TotalMemory);
        KInvariant(resourceCounter.PendingWriteCount == recordContainer._RecordList.Count());
    }

#pragma region Read API tests

    // Exact read
    {
        KBuffer::SPtr metaDataBuffer;
        KIoBuffer::SPtr ioBuffer;
        ULONGLONG version;
        RvdLogAsn asn;

        // Read an existing record
        asn = 2;
        status = destager->Read(asn, &version, metaDataBuffer, ioBuffer);
        KInvariant(NT_SUCCESS(status));

        BOOLEAN b = TestLogRecord::Verify(asn, version, metaDataBuffer, ioBuffer);
        KInvariant(b);

        // Read a non-existing record
        asn = 1;
        status = destager->Read(asn, &version, metaDataBuffer, ioBuffer);
        KInvariant(!NT_SUCCESS(status));
    }

    // ReadPrevious
    {
        KBuffer::SPtr metaDataBuffer;
        KIoBuffer::SPtr ioBuffer;
        ULONGLONG version;
        RvdLogAsn asn;
        RvdLogAsn outAsn;

        // Read an existing record
        asn = 4;
        status = destager->ReadPrevious(asn, &outAsn, &version, metaDataBuffer, ioBuffer);
        KInvariant(NT_SUCCESS(status));

        BOOLEAN b = TestLogRecord::Verify(outAsn, version, metaDataBuffer, ioBuffer);
        KInvariant(b);

        // Read a non-existing record
        asn = 1;
        status = destager->ReadPrevious(asn, &outAsn, &version, metaDataBuffer, ioBuffer);
        KInvariant(!NT_SUCCESS(status));

        asn = 2;
        status = destager->ReadPrevious(asn, &outAsn, &version, metaDataBuffer, ioBuffer);
        KInvariant(!NT_SUCCESS(status));
    }

    // ReadNext
    {
        KBuffer::SPtr metaDataBuffer;
        KIoBuffer::SPtr ioBuffer;
        ULONGLONG version;
        RvdLogAsn asn;
        RvdLogAsn outAsn;

        // Read an existing record
        asn = 2;
        status = destager->ReadNext(asn, &outAsn, &version, metaDataBuffer, ioBuffer);
        KInvariant(NT_SUCCESS(status));

        BOOLEAN b = TestLogRecord::Verify(outAsn, version, metaDataBuffer, ioBuffer);
        KInvariant(b);

        // Read a non-existing record
        asn = 1;
        status = destager->ReadNext(asn, &outAsn, &version, metaDataBuffer, ioBuffer);
        KInvariant(!NT_SUCCESS(status));

        asn = recordCount * 2;  // This is the highest Asn; there's no next to it
        status = destager->ReadNext(asn, &outAsn, &version, metaDataBuffer, ioBuffer);
        KInvariant(!NT_SUCCESS(status));
    }

    // ReadContaining
    {
        KBuffer::SPtr metaDataBuffer;
        KIoBuffer::SPtr ioBuffer;
        ULONGLONG version;
        RvdLogAsn asn;
        RvdLogAsn outAsn;

        // Read an existing record
        asn = 2;
        status = destager->ReadContaining(asn, &outAsn, &version, metaDataBuffer, ioBuffer);
        KInvariant(NT_SUCCESS(status));
        KInvariant(outAsn == 2);

        BOOLEAN b = TestLogRecord::Verify(outAsn, version, metaDataBuffer, ioBuffer);
        KInvariant(b);

        asn = 3;
        status = destager->ReadContaining(asn, &outAsn, &version, metaDataBuffer, ioBuffer);
        KInvariant(NT_SUCCESS(status));
        KInvariant(outAsn == 2);

        b = TestLogRecord::Verify(outAsn, version, metaDataBuffer, ioBuffer);
        KInvariant(b);

        // Read a non-existing record
        asn = 1;    // This is the lowest Asn; there's no containing record
        status = destager->ReadNext(asn, &outAsn, &version, metaDataBuffer, ioBuffer);
        KInvariant(!NT_SUCCESS(status));
    }

#pragma endregion

#pragma region Flush tests

    {
        // trigger a flush up to the lower half of Asn's.
        // Asn's are 2x of recordCount.
        RvdLogAsn flushToAsn(recordCount);
        destager->FlushByAsn(flushToAsn);

        // Wait until the flushed records are 'written' to the log. 
        // Since we know the max write latency, we know how long to wait.
        // Add a fudge factor to account for thread sheduling.
        KNt::Sleep(maxWriteLatency + 100);

        // Check if these records are written successfully.
        recordContainer.CleanFlushedRecord(TRUE, TRUE);
    }

    // trigger a flush based on memory
    {
        ULONG targetMemory = recordContainer._TotalMemory/2;
        destager->FlushByMemory(targetMemory);

        // Wait until the flushed records are 'written' to the log. 
        // Since we know the max write latency, we know how long to wait.
        // Add a fudge factor to account for thread sheduling.
        KNt::Sleep(maxWriteLatency + 100);

        // Clean up log records that have been flushed
        recordContainer.CleanFlushedRecord(TRUE, TRUE);

        // The remaining memory and count should match
        KInvariant(recordContainer._TotalMemory <= targetMemory);

        destager->QueryResourceCounter(resourceCounter);
        KInvariant(resourceCounter.LocalMemoryUsage <= targetMemory);
    }

    // Flush by time test
    {
        ULONG flushTimeThreashold = 3 * 1000;   // 3 sec

        // wait until the flush time threshold has passed.
        KNt::Sleep(flushTimeThreashold + 1000);       

        // Add a fresh log record, which should survive the flush.
        PopulateTestRecord(recordContainer, 100, 1, *destager);

        // Immediately trigger a flush. Everything should be older than flushTimeThreashold except for the newest one.
        destager->FlushByTime(flushTimeThreashold);

        // Wait until the flushed records are 'written' to the log. 
        // Since we know the max write latency, we know how long to wait.
        // Add a fudge factor to account for thread sheduling.
        KNt::Sleep(maxWriteLatency + 100);

        // Clean up log records that have been flushed
        recordContainer.CleanFlushedRecord(TRUE, TRUE);

        // Only the newest record will stay
        destager->QueryResourceCounter(resourceCounter);
        KInvariant(resourceCounter.PendingWriteCount == 1);
    }

    // Purge test
    {
        // Purge eveything
        destager->PurgeQueuedWrites(MAXULONG64);

        // Wait until the flushed records are 'written' to the log. 
        // Since we know the max write latency, we know how long to wait.
        // Add a fudge factor to account for thread sheduling.
        KNt::Sleep(maxWriteLatency + 100);

        // Clean up log records that have been flushed
        recordContainer.CleanFlushedRecord(TRUE, FALSE);

        // Nothing should stay
        destager->QueryResourceCounter(resourceCounter);
        KInvariant(resourceCounter.PendingWriteCount == 0);

        // Repopulate log records
        PopulateTestRecord(recordContainer, 2, 16, *destager);

        // Purge half of them
        RvdLogAsn purgePoint = 16;
        destager->PurgeQueuedWrites(purgePoint);

        // Wait until the flushed records are 'written' to the log. 
        // Since we know the max write latency, we know how long to wait.
        // Add a fudge factor to account for thread sheduling.
        KNt::Sleep(maxWriteLatency + 100);

        // Clean up log records that have been flushed
        recordContainer.CleanFlushedRecord(TRUE, FALSE);

        // Half of records should stay
        KBuffer::SPtr metaDataBuffer;
        KIoBuffer::SPtr ioBuffer;
        ULONGLONG version;
        RvdLogAsn asn;

        for (asn = 2; asn < 2 + 16 * 2; asn.Set(asn.Get() + 2))
        {
            status = destager->Read(asn, &version, metaDataBuffer, ioBuffer);
            if (asn <= purgePoint)
            {
                KInvariant(!NT_SUCCESS(status));
            }
            else
            {
                KInvariant(NT_SUCCESS(status));
            }
        }
    }

    // Graceful close test
    {
        // First purge eveything in the queue
        destager->PurgeQueuedWrites(MAXULONG64);

        // Wait until the flushed records are 'written' to the log. 
        // Since we know the max write latency, we know how long to wait.
        // Add a fudge factor to account for thread sheduling.
        KNt::Sleep(maxWriteLatency + 100);

        // Clean up log records that have been flushed
        recordContainer.CleanFlushedRecord(TRUE, FALSE);

        // Repopulate log records
        PopulateTestRecord(recordContainer, 2, 16, *destager);

        KtlLogDestager::AsyncCloseContext::SPtr closeContext;
        status = destager->CreateAsyncCloseContext(closeContext);
        KInvariant(NT_SUCCESS(status));

        // Close the destager
        KSynchronizer sync;
        closeContext->StartClose(FALSE, nullptr, sync);

        status = sync.WaitForCompletion();
        KInvariant(NT_SUCCESS(status));

        // Clean up log records that have been flushed
        recordContainer.CleanFlushedRecord(FALSE, TRUE);

        // Nothing should stay
        destager->QueryResourceCounter(resourceCounter);
        KInvariant(resourceCounter.PendingWriteCount == 0);

        // The destager must have released its ref to the log stream
        KInvariant(logStream->IsUnique());
    }

#pragma endregion

    // all I/O should have completed
    KInvariant(destager->IsUnique());
    
    return status;
}

#pragma region TestLogRecord implementation

TestLogRecord::TestLogRecord(
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in ULONG MetaDataBufferSize,
    __in ULONG IoBufferSize
    )
{
    _RecordAsn = RecordAsn;
    _Version = Version;

    NTSTATUS status = CreateBuffer(RecordAsn, Version, MetaDataBufferSize, IoBufferSize, _MetaDataBuffer, _IoBuffer);
    KInvariant(NT_SUCCESS(status));

    _MemoryUsage = MetaDataBufferSize + IoBufferSize;

    _WriteCompletionStatus = STATUS_PENDING;
}

TestLogRecord::~TestLogRecord()
{
}

NTSTATUS
TestLogRecord::CreateBuffer(
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in ULONG MetaDataBufferSize,
    __in ULONG IoBufferSize,
    __out KBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    FillPattern* pattern = nullptr;

    if (MetaDataBufferSize > 0)
    {
        status = KBuffer::Create(MetaDataBufferSize, MetaDataBuffer, KtlSystem::GlobalNonPagedAllocator());
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        MetaDataBuffer->Zero();
        if (MetaDataBuffer->QuerySize() >= sizeof(FillPattern))
        {
            FillPattern* pattern = (FillPattern*)MetaDataBuffer->GetBuffer();
            pattern->RecordAsn = RecordAsn;
            pattern->Version = Version;
        }
    }
    else
    {
        MetaDataBuffer.Reset();
    }

    if (IoBufferSize > 0)
    {
        PVOID buffer = nullptr;
        status = KIoBuffer::CreateSimple(IoBufferSize, IoBuffer, buffer, KtlSystem::GlobalNonPagedAllocator());
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        RtlZeroMemory(buffer, IoBufferSize);

        pattern = (FillPattern*)buffer;
        pattern->RecordAsn = RecordAsn;
        pattern->Version = Version;
    }
    else
    {
        IoBuffer.Reset();
    }

    return status;
}

BOOLEAN 
TestLogRecord::Verify(
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in KBuffer::SPtr MetaDataBuffer,
    __in KIoBuffer::SPtr IoBuffer
    )
{
    FillPattern* pattern = nullptr;

    if (MetaDataBuffer && MetaDataBuffer->QuerySize() >= sizeof(FillPattern))
    {
        pattern = (FillPattern*)MetaDataBuffer->GetBuffer();
        if (pattern->RecordAsn != RecordAsn) return FALSE;
        if (pattern->Version != Version) return FALSE;
    }

    if (IoBuffer && IoBuffer->QuerySize())
    {
        pattern = (FillPattern*)IoBuffer->First()->GetBuffer();
        if (pattern->RecordAsn != RecordAsn) return FALSE;
        if (pattern->Version != Version) return FALSE;
    }

    return TRUE;
}

VOID
TestLogRecord::LogStreamWriteCallback(
    __in KAsyncContextBase* const Parent, 
    __in KAsyncContextBase& CompletingSubOp
    )
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletingSubOp.Status();
    _WriteCompletionStatus = status;
}


#pragma endregion

#pragma region LogRecordContainer implementation

VOID 
LogRecordContainer::AddLogRecord(
    __in TestLogRecord& Record
    )
{
    _RecordList.AppendTail(&Record);
    _TotalMemory += Record.GetMemoryUsage();
}

VOID 
LogRecordContainer::RemoveLogRecord(
    __in TestLogRecord& Record
    )
{
    _RecordList.Remove(&Record);
    _TotalMemory -= Record.GetMemoryUsage();
}

VOID 
LogRecordContainer::CleanFlushedRecord(
    __in BOOLEAN CheckStatus,
    __in BOOLEAN ExpectSuccess
    )
{
    TestLogRecord::SPtr record = _RecordList.PeekHead();
    while (record)
    {
        NTSTATUS status = record->GetWriteStatus();
        if (status != STATUS_PENDING)
        {
            // This log record has been written to the log
            if (CheckStatus)
            {
                if (ExpectSuccess)
                {
                    KInvariant(NT_SUCCESS(status));
                }
                else
                {
                    KInvariant(!NT_SUCCESS(status));
                }
            }

            TestLogRecord::SPtr temp = record;
            record = _RecordList.Successor(record.RawPtr());

            RemoveLogRecord(*temp);
        }
        else
        {
            record = _RecordList.Successor(record.RawPtr());
        }
    }
}

#pragma endregion

VOID
DestagerTest(
    KGuid diskId,
    KtlLogManager::SPtr logManager
    )
{
    UNREFERENCED_PARAMETER(diskId);
    UNREFERENCED_PARAMETER(logManager);

    NTSTATUS status = STATUS_SUCCESS;

    status = LogDestagerBasicTest(0, nullptr);
    if (!NT_SUCCESS(status))
    {
        KDbgPrintf("LogDestagerTests: LogDestagerBasicTest() failed: %i\n", __LINE__);
        return;
    }
}
