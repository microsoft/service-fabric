/*++

Copyright (c) Microsoft Corporation

Module Name:

    RvdLogTestHelpers.h

Abstract:

    Contains various helper support for the RvdLogger unit test.


--*/

#pragma once

#ifdef KMUNIT_FRAMEWORK
#if KTL_USER_MODE
#include <KmUser.h>
#else
#include <KmUnit.h>
#endif
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include <InternalKtlLogger.h>

NTSTATUS
GetLogDriveRootPath(
    __in WCHAR const DriveLetter,
    __out KWString& RootPath);

NTSTATUS
GetLogFilesOnLogDrive(
    __in WCHAR const DriveLetter,
    __out KVolumeNamespace::NameArray& LogFiles);

NTSTATUS
CleanAndPrepLog(
    __in WCHAR const DriveLetter,
    __in BOOLEAN CreateDir = TRUE);

NTSTATUS
GetLogDriveGuid(
    __in WCHAR const DriveLetter,
    __out KGuid& DiskIdGuid);

NTSTATUS
GetLogDriveLetter(
    __in KGuid const& LogDriveGuid,
    __out WCHAR* LogDriveLetter);

NTSTATUS
CheckIfLogExists(
    __in KGuid const& LogId,
    __in KGuid const& LogDriveId);

NTSTATUS
CheckLogAttributes(
    __in RvdLog::SPtr& Log,
    __in KGuid const& LogId,
    __in KGuid const& DiskId,
    __in KWString const& LogType,
    __in LONGLONG LogSize);

NTSTATUS
GetDriveAndPathOfLogFile(
    __in WCHAR DriveLetter,
    __in RvdLogId& LogId,
    __out KGuid& DriveGuid,
    __out KWString& FullyQualifiedDiskFileName);

BOOLEAN
ToULonglong(__in KWString& FromStr, __out ULONGLONG& Result);

BOOLEAN
ToULong(__in KWString& FromStr, __out ULONG& Result);


//
// A simple random number generator wrapper that works in kernel and user mode.
// It can be seeded on the current performance counter, or any specific value.
//
// NOTE:
// - This is NOT a secure random number generator.
// - This is NOT thread-safe.
//
class RandomGenerator
{
public:
    static VOID SetSeed(__in ULONG Seed = GetAutoSeed())
    {
        _Seed = Seed;
    }

    static ULONG GetAutoSeed()
    {
        LARGE_INTEGER li;

    #if KTL_USER_MODE
        QueryPerformanceCounter(&li);
    #else
        KeQueryPerformanceCounter(&li);
    #endif

        return (ULONG)li.LowPart;
    }

    //
    // Return an uniformly distributed random number in [0, MAXULONG]
    //
    static ULONG Get()
    {
        return Get(MAXULONG, 0);
    }

    //
    // Return an uniformly distributed random number in [Min, Max]
    //
    static ULONG Get(__in ULONG Max, __in ULONG Min = 0)
    {
        ASSERT(Max >= Min);

        // BUG, richhas, xxxxxx, theory is RtlRandomEx not really a closed interval. The original code was...
        //                       ULONG x = (ULONG)((ULONGLONG)RtlRandomEx(&_Seed) * (Max - Min) / (MAXLONG - 1) + Min);
        ULONG x = (ULONG)((ULONGLONG)RtlRandomEx(&_Seed) * (Max + 1 - Min) / (MAXLONG - 1) + Min);

        //
        // RtlRandomEx() is not thread safe. The returned value can be out of range.
        //
        if (x < Min)
        {
            x = Min;
        }
        if (x > Max)
        {
            x = Max;
        }

        return x;
    }

private:
    RandomGenerator();
    ~RandomGenerator();

    static ULONG _Seed;
};

class Synchronizer
{
public:
    Synchronizer() :
        _Event(
            FALSE,  // IsManualReset
            FALSE   // InitialState
            ),
        _CompletionStatus(STATUS_SUCCESS)
    {
        _Callback.Bind(this, &Synchronizer::AsyncCompletion);
    }

    ~Synchronizer() {}

    //
    // Use the returned KDelegate as the async completion callback.
    // Then wait for completion using WaitForCompletion().
    //

    KAsyncContextBase::CompletionCallback
    AsyncCompletionCallback()
    {
        return _Callback;
    }

    NTSTATUS
    WaitForCompletion()
    {
        _Event.WaitUntilSet();
        return _CompletionStatus;
    }

private:
    VOID
    AsyncCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        )
    {
        UNREFERENCED_PARAMETER(Parent);

        _CompletionStatus = CompletingOperation.Status();
        _Event.SetEvent();
    }

    KEvent _Event;
    NTSTATUS _CompletionStatus;
    KAsyncContextBase::CompletionCallback _Callback;
};

class CompletionEvent : public KEvent
{
public:
    CompletionEvent(
        __in BOOLEAN IsManualReset = FALSE,
        __in BOOLEAN InitialState = FALSE)
        :   _Status(STATUS_SUCCESS),
            KEvent(IsManualReset, InitialState)
    {
    }

    void EventCallback(
        KAsyncContextBase* const  Parent,
        KAsyncContextBase& Context)
    {
        UNREFERENCED_PARAMETER(Parent);

        _Status = Context.Status();
        SetEvent();
    }

    NTSTATUS
    WaitUntilSet()
    {
        KEvent::WaitUntilSet();
        return(_Status);
    }

    NTSTATUS _Status;
};


//** KBlockDevice interceptor implementation for Log recovery fault injection
//
//   The KInjectorBlockDevice class is aimed at being a general I/O fault injection object that
//   sets between the Log runtime and the real KBlockDevice implementations.
//
//   The current version only basic write failure cases around the region of chaos that is allowed
//   at the tail of the log. In the future, this class wil be expanded to cover additional cases
//   supported simulated I/O faults.
//
class KInjectorBlockDevice : public KBlockDevice
{
    K_FORCE_SHARED(KInjectorBlockDevice);

public:
    // Types of fault injection cases
    enum InjectionConstraint
    {
        UserRecordHeaderOnly = 0,
        UserRecordIoBufferDataOnly = 1,
        CheckpointOnly = 2,
    };

    // Record of a LSN region where a fault was injected.
    struct FaultedRegionDescriptor
    {
        // Types of injected faults
        enum Type
        {
            DroppedWrite,
        };

        Type            _FaultType;
        RvdLogLsn       _StartOfRegion;
        ULONG           _RegionSize;
    };

    // Record of faulted records (carries the first block on the header/metadata for the faulted record)
    struct FaultedRecordHeader : public KShared<FaultedRecordHeader>
    {
        K_FORCE_SHARED(FaultedRecordHeader);

    public:
        RvdLogRecordHeader&                     GetLogRecordHeader();
        BOOLEAN                                 IsUserStreamRecord();
        RvdLogUserStreamRecordHeader&           GetUserStreamRecordHeader();
        BOOLEAN                                 IsCheckpointStreamRecord();
        RvdLogPhysicalCheckpointRecord&         GetCheckpointRecordHeader();
        BOOLEAN                                 IsUserRecord();
        RvdLogUserRecordHeader&                 GetUserRecordHeader();
        BOOLEAN                                 IsUserStreamCheckpointRecord();
        RvdLogUserStreamCheckPointRecordHeader& GetUserStreamCheckpointRecordHeader();

    private:
        friend class KInjectorBlockDevice;
        UCHAR       _Blk[RvdDiskLogConstants::BlockSize];
    };

public:
    FAILABLE
    KInjectorBlockDevice(KBlockDevice::SPtr UnderlyingDevice, RvdLog::SPtr Log);

    // Accessors
    VOID                                SetWriteIntercept(BOOLEAN To);
    BOOLEAN                             GetWriteIntercept();
    VOID                                SetInjectionConstraint(InjectionConstraint To);
    KArray<FaultedRegionDescriptor>&    GetFaultedRegions();
    KArray<FaultedRecordHeader::SPtr>&  GetFaultedRecords();
    KBlockDevice::SPtr                  GetUnderlyingDevice();

    //** KBlockDevice API implementation
    VOID
    QueryDeviceAttributes(__out DeviceAttributes& Attributes);

    VOID
    QueryDeviceHandle(__out HANDLE& Handle);

    NTSTATUS
    AllocateReadContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag);

    NTSTATUS
    AllocateWriteContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag);

    NTSTATUS
    AllocateFlushContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag);

    NTSTATUS
    AllocateTrimContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag);

    NTSTATUS
    AllocateQueryAllocationsContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag);

    NTSTATUS
    AllocateSetSystemIoPriorityHintContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        );

    NTSTATUS
    AllocateNonContiguousReadContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_BLOCK_DEVICE
        );

    NTSTATUS
    ReadNonContiguous(
        __in ULONGLONG Offset,
        __in ULONG ReadLength,
        __in ULONG BufferLength,
        __out KIoBuffer::SPtr& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr,
        __in_opt KArray<KAsyncContextBase::SPtr>* const BlockDeviceReadOps = nullptr
        );

    NTSTATUS
    Read(
        __in ULONGLONG Offset,
        __in ULONG Length,
        __in BOOLEAN ContiguousIoBuffer,
        __out KIoBuffer::SPtr& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync,
        __in_opt KAsyncContextBase::SPtr Async);

    NTSTATUS
    Write(
        __in ULONGLONG Offset,
        __in KIoBuffer& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync,
        __in_opt KAsyncContextBase::SPtr Async);

    NTSTATUS
    Write(
        __in KBlockFile::IoPriority IoPriority,
        __in ULONGLONG Offset,
        __in KIoBuffer& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync,
        __in_opt KAsyncContextBase::SPtr Async);

    NTSTATUS
    Write(
        __in KBlockFile::IoPriority IoPriority,
        __in KBlockFile::SystemIoPriorityHint IoPriorityHint,
        __in ULONGLONG Offset,
        __in KIoBuffer& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync,
        __in_opt KAsyncContextBase::SPtr Async);

    NTSTATUS
    Flush(
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync,
        __in_opt KAsyncContextBase::SPtr Async);

    NTSTATUS
    Trim(
        __in ULONGLONG TrimFrom,
        __in ULONGLONG TrimToPlusOne,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync,
        __in_opt KAsyncContextBase::SPtr Async);

    NTSTATUS
    QueryAllocations(
        __in ULONGLONG QueryFrom,
        __in ULONGLONG Length,
        __inout KArray<KBlockFile::AllocatedRange>& Results,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync,
        __in_opt KAsyncContextBase::SPtr Async);

    NTSTATUS
    SetSystemIoPriorityHint(
        __in KBlockFile::SystemIoPriorityHint Hint,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncContextBase::SPtr Async = nullptr
        );

    NTSTATUS
    SetSparseFile(
        __in BOOLEAN IsSparseFile
        );
    
    VOID
    Close();

private:
    BOOLEAN
    StateMachine(
        __in ULONGLONG Offset,
        __in KIoBuffer& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync,
        __in_opt KAsyncContextBase::SPtr Async);

    VOID
    UnsafeComputeAndStartWrite(
        __in ULONGLONG Offset,
        __in KIoBuffer& IoBuffer,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync,
        __in_opt KAsyncContextBase::SPtr Async);

    VOID
    OnInterceptedWriteComplete(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& CompletingSubOp);

private:
    class WriteOpInceptor : public KAsyncContextInterceptor
    {
        K_FORCE_SHARED(WriteOpInceptor);

    public:
        static NTSTATUS
        Create(ULONG Tag, KAllocator& Allocator, WriteOpInceptor::SPtr& Result);

    private:
        VOID
        OnCompletedIntercepted();
    };

    class InterceptedWriteOp : public KAsyncContextBase
    {
        K_FORCE_SHARED(InterceptedWriteOp);

    public:
        InterceptedWriteOp(KInjectorBlockDevice& Owner);

        VOID
        StartInterceptedWrite(
            __in KAsyncContextBase::SPtr InterceptedWrite,
            __in KAsyncContextBase::CompletionCallback InterceptedWriteCompletion,
            __in_opt KAsyncContextBase* const InterceptedWriteParentAsync,
            __in ULONGLONG Offset0,
            __in KIoBufferView::SPtr WriteView0,
            __in ULONGLONG Offset1,
            __in KIoBufferView::SPtr WriteView1,
            __in KAsyncContextBase::CompletionCallback Completion,
            __in_opt KAsyncContextBase* const ParentAsync);

    private:
        VOID
        OnStart();

        VOID
        OnUnderlyingWriteComplete(
            KAsyncContextBase* const Parent,
            KAsyncContextBase& CompletingSubOp);

        NTSTATUS
        ScheduleUnderlyingWrite(ULONGLONG OffsetOfWrite, KIoBufferView::SPtr& WriteView);

        VOID
        OnCompleted();

    private:
        KInjectorBlockDevice&                   _Owner;
        WriteOpInceptor::SPtr                   _Interceptor;
        ULONG                                   _NumberOfWrites;

        KAsyncContextBase::SPtr                 _InterceptedKAsync;
        KAsyncContextBase::CompletionCallback   _InterceptedWriteCompletion;
        KAsyncContextBase*                      _InterceptedWriteParentAsync;
        ULONGLONG                               _OffsetOfWrite0;
        KIoBufferView::SPtr                     _WriteView0;
        ULONGLONG                               _OffsetOfWrite1;
        KIoBufferView::SPtr                     _WriteView1;
    };

private:
    RvdLogManagerImp::RvdOnDiskLog& _Log;
    KBlockDevice::SPtr              _UnderlyingDevice;
    BOOLEAN                         _IsInterceptWrite;
    InjectionConstraint             _InjectionConstraint;
    ULONG                           _State;
    RvdLogLsn                       _RecordLocationToFault;
    ULONGLONG                       _FileLocationToFault;

    KSpinLock                       _ThisLock;
        KArray<FaultedRegionDescriptor>     _FaultedRegions;
        KArray<FaultedRecordHeader::SPtr>   _FaultedRecords;
};

