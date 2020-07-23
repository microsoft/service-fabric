/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLogStream.AsyncWriteStream.cpp

    Description:
      RvdLogStream::AsyncWriteStream implementation.

    History:
      PengLi/Richhas        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"

#include "ktrace.h"

// BUG, richhas, xxxxx, todo: fix buffer management around record formation and split writes at EOF
// BUG, richhas, 308, consider: creating a common base class and then four derivations: user, phy cp, log cp, truncate, log stream cp, force cp...
// BUG, richhas, xxxxx, add detection for STATUS_DELETE_PENDING

//*** RvdLogStream::AsyncWriteContext base class ctor and dtor.
//
RvdLogStream::AsyncWriteContext::AsyncWriteContext()
{
}

RvdLogStream::AsyncWriteContext::~AsyncWriteContext()
{
}

//*** RvdLogStreamImp::AsyncWriteStream imp

//* UserWriteOp form of ctor
RvdLogStreamImp::AsyncWriteStream::AsyncWriteStream(RvdLogStreamImp* const Owner)
    :   _Type(AsyncWriteStream::Type::UserWriteOp),
        _OwningStream(Owner),
        _AsnIndex(&Owner->_LogStreamDescriptor.AsnIndex),
        _StreamDesc(Owner->_LogStreamDescriptor),
        _StreamInfo(Owner->_LogStreamDescriptor.Info),
        _QuantaAcquired(0),
        _CurrentLogSize(nullptr),
        _CurrentLogSpaceRemaining(nullptr)
{
    UserWriteOpCtorInit(*Owner->_Log);
}

RvdLogStreamImp::AsyncWriteStream::AsyncWriteStream(
    __in RvdLogManagerImp::RvdOnDiskLog& Log,
    __in LogStreamDescriptor& ForStreamDesc)
    :   _Type(AsyncWriteStream::Type::UserWriteOp),
        _AsnIndex(nullptr),
        _StreamDesc(ForStreamDesc),
        _StreamInfo(ForStreamDesc.Info),
        _QuantaAcquired(0),
        _CurrentLogSize(nullptr),
        _CurrentLogSpaceRemaining(nullptr)
{
    UserWriteOpCtorInit(Log);
}

VOID
RvdLogStreamImp::AsyncWriteStream::UserWriteOpCtorInit(__in RvdLogManagerImp::RvdOnDiskLog& Log)
{
    _LowPriorityIO = FALSE;
    _AsyncActivitiesLeft = 0;
    _Log = &Log;

    _SubCheckpointWriteCompletion = KAsyncContextBase::CompletionCallback(&AsyncWriteStream::SubCheckpointWriteCompletion);
    _ForcedCheckpointWriteCompletion = KAsyncContextBase::CompletionCallback(
        this,
        &AsyncWriteStream::ForcedCheckpointWriteCompletion);

    NTSTATUS status = CommonCtorInit(*_Log);
    if (NT_SUCCESS(status))
    {
        status = KIoBuffer::CreateEmpty(_CombinedUserRecordIoBuffer, GetThisAllocator(), KTL_TAG_LOGGER);
    }
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
    }

    _InstrumentedOperation.SetInstrumentedComponent(*(_StreamDesc._InstrumentedComponent));         
}


//* PhysicalCheckPointWriteOp form of ctor
RvdLogStreamImp::AsyncWriteStream::AsyncWriteStream(__in RvdLogManagerImp::RvdOnDiskLog& Log)
    :   _Type(AsyncWriteStream::Type::PhysicalCheckPointWriteOp),
        _AsyncActivitiesLeft(0),
        _Log(nullptr),                              // bound to log only while an op is active
        _StreamDesc(*(Log._CheckPointStream)),
        _StreamInfo(_StreamDesc.Info),
        _AsnIndex(nullptr),
        _PhysicalCheckpointRecord(nullptr),
        _QuantaAcquired(0),
        _CurrentLogSize(nullptr),
        _CurrentLogSpaceRemaining(nullptr)
{
    NTSTATUS status = CommonCtorInit(Log);
    if (NT_SUCCESS(status))
    {
        status = CommonCheckpointCtorInit();
    }
    if (NT_SUCCESS(status))
    {
        status = KIoBufferElement::CreateNew(
            RvdLogPhysicalCheckpointRecord::ComputeMaxSizeOnDisk(Log._Config),
            _PhysicalCheckpointRecordBuffer,
            (void*&)_PhysicalCheckpointRecord,
            GetThisAllocator(),
            KTL_TAG_LOGGER);
    }

    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
    }
}

//* LogicalCheckPointWriteOp, LogicalCheckPointSegmentWriteOp, and TruncateWriteOp and form of ctor
RvdLogStreamImp::AsyncWriteStream::AsyncWriteStream(
    __in RvdLogManagerImp::RvdOnDiskLog& Log,
    __in LogStreamDescriptor& StreamDesc,
    __in AsyncWriteStream::Type InstanceType)
    :   _Type(InstanceType),
        _AsyncActivitiesLeft(0),
        _Log(nullptr),                              // bound to log only while an op is active
        _StreamDesc(StreamDesc),
        _StreamInfo(_StreamDesc.Info),
        _AsnIndex(nullptr),
        _QuantaAcquired(0),
        _IsInputOpQueueSuspended(FALSE),
        _ForceStreamCheckpoint(FALSE),
        _CurrentLogSize(nullptr),
        _CurrentLogSpaceRemaining(nullptr)
{
    KInvariant((InstanceType == AsyncWriteStream::Type::LogicalCheckPointWriteOp) ||
               (InstanceType == AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp) ||
               (InstanceType == AsyncWriteStream::Type::TruncateWriteOp) ||
               (InstanceType == AsyncWriteStream::Type::UpdateReservationOp));

    NTSTATUS status = STATUS_SUCCESS;

    if (InstanceType != AsyncWriteStream::Type::UpdateReservationOp)
    {
        status = CommonCtorInit(Log);

        if (NT_SUCCESS(status))
        {
            if (InstanceType == AsyncWriteStream::Type::LogicalCheckPointWriteOp)
            {
                _OnCheckpointSegmentWriteCompletion = KAsyncContextBase::CompletionCallback(
                                                        this,
                                                        &AsyncWriteStream::OnCheckpointSegmentWriteCompletion);

                status = CommonCheckpointCtorInit();
                if (NT_SUCCESS(status))
                {
                    status = AsyncWriteStream::CreateCheckpointAsyncSegmentWriteContext(
                        GetThisAllocator(),
                        Log,
                        StreamDesc,
                        _CheckpointSegmentWriteOp0);

                    if (NT_SUCCESS(status))
                    {
                        status = AsyncWriteStream::CreateCheckpointAsyncSegmentWriteContext(
                            GetThisAllocator(),
                            Log,
                            StreamDesc,
                            _CheckpointSegmentWriteOp1);
                    }
                }
            }

            else if (InstanceType == AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp)
            {
                status = KIoBufferView::Create(_SegmentView, GetThisAllocator(), KTL_TAG_LOGGER);
            }

            else if (InstanceType == AsyncWriteStream::Type::TruncateWriteOp)
            {
                // Make the reserved checkpoint write op to be used during truncates
                _TruncateCheckPointWriteCompletion =
                    KAsyncContextBase::CompletionCallback(this, &AsyncWriteStream::TruncateCheckPointWriteComplete);

                _TruncateTrimCompletion =
                    KAsyncContextBase::CompletionCallback(this, &AsyncWriteStream::TruncateTrimComplete);

                status = AsyncWriteStream::CreateCheckpointStreamAsyncWriteContext(
                    GetThisAllocator(),
                    Log,
                    _TruncateCheckPointOp);
            }
            else KInvariant(FALSE);
        }
    }

    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
    }
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::CommonCtorInit(__in RvdLogManagerImp::RvdOnDiskLog& Log)
{
    // BUG, richhas, xxxxxx, todo: Allocate one page aligned buffer for use as the first buffer of the
    //                             record header and metadata - all need this and most cases only need this
    _WriteCompletion = KAsyncContextBase::CompletionCallback(this, &AsyncWriteStream::LogFileWriteCompletion);
    NTSTATUS status = Log._BlockDevice->AllocateWriteContext(
        _WriteOp0,
        KTL_TAG_LOGGER);

    if (NT_SUCCESS(status))
    {
        status = Log._BlockDevice->AllocateWriteContext(
            _WriteOp1,
            KTL_TAG_LOGGER);
    }

    if (NT_SUCCESS(status))
    {
        status = KIoBuffer::CreateEmpty(_WorkingIoBuffer0, GetThisAllocator(), KTL_TAG_LOGGER);
    }

    if (NT_SUCCESS(status))
    {
        status = KIoBuffer::CreateEmpty(_WorkingIoBuffer1, GetThisAllocator(), KTL_TAG_LOGGER);
    }

    KInvariant(_QuantaAcquired == 0);
    _QuantaAcquiredCompletion = KAsyncContextBase::CompletionCallback(this, &AsyncWriteStream::QuantaAcquiredCompletion);
    if (NT_SUCCESS(status))
    {
        status = Log._StreamWriteQuotaGate->CreateAcquireContext(_QuantaAcquireOp);
    }

    if (!NT_SUCCESS(status))
    {
        _WorkingIoBuffer0.Reset();
        _WorkingIoBuffer1.Reset();
        _QuantaAcquireOp.Reset();
    }

    _ForceStreamCheckpoint = FALSE;

    return status;
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::CommonCheckpointCtorInit()
{
    return KIoBuffer::CreateEmpty(_CommonCheckpointRecordIoBuffer, GetThisAllocator(), KTL_TAG_LOGGER);
}

//** Factories for creating the various forms of AsyncWriteContext instances

//* UpdateReservationOp form of AsyncWriteContext Factory
RvdLogStream::AsyncReservationContext::AsyncReservationContext()
{
}

RvdLogStreamImp::AsyncReserveStream::AsyncReserveStream()
{
}

RvdLogStream::AsyncReservationContext::~AsyncReservationContext()
{
}

RvdLogStreamImp::AsyncReserveStream::~AsyncReserveStream()
{
}

RvdLogStreamImp::AsyncReserveStream::AsyncReserveStream(__in RvdLogStreamImp& Owner)
    :   _Owner(&Owner)
{
    NTSTATUS    status = RvdLogStreamImp::AsyncWriteStream::CreateUpdateReservationAsyncWriteContext(
        GetThisAllocator(),
        Owner,
        _BackingOp);

    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
    }
}

NTSTATUS
RvdLogStreamImp::CreateUpdateReservationContext(__out RvdLogStream::AsyncReservationContext::SPtr& Context)
{
    Context = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncReserveStream(*this);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }

    return status;
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::CreateUpdateReservationAsyncWriteContext(
    __in KAllocator& Allocator,
    __in RvdLogStreamImp& Owner,
    __out AsyncWriteStream::SPtr& Context)
{
    Context = _new(KTL_TAG_LOGGER, Allocator) AsyncWriteStream(
                                                    *Owner._Log,
                                                    Owner._LogStreamDescriptor,
                                                    AsyncWriteStream::Type::UpdateReservationOp);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Context->_OwningStream = &Owner;
    Context->_Log = Owner._Log;

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::CreateUpdateReservationAsyncWriteContext(
    __in KAllocator& Allocator,
    __in RvdLogManagerImp::RvdOnDiskLog& Log,
    __in LogStreamDescriptor& LogStreamDescriptor,
    __out KSharedPtr<AsyncWriteStream>& Context)
{
    Context = _new(KTL_TAG_LOGGER, Allocator) AsyncWriteStream(
                                                    Log,
                                                    LogStreamDescriptor,
                                                    AsyncWriteStream::Type::UpdateReservationOp);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Context->_Log = &Log;

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;
}

//* UserWriteOp form of AsyncWriteContext Factory
NTSTATUS
RvdLogStreamImp::CreateAsyncWriteContext(__out RvdLogStream::AsyncWriteContext::SPtr& Context)
{
    Context = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncWriteStream(this);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;
}

//* PhysicalCheckPointWriteOp form of AsyncWriteContext Factory
NTSTATUS
RvdLogStreamImp::AsyncWriteStream::CreateCheckpointStreamAsyncWriteContext(
    __in KAllocator& Allocator,
    __in RvdLogManagerImp::RvdOnDiskLog& Log,
    __out KSharedPtr<AsyncWriteStream>& Context)
{
    Context = _new(KTL_TAG_LOGGER, Allocator) AsyncWriteStream(Log);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;
}

//* LogicalCheckPointWriteOp form of AsyncWriteContext Factory
NTSTATUS
RvdLogStreamImp::AsyncWriteStream::CreateCheckpointRecordAsyncWriteContext(
    __in KAllocator& Allocator,
    __in RvdLogManagerImp::RvdOnDiskLog& Log,
    __in LogStreamDescriptor& ForStreamDesc,
    __out KSharedPtr<AsyncWriteStream>& Context)
{
    Context = _new(KTL_TAG_LOGGER, Allocator) AsyncWriteStream(Log, ForStreamDesc, Type::LogicalCheckPointWriteOp);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;
}

//* LogicalCheckPointSegmentWriteOp form of AsyncWriteContext Factory
NTSTATUS
RvdLogStreamImp::AsyncWriteStream::CreateCheckpointAsyncSegmentWriteContext(
    __in KAllocator& Allocator,
    __in RvdLogManagerImp::RvdOnDiskLog& Log,
    __in LogStreamDescriptor& ForStreamDesc,
    __out KSharedPtr<AsyncWriteStream>& Context)

{
    Context = _new(KTL_TAG_LOGGER, Allocator) AsyncWriteStream(Log, ForStreamDesc, Type::LogicalCheckPointSegmentWriteOp);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();

    if (NT_SUCCESS(status))
    {
    }

    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;
}


//* TruncateWriteOp form of AsyncWriteContext Factory
NTSTATUS
RvdLogStreamImp::AsyncWriteStream::CreateTruncationAsyncWriteContext(
    __in KAllocator& Allocator,
    __in RvdLogManagerImp::RvdOnDiskLog& Log,
    __in LogStreamDescriptor& ForStreamDesc,
    __out KSharedPtr<AsyncWriteStream>& Context)
{
    Context = _new(KTL_TAG_LOGGER, Allocator) AsyncWriteStream(Log, ForStreamDesc, Type::TruncateWriteOp);
    if (!Context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Context->Status();
    if (!NT_SUCCESS(status))
    {
        Context.Reset();
    }
    return status;
}


#pragma region Common RvdLogStreamImp::AsyncWriteStream imp

//** Common RvdLogStreamImp::AsyncWriteStream imp

RvdLogStreamImp::AsyncWriteStream::~AsyncWriteStream()
{
}

VOID
RvdLogStreamImp::AsyncWriteStream::DoCommonComplete(__in NTSTATUS Status)
{
    KInvariant(_AsyncActivitiesLeft == 0);

    if (_WorkingIoBuffer0)
    {
        _WorkingIoBuffer0->Clear();
    }

    if (_WorkingIoBuffer1)
    {
        _WorkingIoBuffer1->Clear();
    }

    if ((_Type == AsyncWriteStream::Type::UserWriteOp) &&
        (! _ForcedCheckpointWrite))
    {
        _InstrumentedOperation.EndOperation(_WriteSize);
    }
    
    _DedicatedForcedCheckpointWriteOp.Reset();
    BOOLEAN doingComplete = Complete(Status);
    KInvariant(doingComplete);
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnStart()
{
    switch (_Type)
    {
    case AsyncWriteStream::Type::UserWriteOp:
        OnStartUserRecordWrite();
        break;
    case AsyncWriteStream::Type::LogicalCheckPointWriteOp:
        OnCheckPointRecordWriteStart();
        break;
    case AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp:
        OnCheckPointRecordSegmentWriteStart();
        break;
    case AsyncWriteStream::Type::PhysicalCheckPointWriteOp:
        OnStartPhysicalCheckPointWrite();
        break;
    case AsyncWriteStream::Type::TruncateWriteOp:
        OnStartTruncation();
        break;
    case AsyncWriteStream::Type::UpdateReservationOp:
        OnStartUpdateReservation();
        break;
    default:
        KInvariant(FALSE);
    }
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnCompleted()
{
    switch (_Type)
    {
    case AsyncWriteStream::Type::UserWriteOp:
        OnCompletedUserRecordWrite();
        break;

    case AsyncWriteStream::Type::TruncateWriteOp:
        OnCompletedTruncation();
        break;
    }

    // No Quanta can stay allocated beyond this point
    KInvariant(_QuantaAcquired == 0);
}

VOID
RvdLogStreamImp::AsyncWriteStream::QuantaAcquiredCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
// Continued from OnStart*()
{
    UNREFERENCED_PARAMETER(CompletingSubOp);
    UNREFERENCED_PARAMETER(Parent);

    switch (_Type)
    {
    case AsyncWriteStream::Type::UserWriteOp:
        return OnUserRecordWriteQuantaAcquired();
        break;
    case AsyncWriteStream::Type::TruncateWriteOp:
        return OnTruncationQuantaAcquired();
        break;
    default:
        // Only UserWriteOps and TruncateWriteOps can own a quanta
        KInvariant(FALSE);
    }
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::ContinueRecordWrite()
//
// Continued from OnStart*()/On*QuantaAcquired() via log's stream write op queue - NOT appt safe
//
// Returns: STATUS_SUCCESS - Indicates log write op(s) started ok; else the Log will be marked as
//                           having failed (_IsLogFailed == TRUE)
{
    switch (_Type)
    {
    case AsyncWriteStream::Type::UserWriteOp:
        return OnContinueUserRecordWrite();
        break;
    case AsyncWriteStream::Type::TruncateWriteOp:
        return OnContinueTruncation();
        break;
    case AsyncWriteStream::Type::UpdateReservationOp:
        return OnContinueUpdateReservation();
        break;
    default:
        KInvariant(FALSE);
        return STATUS_NOT_IMPLEMENTED;
    }
}

RvdLogLsn
RvdLogStreamImp::AsyncWriteStream::GetAsyncOrderedGateKey()
{
    return _AssignedLsn;
}

RvdLogLsn
RvdLogStreamImp::AsyncWriteStream::GetNextAsyncOrderedGateKey()
{
    return _AssignedLsn + _SizeToWrite;
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::ComputePhysicalWritePlan(
    __in KIoBuffer& CompleteRecordBuffer,
    __out ULONGLONG& FirstSegmentFileOffset,
    __out ULONGLONG& SecondSegmentFileOffset)
//
// Builds _WorkingIoBuffer0 (and _WorkingIoBuffer1 if split EOF write). Returns
// file offsets for first and optional second segments of the write.
//
// Uses: _AssignedLsn and _SizeToWrite
{
    KInvariant(_SizeToWrite <= CompleteRecordBuffer.QuerySize());
    KInvariant((_SizeToWrite % RvdDiskLogConstants::BlockSize) == 0);

    // BUG, richhas, xxxxxx, todo: make the changes to take advantage of the new KIoBufferView
    ULONG       firstSegmentSize;
    ULONGLONG   firstSegmentFileOffset = _Log->ToFileAddress(_AssignedLsn, _SizeToWrite, firstSegmentSize);
    ULONG       secondSegmentSize = 0;
    ULONGLONG   secondSegmentFileOffset = 0;

    if (firstSegmentSize != _SizeToWrite)
    {
        // This record is being split (wrapped)
        KInvariant(firstSegmentSize < _SizeToWrite);
        secondSegmentFileOffset = _Log->ToFileAddress(
            RvdLogLsn(_AssignedLsn + firstSegmentSize),
            _SizeToWrite - firstSegmentSize,
            secondSegmentSize);

        KInvariant((secondSegmentFileOffset % RvdDiskLogConstants::BlockSize) == 0);
    }

    NTSTATUS    status;
    KInvariant((firstSegmentSize + secondSegmentSize) == _SizeToWrite);

    KInvariant(_WorkingIoBuffer0->QuerySize() == 0);
    status = _WorkingIoBuffer0->AddIoBufferReference(
        CompleteRecordBuffer,
        0,
        firstSegmentSize,
        KTL_TAG_LOGGER);

    if (! NT_SUCCESS(status))
    {
        return status;
    }

    if (secondSegmentSize > 0)
    {
        KInvariant(_WorkingIoBuffer1->QuerySize() == 0);
        status = _WorkingIoBuffer1->AddIoBufferReference(
            CompleteRecordBuffer,
            firstSegmentSize,
            secondSegmentSize,
            KTL_TAG_LOGGER);

        if (! NT_SUCCESS(status))
        {
            return status;
        }
    }

    FirstSegmentFileOffset = firstSegmentFileOffset;
    SecondSegmentFileOffset = secondSegmentFileOffset;

    return STATUS_SUCCESS;
}

VOID
RvdLogStreamImp::AsyncWriteStream::RelieveAsyncActivity()
{
    KInvariant(_AsyncActivitiesLeft >= 1);
    if (InterlockedDecrement(&_AsyncActivitiesLeft) == 0)
    {
        NTSTATUS status = _Log->EnqueueCompletedStreamWriteOp(*this);
        KInvariant(NT_SUCCESS(status));

        // Continued @ ContinueRecordWriteCompletion() once this record's LSN is the lowest
        // completing write op for the log
    }
}

VOID
RvdLogStreamImp::AsyncWriteStream::LogFileWriteCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
//
// Continued as write to the log file complete
{
    NTSTATUS status = CompletingSubOp.Status();
    CompletingSubOp.Reuse();

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &CompletingSubOp, (ULONGLONG)Parent, 0);
        InterlockedIncrement(&_WriteOpFailureCount);
    }

    RelieveAsyncActivity();             // Allow continuation @ ContinueRecordWriteCompletion()
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::ContinueRecordWriteCompletion()
//
//  Continued from RelieveAsyncActivity() once once this record's LSN is the lowest
//  completing write op for the log
{
    // Dispatch the completion event back to the _Type specific ContinueRecordWriteCompletions
    switch (_Type)
    {
    case AsyncWriteStream::Type::UserWriteOp:
        return OnUserRecordWriteCompletion();
    case AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp:
        return OnCheckPointRecordSegmentWriteCompletion();
    case AsyncWriteStream::Type::PhysicalCheckPointWriteOp:
        return OnPhysicalCheckPointWriteCompletion();

    default:
        KInvariant(FALSE);
        return STATUS_NOT_IMPLEMENTED;
    }
}

VOID
RvdLogStreamImp::AsyncWriteStream::CommonRecordWriteCompletion()
{
    // We have a successful stream/log write - update _HighestCompletedLsn
    // periodic physical checkpoint stream write
    KInvariant(_Log->_HighestCompletedLsn < _AssignedLsn);
    KInvariant(_Log->_CompletingStreamWriteLsnOrderedGateDispatched);

    if (!_Log->_DebugDisableHighestCompletedLsnUpdates)
    {
        _Log->_HighestCompletedLsn = _AssignedLsn;
    }
}

#pragma endregion Common RvdLogStreamImp::AsyncWriteStream imp

#pragma region AsyncWriteStream::Type::UpdateReservationOp logic
//** AsyncWriteStream::Type::UpdateReservationOp logic

VOID
RvdLogStreamImp::AsyncReserveStream::StartUpdateReservation(
    __in LONGLONG Delta,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _Delta = Delta;
    Start(ParentAsyncContext, CallbackPtr);

    // Continued @ RvdLogStreamImp::AsyncReserveStream::OnStart()
}

VOID
RvdLogStreamImp::AsyncReserveStream::OnStart()
{
    _BackingOp->StartUpdateReservation(_Delta, this, CompletionCallback(
                                                        this,
                                                        &RvdLogStreamImp::AsyncReserveStream::OnReserveUpdated));

    // Continued @ RvdLogStreamImp::AsyncWriteStream::StartUpdateReservation()
}

VOID
RvdLogStreamImp::AsyncWriteStream::StartUpdateReservation(
    __in LONGLONG DeltaReserve,                                 // Amount to adjust the current reserve by
    __in_opt KAsyncContextBase *const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    KInvariant(_Type == AsyncWriteStream::Type::UpdateReservationOp);
    _ReservationDelta = DeltaReserve;

    Start(ParentAsyncContext, CallbackPtr);
    // Continues @ OnStartUpdateReservation()
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnStartUpdateReservation()
{
    KInvariant(_Type == AsyncWriteStream::Type::UpdateReservationOp);

    // Queue up waiting for exclusive access to the Log - for LSN space reservation
    NTSTATUS        status = _Log->EnqueueStreamWriteOp(*this);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoCommonComplete(status);
        return;
    }
    // Continued @ OnContinueUpdateReservation via log's stream write op queue
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::OnContinueUpdateReservation()
{
    KInvariant(_Type == AsyncWriteStream::Type::UpdateReservationOp);

    ULONGLONG           delta = 0;

    if (_ReservationDelta > 0)
    {
        ULONGLONG logSpaceAvailable;

        if (_Log->_LogFileFreeSpace < _Log->_LogFileMinFreeSpace)
        {
            logSpaceAvailable = 0;
        } else {
            logSpaceAvailable = _Log->_LogFileFreeSpace - _Log->_LogFileMinFreeSpace;
        }
        delta = (ULONGLONG)_ReservationDelta;

        if (delta > logSpaceAvailable)
        {
            KTraceFailedAsyncRequest(STATUS_LOG_FULL, this, delta, logSpaceAvailable);
            DoCommonComplete(STATUS_LOG_FULL);
            return STATUS_SUCCESS;
        }

        _OwningStream->_ReservationHeld += delta;
        _Log->_LogFileReservedSpace += delta;
        _Log->_LogFileFreeSpace -= delta;
    }
    else
    {
        // Relieving reserve
        delta = (ULONGLONG)(-_ReservationDelta);

        if (_OwningStream)
        {
            if (delta > _OwningStream->_ReservationHeld)
            {
                KTraceFailedAsyncRequest(K_STATUS_LOG_RESERVE_TOO_SMALL, this, 0, 0);
                DoCommonComplete(K_STATUS_LOG_RESERVE_TOO_SMALL);
                return STATUS_SUCCESS;
            }

            _OwningStream->_ReservationHeld -= delta;
        }

        KInvariant(delta <= _Log->_LogFileReservedSpace);
        _Log->_LogFileReservedSpace -= delta;
        _Log->_LogFileFreeSpace += delta;
    }

    DoCommonComplete(STATUS_SUCCESS);
    return STATUS_SUCCESS;

    // Continued @ RvdLogStreamImp::AsyncReserveStream::OnReserveUpdated()
}

VOID
RvdLogStreamImp::AsyncReserveStream::OnReserveUpdated(__in KAsyncContextBase* const Parent, __in_opt KAsyncContextBase& SubOp)
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS        status = SubOp.Status();
    SubOp.Reuse();
    Complete(status);
}

#pragma endregion Common RvdLogStreamImp::UpdateReservationOp imp


#pragma region AsyncWriteStream::Type::UserWriteOp logic
//** AsyncWriteStream::Type::UserWriteOp logic

VOID
RvdLogStreamImp::AsyncWriteStream::StartWrite(
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    KInvariant(_Type == AsyncWriteStream::Type::UserWriteOp);

    _InstrumentedOperation.BeginOperation();    
    
    //**
    //  Beyond the creation of a physical log file, all user information written to a log file is written thru this
    //  log write pipeline. This pipeline is broken down into 7 sequential stages.
    //
    //  Stage 1: Preparation and validate of write operation and allocation of as much up front dynamic memory
    //           as needed for the operation as possible. Old writes at Asn's <= the current TruncationPoint are
    //           automatically completed successfully and logically they have already been written and truncated.
    //
    //  Stage 2: Single IoBuffer is constructed for the write operation. Headers are built. User metadata and
    //           as much of the header are partially checkedsummed as possible. Maximum record sizes and such
    //           are validated. During this phase, the AsnIndex is updated to reflect a new (or higher version)
    //           of the Asn and Version for the current write operation. The existing state of the impacted
    //           AsnIndex entry is recorded in the op context so that it might be reversed in the case of certain
    //           non-structural errors.
    //
    //  Stage 3: The overall record size is used to acquire a quota of that size from the log's QuotaGate and once
    //           acquired for a write operation, any completion of that operation will cause this acquired quota
    //           to be released back to the log's QuotaGate; allowing more operations held because of low quota to
    //           continue. The QuotaGate allows multiple operations to proceed thru it up to the number of cores in
    //           the machine.
    //
    //  Stage 4: The write op continues by acquiring the log's IncomingStreamWriteOpQueue. This acquisition represents
    //           a short term exclusive lock on the log's LSN and physical space and the stream state representing that
    //           spatial description. During this phase allocation of LSN space is made, final checks made for space
    //           availability, last few LSN space related fields of the header are set based on this allocated LSN and
    //           current LSN log state, the header's final checksum calculated (just the remaining LSN fields), the
    //           IoBuffer is (logically) broken into two IoBuffers if the last record is being written at the end of
    //           physical log space and it needs to be wrapped to the start of file, the AsnEntry (in the AsnIndex) is
    //           updated to pending or maybe the operation is completed with an error if a newer version for the Asn
    //           has raced ahead. If all is ok, any excess quanta is trimmed, and the write(s) are started. If a first
    //           write can't be started, the impact of the LSN space allocation done during this phase can be and is
    //           reversed as no changes are actually made to the disk structure. This phase ends when at least one
    //           of the write(s) successfully is started.
    //
    //           NOTE: This phase of logic does NOT execute in the apartment of the operation's async context. Logically
    //                 there are three forks continuing into Phase 5: 1) First physical write for the first part of the
    //                 log record, 2) Optional second write of the trailing part of the log record at the start of the
    //                 physical log file if a wrap was needed, and 3) the continuation of logic after the first successful
    //                 write starts. It is best to think of these three forks all racing with each other. Note that the
    //                 LSN space is still protected by the last of these three forks until it finishes its work (which
    //                 takes advantage of overlap with the physical writes).
    //
    //  Stage 5: This phase consists of up to three (but at least two) racing forks of execution - as described in
    //           Stage 4. Continuation from this stage only occurs when all of these forks are joined back together; at
    //           which time continuation is scheduled thru the final gate and called the log's LsnOrderedCompleationGate and
    //           leading to Stage 6. As the one (or two) physical write operation(s) complete and continue they record any
    //           physical write errors that might have occurred. The logic that continues after the one or two writes are
    //           started in Phase 4, represents the third fork of this stage. It does a small amount of LSN-space protected
    //           work: 1) adds the new record's LSN to the current stream's LSN-entry table, 2) updates the stream's highest
    //           LSN value, and conditionally triggers an asynchronous operation to write a new checkpoint record into the
    //           current stream and recording that highest checkpoint LSN for the stream, 3) conditionally triggers a physical
    //           checkpoint into the log's dedicated checkpoint stream if there has been a large enough spatial interval log
    //           since the last such checkpoint and there is enough free space, and 4) causes the log's
    //           IncomingStreamWriteOpQueue to be pumped allowing the next write operation into its stage 4. This last
    //           action allows any number of write operations to be active in their Stage 5 at the same time and doing
    //           actually physical writes to the log. NOTE: all of the decisions and any needed resource allocations to
    //           support writing stream-level and phyical checkpoint records are actually done in Stage 4 before any write
    //           to the log file are scheduled - their respective async op instances are simply started during this phase.
    //
    //  Stage 6: Continuation at this stage occurs when all of the processes in Stage 5 have completed at which time the
    //           current write operation is queued on the log's LsnOrderedCompletionGate and only the operation with the next
    //           higher sequential assigned LSN will clear the gate, giving it exclusive right to the completion logic/state
    //           for the log. During this phase, any disk write failures will be considered fatal for the log structure
    //           (recovery is required), all later or at the failed LSN will be reported as failed due to a structure fault.
    //           If there were no write errors, the record disposition for the Asn/Version in the current operation's stream's
    //           AsnIndex entry will be marked actually being on disk. Note: this only occurs iif a newer version of the
    //           operation's Asn has not executed their Stage 2 (see above). The log's highest completed LSN is updated to
    //           the operation's LSN. The LsnOrderedCompletionGate is then released; allowing the next higher operation in
    //           LSN-space to enter its stage 6.
    //
    //  Stage 7: Complete the current user write operation. NOTE: any triggered checkpoint write opertions are distinct and
    //           will complete in the LSN order assigned them by this opertion implementation. It is during this stage that
    //           that the remaining quanta from Stages 3 and 4 is released by the OnCompleted() method of the user write op.
    //**

    //** START: Stage 1
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _ForcedCheckpointWrite = FALSE;
    _ReserveToUse = 0;
    _LowPriorityIO = FALSE;
    _CurrentLogSize = nullptr;
    _CurrentLogSpaceRemaining = nullptr;

    InterlockedIncrement(&_Log->_CountOfOutstandingWrites);     // Test support
    Start(ParentAsyncContext, CallbackPtr);
    // Continues @ OnStartUserRecordWrite()
}

VOID
RvdLogStreamImp::AsyncWriteStream::StartWrite(
    __in BOOLEAN LowPriorityIO,
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    KInvariant(_Type == AsyncWriteStream::Type::UserWriteOp);

    _InstrumentedOperation.BeginOperation();
    
    //** START: Stage 1
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _ForcedCheckpointWrite = FALSE;
    _ReserveToUse = 0;
    _LowPriorityIO = LowPriorityIO;
    _CurrentLogSize = nullptr;
    _CurrentLogSpaceRemaining = nullptr;

    InterlockedIncrement(&_Log->_CountOfOutstandingWrites);     // Test support
    Start(ParentAsyncContext, CallbackPtr);
    // Continues @ OnStartUserRecordWrite()
}


VOID
RvdLogStreamImp::AsyncWriteStream::StartReservedWrite(
    __in ULONGLONG ReserveToUse,
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    KInvariant(_Type == AsyncWriteStream::Type::UserWriteOp);

    _InstrumentedOperation.BeginOperation();
    
    //** START: Stage 1
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _ForcedCheckpointWrite = FALSE;
    _ReserveToUse = ReserveToUse;
    _LowPriorityIO = FALSE;
    _CurrentLogSize = nullptr;
    _CurrentLogSpaceRemaining = nullptr;

    InterlockedIncrement(&_Log->_CountOfOutstandingWrites);     // Test support
    Start(ParentAsyncContext, CallbackPtr);
    // Continues @ OnStartUserRecordWrite()
}

VOID
RvdLogStreamImp::AsyncWriteStream::StartReservedWrite(
    __in ULONGLONG ReserveToUse,
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __out ULONGLONG& LogSize,
    __out ULONGLONG& LogSpaceRemaining,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    KTraceFailedAsyncRequest(STATUS_NOT_IMPLEMENTED, this, 0, 0);
    KInvariant(FALSE);

    KInvariant(_Type == AsyncWriteStream::Type::UserWriteOp);

    _InstrumentedOperation.BeginOperation();

    //** START: Stage 1
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _ForcedCheckpointWrite = FALSE;
    _ReserveToUse = ReserveToUse;
    _LowPriorityIO = FALSE;
    _CurrentLogSize = &LogSize;
    _CurrentLogSpaceRemaining = &LogSpaceRemaining;

    InterlockedIncrement(&_Log->_CountOfOutstandingWrites);     // Test support
    Start(ParentAsyncContext, CallbackPtr);
    // Continues @ OnStartUserRecordWrite()
}

VOID
RvdLogStreamImp::AsyncWriteStream::StartReservedWrite(
    __in BOOLEAN LowPriorityIO,
    __in ULONGLONG ReserveToUse,
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    KInvariant(_Type == AsyncWriteStream::Type::UserWriteOp);

    _InstrumentedOperation.BeginOperation();
    
    //** START: Stage 1
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _ForcedCheckpointWrite = FALSE;
    _ReserveToUse = ReserveToUse;
    _LowPriorityIO = LowPriorityIO;
    _CurrentLogSize = nullptr;
    _CurrentLogSpaceRemaining = nullptr;

    InterlockedIncrement(&_Log->_CountOfOutstandingWrites);     // Test support
    Start(ParentAsyncContext, CallbackPtr);
    // Continues @ OnStartUserRecordWrite()
}


// Special form of user write that forces a physical checkpoint write to occur and holds completion
// of this write op until the CP write completes. The result (status) of that CP write is the
// resulting completion status of this op.
VOID
RvdLogStreamImp::AsyncWriteStream::StartForcedCheckpointWrite(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _RecordAsn = RvdLogAsn::Null();
    _Version = 0;
    _MetaDataBuffer.Reset();
    _IoBuffer.Reset();
    _ForcedCheckpointWrite = TRUE;
    _ReserveToUse = 0;
    _CurrentLogSize = nullptr;
    _CurrentLogSpaceRemaining = nullptr;

    InterlockedIncrement(&_Log->_CountOfOutstandingWrites);     // Test support
    Start(ParentAsyncContext, CallbackPtr);
    // Continues @ OnStartUserRecordWrite()
}

VOID
RvdLogStreamImp::AsyncWriteStream::DoUserRecordWriteComplete(__in NTSTATUS Status)
{
    KInvariant(_Type == AsyncWriteStream::Type::UserWriteOp);
    InterlockedDecrement(&_Log->_CountOfOutstandingWrites);

    if (_TryDeleteAsnIndexEntryOnCompletion)
    {
        // this is multally exclusive to _IsSavedAsnIndexEntryValid
        KInvariant(!_ForcedCheckpointWrite);
        _AsnIndex->TryRemove(_AsnIndexEntry, _Version);
        _TryDeleteAsnIndexEntryOnCompletion = FALSE;
    }

    if (_IsSavedAsnIndexEntryValid)
    {
        // this is multally exclusive to _TryDeleteAsnIndexEntryOnCompletion
        KInvariant(!_TryDeleteAsnIndexEntryOnCompletion);
        _AsnIndex->Restore(_AsnIndexEntry, _Version, _SavedAsnIndexEntry);
        _IsSavedAsnIndexEntryValid = FALSE;
    }

    _IoBuffer.Reset();
    _MetaDataBuffer.Reset();
    _AsnIndexEntry.Reset();
    _CombinedUserRecordIoBuffer->Clear();

    DoCommonComplete(Status);
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnStartUserRecordWrite()
//
// Continued from StartWrite() or StartForcedCheckpointWrite()
{
    KInvariant(_Type == AsyncWriteStream::Type::UserWriteOp);

    NTSTATUS    status = STATUS_SUCCESS;
    
    _IsSavedAsnIndexEntryValid = FALSE;
    _TryDeleteAsnIndexEntryOnCompletion = FALSE;
    _WroteMultiSegmentStreamCheckpoint = FALSE;

                                    // Assume a physical CP record will be written
    ULONGLONG   quantaSizeToAcquire = _Log->_MaxCheckPointLogRecordSize;

    if (!_ForcedCheckpointWrite)
    {
        if (_IoBuffer == nullptr)
        {
            _IoBuffer = _Log->_EmptyIoBuffer;
            KInvariant(_IoBuffer->QuerySize() == 0);
        }

        if (_MetaDataBuffer == nullptr)
        {
            _MetaDataBuffer = _Log->_EmptyBuffer;
            KInvariant(_MetaDataBuffer->QuerySize() == 0);
        }

        ULONG       ioBufferSize = _IoBuffer->QuerySize();
        ULONG       metaDataSize = RvdLogUserRecordHeader::ComputeMetaDataSize(_MetaDataBuffer->QuerySize());
        ULONG       headerAndMetaDataSize = RvdLogUserRecordHeader::ComputeSizeOnDisk(_MetaDataBuffer->QuerySize());

        // Compute the worst case quanta of the outstanding write quota required by this operation.
        // The possibility of a worst case physical CP record has been included in quantaSizeToAcquire
        // already.
        _WriteSize = (headerAndMetaDataSize + ioBufferSize);
        
        quantaSizeToAcquire +=
            (headerAndMetaDataSize +
             ioBufferSize +
             RvdLogUserStreamCheckPointRecordHeader::GetMaxSinglePartRecordSize(_Log->_Config));

        if ((headerAndMetaDataSize > _Log->_Config.GetMaxMetadataSize()) ||
            (ioBufferSize > _Log->_Config.GetMaxIoBufferSize()) ||
            (quantaSizeToAcquire > _Log->_Config.GetMaxRecordSize()))
        {
            KTraceFailedAsyncRequest(STATUS_BUFFER_OVERFLOW, this, ioBufferSize, _Log->_Config.GetMaxIoBufferSize());
            DoUserRecordWriteComplete(STATUS_BUFFER_OVERFLOW);
            return;
        }

        if (_RecordAsn <= _StreamDesc.TruncationPoint)
        {
            // The record is already written and then has been truncated (logically)
            DoUserRecordWriteComplete(STATUS_SUCCESS);
            return;
        }

        //
        // TODO: Allow logical log IoBuffer for metadata to be used
        // instead of copying
        //
        KIoBufferElement::SPtr      headerAndMetaData;
        union
        {
            void*                   headerAndMetaDataBuffer;
            RvdLogUserRecordHeader* headerPtr;
        };

        status = KIoBufferElement::CreateNew(
            headerAndMetaDataSize,
            headerAndMetaData,
            headerAndMetaDataBuffer,
            GetThisAllocator(),
            KTL_TAG_LOGGER);

        if (!NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            DoUserRecordWriteComplete(status);
            return;
        }
        //** END: Stage 1


        //** START: Stage 2

        //* Init the portions of the header that we can now in the
        // RvdLogRecordHeader and RvdLogUserStreamRecordHeader portions
        headerPtr->Initialize(
            RvdLogUserStreamRecordHeader::Type::UserRecord,
            _Log->_LogId,
            &_Log->_LogSignature[0],
            _StreamInfo.LogStreamId,
            _StreamInfo.LogStreamType,
            headerAndMetaDataSize,              // Overall HeaderSize
            ioBufferSize,
            metaDataSize,
            _StreamDesc.TruncationPoint);

        // RvdLogUserRecordHeader portion
        headerPtr->Asn = _RecordAsn;
        headerPtr->AsnVersion = _Version;

        // Move in Caller's Metadata
        {
            ULONG userMetadataSize = headerPtr->GetSizeOfUserMetaData();
            KInvariant(userMetadataSize == _MetaDataBuffer->QuerySize());

            if (userMetadataSize > 0)
            {
                KMemCpySafe(
                    &headerPtr->UserMetaData[0],
                    headerPtr->GetSizeOfUserMetaData(),
                    _MetaDataBuffer->GetBuffer(),
                    userMetadataSize);
            }
        }

        // Compute a partial chksum - sans RvdLogRecordHeader::LsnChksumBlock
        headerPtr->ThisBlockChecksum = KChecksum::Crc64(
            ((UCHAR*)headerPtr) + sizeof(headerPtr->LsnChksumBlock),
            metaDataSize + sizeof(RvdLogRecordHeader) - sizeof(headerPtr->LsnChksumBlock),
            0);

        // Build _CombinedUserRecordIoBuffer - This buffer may be split into two buffers once file space allocation occurs
        _CombinedUserRecordIoBuffer->AddIoBufferElement(*headerAndMetaData);     // Add header + metadata
        if (ioBufferSize > 0)
        {
            status = _CombinedUserRecordIoBuffer->AddIoBufferReference(                     // Add any page aligned I/O buffers
                *_IoBuffer,
                0,                              // Offset into _IoBuffer
                ioBufferSize,
                KTL_TAG_LOGGER);

            if (!NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, this, 0, 0);
                DoUserRecordWriteComplete(status);
                return;
            }
        }

        RvdAsnIndexEntry::SPtr  recordIndex = _new(KTL_TAG_LOGGER, GetThisAllocator())
                                              RvdAsnIndexEntry(_RecordAsn, _Version, _CombinedUserRecordIoBuffer->QuerySize());
        if (!recordIndex)
        {
            KTraceFailedAsyncRequest(STATUS_INSUFFICIENT_RESOURCES, this, 0, 0);
            DoUserRecordWriteComplete(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }

        ULONGLONG dontCare;
        status = _AsnIndex->AddOrUpdate(recordIndex, _AsnIndexEntry, _SavedAsnIndexEntry, _IsSavedAsnIndexEntryValid, dontCare);
        if (!NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), _Version);
            DoUserRecordWriteComplete(status);
            return;
        }
        KInvariant(_AsnIndexEntry);

        #if DBG
            // Record "this" for an active log stream write
            _AsnIndexEntry->DebugContext = (ULONGLONG)this;
        #endif

        // _TryDeleteAsnIndexEntryOnCompletion is set TRUE if a new _AsnIndexEntry was added. This will cause the
        // operation completion code to try and delete (version-protected) that new entry. It is up to later logic
        // in this pipeline to set _TryDeleteAsnIndexEntryOnCompletion -> FALSE once it is determined that we will
        // keep such a newly created RvdAsnIndexEntry for this write opertion.
        _TryDeleteAsnIndexEntryOnCompletion = !_IsSavedAsnIndexEntryValid;

        //** END: Stage 2
    }
    else
    {
        // Doing a forced CP write - allocate an CP write op
        KInvariant(quantaSizeToAcquire == _Log->_MaxCheckPointLogRecordSize);

        // Allocate a temp async op instance used to write physical log record
        status = CreateCheckpointStreamAsyncWriteContext(
            GetThisAllocator(),
            *_Log,
            _DedicatedForcedCheckpointWriteOp);

        if (!NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            DoUserRecordWriteComplete(status);
            return;
        }
    }

    //** START: Stage 3
    // Acquire max quanta from log level outstanding write quota gate -
    // max of MaxQueuedWriteDepth.


    //
    // Ensure there is enough total quanta to satisfy this write. There
    // may be a case where the maximum available quanta is not large
    // enough
    //
    if (quantaSizeToAcquire > _Log->_Config.GetMaxQueuedWriteDepth())
    {
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoUserRecordWriteComplete(status);
        return;
    }

    _QuantaAcquireOp->StartAcquire(quantaSizeToAcquire, this, _QuantaAcquiredCompletion);
    // Continued @ OnUserRecordWriteQuantaAcquired

    //** END: Stage 3
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnUserRecordWriteQuantaAcquired()
//  Continued from OnStartUserRecordWrite()
{
    NTSTATUS status = _QuantaAcquireOp->Status();
    if (!NT_SUCCESS(status))
    {
        _QuantaAcquireOp->Reuse();
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoUserRecordWriteComplete(status);
        return;
    }

    //** A quanta has been acquired from the log's outstanding write quota - record it.
    //   any completion (thru DoCommonComplete) will automatically release a quanta of
    //   _QuantaAcquired back to the Log's quota gate.
    //
    KInvariant(_QuantaAcquired == 0);
    _QuantaAcquired = _QuantaAcquireOp->GetDesiredQuanta();
    _QuantaAcquireOp->Reuse();

    // Queue up waiting for exclusive access to the Log - for LSN space assignment and
    // write scheduling.
    status = _Log->EnqueueStreamWriteOp(*this);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoUserRecordWriteComplete(status);
        return;
    }
    // Continued @ OnContinueUserRecordWrite via log's stream write op queue

    //** END: Stage 3
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::OnContinueUserRecordWrite()
//
// Continued from OnUserRecordWriteQuantaAcquired() via log's stream write op queue - NOT appt safe
//
// Returns: STATUS_SUCCESS          - Indicates log write op(s) started ok
//          All others:             - the Log will be marked as having failed (_IsLogFailed == TRUE)
//
//          NOTE: on return, the log's stream write queue will have another dequeue posted; allowing
//                the next write op in the queue to dispatched here eventually. This is defeated if
//                the logic below calls log->SuspendIncomingStreamWriteDequeuing() before returned. If
//                SuspendIncomingStreamWriteDequeuing() is called, a later call to ResumeIncomingStreamWriteDequeuing()
//                unlocks the queue and allows the next write op in the to be dispatched here.
//
{
    KInvariant(_Type == AsyncWriteStream::Type::UserWriteOp);

    //** START: Stage 4
    RvdLogManagerImp::RvdOnDiskLog* log = _Log.RawPtr();
    KInvariant(log->IsIncomingStreamWriteOpQueueDispatched());

    if (log->_IsLogFailed)
    {
        // Block all further stream writes once the log file has faulted.
        DoUserRecordWriteComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return K_STATUS_LOG_STRUCTURE_FAULT;
    }

    NTSTATUS                status = STATUS_SUCCESS;
    RvdLogRecordHeader*     header = nullptr;
    ULONG                   sizeToWrite;

    // Allocate LSN
    _AssignedLsn = log->_NextLsnToWrite;

    if (_ForcedCheckpointWrite)
    {
        _SizeToWrite = 0;
        sizeToWrite = 0;
    }
    else
    {
        // Guarentee that the LsnIndex.AddHigherLsnRecord() calls below are NOFAIL
        status = _StreamDesc.LsnIndex.GuarenteeAddTwoHigherRecords();
        if (!NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            DoUserRecordWriteComplete(status);
            return STATUS_SUCCESS;
        }

        // At this point this StreamWriteOp "owns" the LogFile Write engine and LSN state. Next, both the LSN space and
        // file location space must be allocated for this pending record - and any checkpoint records to be triggered as
        // a side effect of this write.
        _SizeToWrite = _CombinedUserRecordIoBuffer->QuerySize();
        sizeToWrite =_SizeToWrite;
        header = (RvdLogRecordHeader*)(_CombinedUserRecordIoBuffer->First()->GetBuffer());

        KInvariant((sizeToWrite % RvdDiskLogConstants::BlockSize) == 0);

        // Allocate LSN to record and finish the chksum of the record header/metadata
        header->InitializeLsnProperties(
            _AssignedLsn,                                   // Lsn of record
            log->_HighestCompletedLsn,                      // Highest completed LSN in physical log
            log->_CheckPointStream->Info.HighestLsn,        // LastCheckPointLsn
            _StreamInfo.IsEmptyStream()                     // PrevLsnInLogStream
                ? RvdLogLsn::Null()
                : _StreamInfo.HighestLsn);

        header->ThisBlockChecksum = KChecksum::Crc64((void*)header, sizeof header->LsnChksumBlock, header->ThisBlockChecksum);
        //*** NOTE: Record content is ready to write at this point

        // Update Log File LSN space allocation info
        KInvariant(header->Lsn == log->_NextLsnToWrite);

        // Update the record's LSN and Disposition so that it will be included in the next stream cp - possible
        // scheduled below
        if (!_AsnIndex->UpdateLsnAndDisposition(
                _AsnIndexEntry,
                _Version,
                RvdLogStream::RecordDisposition::eDispositionPending,
                _AssignedLsn))
        {
            // A later version of this record (ASN) raced ahead between here and OnStart() for this version's operation
            // instance (this); disallow over writing or deleting newer ASN entry.
            _IsSavedAsnIndexEntryValid = FALSE;
            _TryDeleteAsnIndexEntryOnCompletion = FALSE;

            KTraceFailedAsyncRequest(STATUS_OBJECT_NAME_COLLISION, this, 0, 0);
            DoUserRecordWriteComplete(STATUS_OBJECT_NAME_COLLISION);
            return STATUS_SUCCESS;
        }
    }

    // Determine if one or both checkpoint (CP) thresholds have been reached (or any other condition triggering
    // CP writes). If there are to be CP writes, use the pre-allocated write contexts in the log, and setup
    // those writes at this point (meaning calc their size, record their LSNs locally). INCLUDE those lengths
    // in the log full determination and disallow the entire set of writes in not enough room. Once the record
    // write(s) have/has been started successfully, move the allocated LSN space forward and freespace back to
    // match any to-be scheduled one or two reserved (and preallocated) async ops for the CP writes, actually
    // kick those off once the record writes are started for the user record. NOTE: they do not modify the
    // LSN space (this is done here). These ops get gated thru the log's LSN ordered comp gate and will thus
    // update the log's highest comp lsn. In the case of the CP log stream, its StreamDesc and StreamInfo both
    // will be updated. In the case of the logical CP record being written to the current stream, its _StreamInfo
    // (HighestLsn and NextLsn) and the StreamDesc's HighestLsn will be updated.
    //
    // To summerize the worst case (ignoring the details of wrapping writes), one user record write can result in 1)
    // a max logical CP record in it's own stream, AND 2) a max physical checkpoint record being written into dedicated
    // CP stream. From a QuotaGate perspective all user write ops will assume this worst case when waiting for their
    // quota. But once the actual overall size of all data to be written into the physical log is computes (as
    // described above), there is a partial release of quota back to the gate. The remaining allocated quota is then
    // represents these (up to three) operations: user, user stream CP, and phyiscal CP record in dedicated CP stream.
    // Once all of these operations complete, the parent (user op) that spawned any CP activity will complete and it's
    // OnCompleted() method will release the overall allocated quota. This all occurs once all sub-ops clear the log's
    // LSN OrderedGate during their completions. NOTE: in the normal case there are no CP records written so this
    // boils down to a very short over quota allocation allowing for this worst case and then a release (here) back
    // to only what the current user stream write is actually writing.

    //** Pre-decide if there is going to be: up to two CP related writes; need their size and offsets now.

    // NOTE: It is assumed that any of the pre-allocated checkpoint write operation instances will be available
    //       because the maximum QuotaGate quota is smaller than these intervals; providing a guarentee that the
    //       log's _NextLsnToWrite point can't advance too far ahead to allow a given second CP write to be needed
    //       while there's still an outstanding cp write - that outstanding op gates progress thru the log's LSN
    //       Ordered Completion Gate - another guarentee.
    //
    RvdLogLsn               physicalCheckpointRecordLsn;
    ULONGLONG               overallPhysicalCheckpointRecordSize = 0;
    AsyncWriteStream::SPtr  physicalCheckPointWriteOp;
    RvdLogLsn               nextLsnToWriteAfterUserRecord = _AssignedLsn + sizeToWrite;
    RvdLogLsn               highestLsnGeneratedByPhysLogOp;

    // (#3) Save current Stream LSN state in case of recoverable error between here and
    // a successful start of the first write; update stream state if not a forced CP write
    RvdLogLsn               savedStreamInfoHighestLsn = _StreamInfo.HighestLsn;
    RvdLogLsn               savedStreamInfoNextLsn    = _StreamInfo.NextLsn;
    RvdLogLsn               savedStreamInfoLowestLsn  = _StreamInfo.LowestLsn;

    if (!_ForcedCheckpointWrite)
    {
        // (#3) Update current stream LSN state so that PrepareForPhysicalCheckPointWrite() will capture
        // the current record's impact.
        if (_StreamInfo.IsEmptyStream())
        {
            // Establish the LowestLsn for the stream when going non-empty
            _StreamInfo.LowestLsn = _AssignedLsn;
        }

        _StreamInfo.HighestLsn = _AssignedLsn;
        _StreamInfo.NextLsn    = nextLsnToWriteAfterUserRecord;
    }

    if (_ForcedCheckpointWrite ||
        (!log->_DebugDisableAutoCheckpointing &&
            (log->_CheckPointStream->Info.NextLsn.IsNull()      ||
            ((nextLsnToWriteAfterUserRecord.Get() -
              log->_CheckPointStream->Info.NextLsn.Get()) >= (LONGLONG)log->_Config.GetCheckpointInterval()))))
    {
        // Time for next physical checksum write - capture state and overall LSN size. The only failure after a successful
        // call to PrepareForPhysicalCheckPointWrite will be an I/O error - faulting the entire log if that occurs
        physicalCheckPointWriteOp = (_DedicatedForcedCheckpointWriteOp == nullptr)  // Use the dedicated write op if possible; else the
                                    ?  log->_PhysicalCheckPointWriteOp              // use the log's cp write op
                                    : _DedicatedForcedCheckpointWriteOp;

        physicalCheckpointRecordLsn = nextLsnToWriteAfterUserRecord;

        physicalCheckPointWriteOp->PrepareForPhysicalCheckPointWrite(
            *log,
            physicalCheckpointRecordLsn,
            log->_HighestCompletedLsn,
            log->_CheckPointStream->Info.HighestLsn,        // LastCheckPointLsn
            log->_CheckPointStream->Info.HighestLsn,        // PrevLsnInLogStream
            overallPhysicalCheckpointRecordSize,
            highestLsnGeneratedByPhysLogOp);
    }

    RvdLogLsn                   logicalCheckpointRecordLsn;
    RvdLogLsn                   highestLogicalCheckpointRecordLsn;
    ULONG                       streamCheckpointRecordSize = 0;
    ULONG                       streamCheckpointQuanta = 0;
    AsyncWriteStream::SPtr      checkPointRecordWriteOp;

    if (!_ForcedCheckpointWrite)
    {
        // #1: Record the next higher LSN being written for the stream - this has to be done here so that if a stream
        // level checkpoint record is generated (PrepareForCheckPointRecordWrite) below, this record will be included
        // in that checkpoint.
        KInvariant((header->MetaDataSize + header->IoBufferSize) <= sizeToWrite);
        if (!NT_SUCCESS(_StreamDesc.LsnIndex.AddHigherLsnRecord(_AssignedLsn, header->ThisHeaderSize, header->IoBufferSize)))
        {
            // This can't happen because an entry was guarenteed by the call to LsnIndex.GuarenteeAddTwoHigherRecords()
            // above.
            KInvariant(FALSE);
        }

        if (!log->_DebugDisableAutoCheckpointing &&
            (_StreamDesc.LastCheckPointLsn.IsNull() ||
            _ForceStreamCheckpoint ||
            ((_AssignedLsn.Get() -
              _StreamDesc.LastCheckPointLsn.Get()) >= (LONGLONG)log->_Config.GetStreamCheckpointInterval())))
        {
            // Time to write a checkpoint record into this stream - prepare the op such that no failure except I/O
            // can occur after PrepareForCheckPointRecordWrite is called.
            checkPointRecordWriteOp = _StreamDesc.CheckPointRecordWriteOp;
            logicalCheckpointRecordLsn = nextLsnToWriteAfterUserRecord + overallPhysicalCheckpointRecordSize;

            // TODO: Need to ensure there is LSNIndex space for all
            // stream checkpoint segments

            status = checkPointRecordWriteOp->PrepareForCheckPointRecordWrite(
                *log,
                logicalCheckpointRecordLsn,
                log->_HighestCompletedLsn,
                (physicalCheckPointWriteOp) ? physicalCheckpointRecordLsn :                 // LastCheckPointLsn
                                              log->_CheckPointStream->Info.HighestLsn,
                _AssignedLsn,                                                               // PrevLsnInLogStream
                streamCheckpointRecordSize,
                highestLogicalCheckpointRecordLsn);

            if (!NT_SUCCESS(status))
            {
                // Reverse (#3) above
                _StreamInfo.HighestLsn = savedStreamInfoHighestLsn;
                _StreamInfo.NextLsn    = savedStreamInfoNextLsn;
                _StreamInfo.LowestLsn  = savedStreamInfoLowestLsn;

                if (physicalCheckPointWriteOp)
                {
                    physicalCheckPointWriteOp->CancelPrepareForPhysicalCheckPointWrite();
                }

                RvdLogLsn highestLsnRemoved = _StreamDesc.LsnIndex.RemoveHighestLsnRecord();  // reverse #1: AddHigherLsnRecord()
                KInvariant(_AssignedLsn == highestLsnRemoved);
                KTraceFailedAsyncRequest(status, this, 0, 0);
                DoUserRecordWriteComplete(status);
                return STATUS_SUCCESS;
            }

            KInvariant((streamCheckpointRecordSize % RvdDiskLogConstants::BlockSize) == 0);
            KInvariant(streamCheckpointRecordSize >= RvdDiskLogConstants::BlockSize);

            // streamCheckpointQuanta will be MaxSinglePartRecordSize iif a multi-segment stream CP was generated by
            // PrepareForCheckPointRecordWrite() above
            streamCheckpointQuanta =
                (streamCheckpointRecordSize > RvdLogUserStreamCheckPointRecordHeader::GetMaxSinglePartRecordSize(log->_Config))
                            ? RvdLogUserStreamCheckPointRecordHeader::GetMaxSinglePartRecordSize(log->_Config)
                            : streamCheckpointRecordSize;
        }
    }

    // At this point all required LSN space is know exactly. There must be enough free space w/pad
    // for all records (user, physical cp, and this stream's cp).
    ULONGLONG   totalLsnSpaceNeeded = sizeToWrite + overallPhysicalCheckpointRecordSize + streamCheckpointRecordSize;

    // NOTE: streamCheckpointRecordSize may be > streamCheckpointQuanta
    ULONGLONG   totalQuantaNeeded = sizeToWrite + overallPhysicalCheckpointRecordSize + streamCheckpointQuanta;
    ULONGLONG   reservationToUse = 0;
    BOOLEAN     reservationOveruse = FALSE;

    // Cause LOG-FULL when _LogFileFreeSpace becomes less than _LogFileMinFreeSpace; allowing for write reservations
    if (_ReserveToUse > 0)
    {
        KInvariant(_OwningStream != nullptr);       // Only user API driven write can use reservations

        // Check for over use of stream reserve
        if (_OwningStream->_ReservationHeld < _ReserveToUse)
        {
            reservationOveruse = TRUE;
        }
        else
        {
            KInvariant(log->_LogFileReservedSpace >= _ReserveToUse);    // Some how the stream level reserve not
                                                                    // in sync with log level info
            reservationToUse = _ReserveToUse;
            _ReserveToUse = 0;
        }
    }

    // NOTE: _LogFileFreeSpace can become < _LogFileMinFreeSpace do to truncation generated physical CPs which do not limit
    //       themselves to the _LogFileMinFreeSpace constraint - if fact _LogFileMinFreeSpace is a reserve for those CPs.
    ULONGLONG   availableSpaceInLog = (log->_LogFileFreeSpace < log->_LogFileMinFreeSpace)
                                          ? 0
                                          : (log->_LogFileFreeSpace - log->_LogFileMinFreeSpace);

    if ((totalLsnSpaceNeeded > (availableSpaceInLog + reservationToUse)) || reservationOveruse)
    {
        NTSTATUS        status1 = reservationOveruse ? K_STATUS_LOG_RESERVE_TOO_SMALL : STATUS_LOG_FULL;

        // LOG FULL or attempted Reservation overuse
        if (physicalCheckPointWriteOp)
        {
            physicalCheckPointWriteOp->CancelPrepareForPhysicalCheckPointWrite();
        }
        if (checkPointRecordWriteOp)
        {
            checkPointRecordWriteOp->CancelPrepareForCheckPointRecordWrite();
        }

        if (!_ForcedCheckpointWrite)
        {
            RvdLogLsn highestLsnRemoved = _StreamDesc.LsnIndex.RemoveHighestLsnRecord();  // reverse #1: AddHigherLsnRecord()
            KInvariant(_AssignedLsn == highestLsnRemoved);

            // Reverse (#3) above
            _StreamInfo.HighestLsn = savedStreamInfoHighestLsn;
            _StreamInfo.NextLsn    = savedStreamInfoNextLsn;
            _StreamInfo.LowestLsn  = savedStreamInfoLowestLsn;
        }

        if (!log->_DebugDisableAssertOnLogFull)
        {
            KInvariant(FALSE);
        }

        KTraceFailedAsyncRequest(status1, this, 0, 0);
        DoUserRecordWriteComplete(status1);
        return STATUS_SUCCESS;
    }

    // Account for any Reserve being used
    if (reservationToUse > 0)
    {
        _OwningStream->_ReservationHeld -= reservationToUse;
        log->_LogFileReservedSpace -= reservationToUse;
        log->_LogFileFreeSpace += reservationToUse;
        availableSpaceInLog += reservationToUse;
        reservationToUse = 0;
    }

    // Trim the quanta reserved from the log's QuotaGate to reflect only totalQuantaNeeded.
    KInvariant(totalQuantaNeeded <= _QuantaAcquired);
    log->_StreamWriteQuotaGate->ReleaseQuanta(_QuantaAcquired - totalQuantaNeeded);
    _QuantaAcquired = totalQuantaNeeded;

    // (#2) Save current LOG state in case of recoverable error between here and
    // a successful start of the first write
    RvdLogLsn   savedLogNextLsnToWrite = log->_NextLsnToWrite;

    // Allocate all physical LSN space needed by this user record and any checkpoint related records
    // triggered by this user write
    KInvariant(log->_LogFileFreeSpace >= totalLsnSpaceNeeded);
    log->_LogFileFreeSpace -= totalLsnSpaceNeeded;
    log->_NextLsnToWrite    = _AssignedLsn + totalLsnSpaceNeeded;
    KInvariant(_Log->_LogFileFreeSpace == (_Log->_LogFileLsnSpace -
                                        _Log->_LogFileReservedSpace -
                                        (_Log->_NextLsnToWrite.Get() - _Log->_LowestLsn.Get())));

    if (_ForcedCheckpointWrite)
    {
        //*** This is a special form of write operation to just force a CP write. This operation will hold its
        // completion until that underlying CP write has completed.

        // Update Checkpoint Stream's LSN spatial info
        log->_CheckPointStream->Info.HighestLsn = highestLsnGeneratedByPhysLogOp;
        log->_CheckPointStream->Info.NextLsn = physicalCheckpointRecordLsn + overallPhysicalCheckpointRecordSize;

        if (physicalCheckpointRecordLsn == RvdLogLsn::Min())
        {
            // First CP written - establish the initial LowestLsn value for that stream.
            KInvariant(log->_CheckPointStream->Info.LowestLsn == RvdLogLsn::Null());
            log->_CheckPointStream->Info.LowestLsn = RvdLogLsn::Min();
        }

        // Fire off physical checkpoint to write(s)
        KInvariant(_QuantaAcquired == overallPhysicalCheckpointRecordSize);
        physicalCheckPointWriteOp->StartPhysicalCheckPointWrite(this, _ForcedCheckpointWriteCompletion);

        return STATUS_SUCCESS;
        // Continued @ ForcedCheckpointWriteCompletion()
    }

    // Update this Stream's LSN spatial bounds info in the case a stream CP record
    // is being written
    if (streamCheckpointRecordSize != 0)
    {
        KInvariant(!logicalCheckpointRecordLsn.IsNull());

        if (_StreamInfo.IsEmptyStream())
        {
            // This occurs only when the first record of a stream is written after the stream is empty
            _StreamInfo.LowestLsn = _AssignedLsn;
        }

        // NOTE: HighestLsn and NextLsn are already saved above (#3)
        _StreamInfo.HighestLsn = highestLogicalCheckpointRecordLsn;
        _StreamInfo.NextLsn    = logicalCheckpointRecordLsn + streamCheckpointRecordSize;
    }

    // Compute the physical file write plan: _CombinedUserRecordIoBuffer -> {_WorkingIoBuffer0, _WorkingIoBuffer1}
    ULONGLONG   firstSegmentFileOffset;
    ULONGLONG   secondSegmentFileOffset;

    status = ComputePhysicalWritePlan(*_CombinedUserRecordIoBuffer, firstSegmentFileOffset, secondSegmentFileOffset);

    // _AsyncActivitiesLeft represents the number of logical parallel activities ongoing in this op instance. When
    // this count reaches zero, RelieveAsyncActivity() will cause continuation thru the Log's LSN's order
    // gate: See ContinueRecordWriteCompletion().
    //
    // The two core activities are at least one log write and the continuation code after the write(s)
    // have been started. This allows delay in calling ContinueRecordWriteCompletion() until this function
    // has finished all its work (after the writes are started). If two writes are required due to EOF wrap,
    // there is an additional activity.
    _AsyncActivitiesLeft = (secondSegmentFileOffset == 0) ? 2 : 3;
    _WriteOpFailureCount = 0;
    _WriteWrappedEOF = FALSE;

    // TEMP!!!!!!!!!!
    _DebugFileWriteOffset0 = 0;
    _DebugFileWriteLength0 = 0;
    _DebugFileWriteOffset1 = 0;
    _DebugFileWriteLength1 = 0;

    // Start 1st Write of the user record
    _DebugFileWriteOffset0 = firstSegmentFileOffset;
    _DebugFileWriteLength0 = _WorkingIoBuffer0->QuerySize();

    if (NT_SUCCESS(status))
    {
        status = log->_BlockDevice->Write(
            _LowPriorityIO ? KBlockFile::IoPriority::eBackground : KBlockFile::IoPriority::eForeground,
            firstSegmentFileOffset,
            *_WorkingIoBuffer0,
            _WriteCompletion,
            this,
            _WriteOp0);
        // Continued @ LogFileWriteCompletion() and
        // OnContinueUserRecordWrite()
    }

    if (!NT_SUCCESS(status))
    {
        // Restore LSN and free space state of the LOG and Stream - see #2 and #3 above
        log->_NextLsnToWrite = savedLogNextLsnToWrite;
        log->_LogFileFreeSpace += totalLsnSpaceNeeded;
        _StreamInfo.HighestLsn = savedStreamInfoHighestLsn;
        _StreamInfo.NextLsn = savedStreamInfoNextLsn;
        _StreamInfo.LowestLsn  = savedStreamInfoLowestLsn;
        KInvariant(_Log->_LogFileFreeSpace == (_Log->_LogFileLsnSpace -
                                            _Log->_LogFileReservedSpace -
                                            (_Log->_NextLsnToWrite.Get() - _Log->_LowestLsn.Get())));

        if (physicalCheckPointWriteOp)
        {
            physicalCheckPointWriteOp->CancelPrepareForPhysicalCheckPointWrite();
        }
        if (checkPointRecordWriteOp)
        {
            checkPointRecordWriteOp->CancelPrepareForCheckPointRecordWrite();
        }

        RvdLogLsn highestLsnRemoved = _StreamDesc.LsnIndex.RemoveHighestLsnRecord();  // reverse #1: AddHigherLsnRecord()
        KInvariant(_AssignedLsn == highestLsnRemoved);

        _AsyncActivitiesLeft = 0;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoUserRecordWriteComplete(status);
        return STATUS_SUCCESS;
    }

    _IsSavedAsnIndexEntryValid = FALSE;
    _TryDeleteAsnIndexEntryOnCompletion = FALSE;
    //** END: Stage 4

    //** START: Stage 5 (two forks)

    //*** NOTE: Any I/O failures after this point kills the log file - must return a non-STATUS_SUCCESS.
    //          Also once at this point, all continuation must go thru the log's
    //          _CompletingStreamWriteLsnOrderedGate (via RelieveAsyncActivity()).
    if (secondSegmentFileOffset != 0)
    {
        // Wrapped write - fire off second write at the front of the file
        _WriteWrappedEOF = TRUE;

        _DebugFileWriteOffset1 = secondSegmentFileOffset;
        _DebugFileWriteLength1 = _WorkingIoBuffer1->QuerySize();
        NTSTATUS status1 = log->_BlockDevice->Write(
            _LowPriorityIO ? KBlockFile::IoPriority::eBackground : KBlockFile::IoPriority::eForeground,
            secondSegmentFileOffset,
            *_WorkingIoBuffer1,
            _WriteCompletion,
            this,
            _WriteOp1);

        if (!NT_SUCCESS(status1))
        {
            KTraceFailedAsyncRequest(status1, this, 0, 0);
            InterlockedIncrement(&_WriteOpFailureCount);

            if (physicalCheckPointWriteOp)
            {
                physicalCheckPointWriteOp->CancelPrepareForPhysicalCheckPointWrite();
            }
            if (checkPointRecordWriteOp)
            {
                checkPointRecordWriteOp->CancelPrepareForCheckPointRecordWrite();
            }

            // Allow continuation @ OnUserRecordWriteCompletion()
            RelieveAsyncActivity();
            return status1;
        }
        // Continued @ LogFileWriteCompletion() and OnContinueUserRecordWrite()
        //** START: Stage 5 (third fork)
    }

    // NOTE: We still have exclusive control of the Log's physical write path and LSN space at this point
    _StreamDesc.MaxAsnWritten.SetIfLarger(_RecordAsn);

    if (physicalCheckPointWriteOp)
    {
        // We have a physical checkpoint to write - schedule it

        // Update Checkpoint Stream's LSN spatial info
        log->_CheckPointStream->Info.HighestLsn = highestLsnGeneratedByPhysLogOp;
        log->_CheckPointStream->Info.NextLsn = physicalCheckpointRecordLsn + overallPhysicalCheckpointRecordSize;

        // Should not get here with an LSN offset == RvdLogLsn::Min()
        KInvariant(highestLsnGeneratedByPhysLogOp > RvdLogLsn::Min());
        KInvariant(log->_CheckPointStream->Info.LowestLsn != RvdLogLsn::Null());

        // Fire off physical checkpoint to write(s)
        KInvariant(_QuantaAcquired >= overallPhysicalCheckpointRecordSize);
        physicalCheckPointWriteOp->StartPhysicalCheckPointWrite(this, _SubCheckpointWriteCompletion);
    }

    // Record the next higher LSN being written
    KInvariant(_AssignedLsn > _StreamDesc.HighestLsn);
    _StreamDesc.HighestLsn = _AssignedLsn;

    if (checkPointRecordWriteOp)
    {
        // We have a checkpoint record to write on this stream - schedule it
        BOOLEAN     doingMultiSegmentCPWrite =
            (streamCheckpointRecordSize > RvdLogUserStreamCheckPointRecordHeader::GetMaxSinglePartRecordSize(log->_Config));

        _WroteMultiSegmentStreamCheckpoint = doingMultiSegmentCPWrite;  // For test support

        KInvariant(_QuantaAcquired >= streamCheckpointQuanta);
        checkPointRecordWriteOp->StartCheckPointRecordWrite(this, _SubCheckpointWriteCompletion);
        _StreamDesc.LastCheckPointLsn = _StreamDesc.HighestLsn;


        // Insert last segment of the multisegment stream checkpoint instead of the first
        ULONG maxMultiPartRecordSize = RvdLogUserStreamCheckPointRecordHeader::GetMaxMultiPartRecordSize(log->_Config);
        ULONG numberCPRecords = (streamCheckpointRecordSize + maxMultiPartRecordSize - 1) /
                                maxMultiPartRecordSize;

        ULONG highestCPRecordSize = streamCheckpointRecordSize - ((numberCPRecords-1) * maxMultiPartRecordSize);

        if (!NT_SUCCESS(_StreamDesc.LsnIndex.AddHigherLsnRecord(
                        highestLogicalCheckpointRecordLsn,
                        highestCPRecordSize,
                        0)))
        {
            // This can't happen because an entry was guarenteed by the call to LsnIndex.GuarenteeAddTwoHigherRecords()
            // above.
            KInvariant(FALSE);
        }
    }

    // This quanta (_QuantaAcquired) will be released back to the log's available write quota gate
    // during completion.
    KInvariant(_QuantaAcquired >= _SizeToWrite);

    // Allow continuation @ OnUserRecordWriteCompletion()
    RelieveAsyncActivity();
    // NOTE: After this call - 'this' can't be assumed to be valid because this thread is NOT executing
    //       within this KAsync's appt

    // Record write(s) are scheduled; let the next stream writes be scheduled for this log file
    // (by returning STATUS_SUCCESS).
    //
    // NOTE: If a multi-segment stream CP write was started, the next stream writes will be held until
    //       the last segment for that CP write is scheduled.
    return STATUS_SUCCESS;
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::OnUserRecordWriteCompletion()
//
//  Continued from LogFileWriteCompletion() or ContinueRecordWrite()
{
    KInvariant(_Type == AsyncWriteStream::Type::UserWriteOp);
    KInvariant(_Log->_CompletingStreamWriteLsnOrderedGateDispatched);

    //** START: Stage 6
    if (_WriteOpFailureCount > 0)
    {
        _AsnIndex->UpdateDisposition(_AsnIndexEntry, _Version, RvdLogStream::RecordDisposition::eDispositionNone);

        // Block all further stream writes once the log file has faulted.
        KTraceFailedAsyncRequest(K_STATUS_LOG_STRUCTURE_FAULT, this, 0, 0);
        DoUserRecordWriteComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return K_STATUS_LOG_STRUCTURE_FAULT;
    }

    // A user log record has been successfully written - update AsnIndex to indicate that and complete
    // the common physical log book-keeping wrt highest completed LSN information
    _AsnIndex->UpdateDisposition(_AsnIndexEntry, _Version, RvdLogStream::RecordDisposition::eDispositionPersisted);
    #if DBG
        // Un-record "this" for an active log stream write
        _AsnIndexEntry->DebugContext = 0;
    #endif

    CommonRecordWriteCompletion();
    //** END: Stage 6

    //** START: Stage 7
    DoUserRecordWriteComplete(STATUS_SUCCESS);

    // Release log's LSN OrderedGate
    return STATUS_SUCCESS;
    //** END: Stage 7
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnCompletedUserRecordWrite()         // Called from OnCompleted()
{
    if (_QuantaAcquired > 0)
    {
        _Log->_StreamWriteQuotaGate->ReleaseQuanta(_QuantaAcquired);
        _QuantaAcquired = 0;
    }

    if (_CurrentLogSize != nullptr)
    {
        KAssert(_CurrentLogSpaceRemaining != nullptr);

        ULONGLONG totalSpace;
        ULONGLONG freeSpace;
        ULONGLONG usedSpace;


        _Log->QuerySpaceInformation(
            &totalSpace,
            &freeSpace);

        //
        // Core logger always reserves 2 records in case it has checkpoint records. This is not reflected in
        // the freeSpace amount returned and so must be accounted for here
        //
        freeSpace -= _Log->QueryReservedSpace();

        usedSpace = totalSpace - freeSpace;

        *_CurrentLogSize = usedSpace;
        *_CurrentLogSpaceRemaining = freeSpace;
    }
}

VOID
RvdLogStreamImp::AsyncWriteStream::SubCheckpointWriteCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
{
    NTSTATUS status = CompletingSubOp.Status();
    CompletingSubOp.Reuse();

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &CompletingSubOp, (ULONGLONG)Parent, 0);
    }
}

VOID
RvdLogStreamImp::AsyncWriteStream::ForcedCheckpointWriteCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
//  Continued from OnContinueUserRecordWrite()
{
    UNREFERENCED_PARAMETER(Parent);

    KInvariant(_ForcedCheckpointWrite);
    KInvariant(Parent == this);

    NTSTATUS status = CompletingSubOp.Status();
    CompletingSubOp.Reuse();

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &CompletingSubOp, 0, 0);
        DoUserRecordWriteComplete(status);
        return;
    }

    DoUserRecordWriteComplete(status);
}
#pragma endregion AsyncWriteStream::Type::UserWriteOp logic


#pragma region AsyncWriteStream::Type::TruncateWriteOp logic
//** AsyncWriteStream::Type::TruncateWriteOp logic

VOID
RvdLogStreamImp::AsyncWriteStream::StartTruncation(
    __in BOOLEAN ForceTruncation,
    __in RvdLogLsn LsnTruncationPointForStream,
    __in RvdLogManagerImp::RvdOnDiskLog::SPtr& Log,             // This is passed because TruncateWriteOps don't keep
    __in_opt KAsyncContextBase* const ParentAsyncContext,       // ia sptr ref to its Log while its not an active operation.
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    KInvariant(_Type == AsyncWriteStream::Type::TruncateWriteOp);
    KInvariant(!_Log);
    KInvariant(Status() == K_STATUS_NOT_STARTED);
    KInvariant(_AsyncActivitiesLeft == 0);

    _Log = Log;
    _LsnTruncationPointForStream = LsnTruncationPointForStream;
    _ForceTruncation = ForceTruncation;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogStreamImp::AsyncWriteStream::DoTruncationComplete(__in NTSTATUS Status)
{
    KInvariant(_Type == AsyncWriteStream::Type::TruncateWriteOp);

    DoCommonComplete(Status);
}

VOID
RvdLogStreamImp::AsyncWriteStream::ReleaseTruncateActivity()
{
    KInvariant(_AsyncActivitiesLeft > 0);
    if (InterlockedDecrement(&_AsyncActivitiesLeft) == 0)
    {
        TruncationCompleted();
    }
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnStartTruncation()
{
    KInvariant(_Type == AsyncWriteStream::Type::TruncateWriteOp);
    KInvariant(_QuantaAcquired == 0);

    //* The physical log truncation process is broken into the following statges:
    //
    // Stage 1: Acquire a Quota for the worst case sized physical log record and header (4K)
    //
    // Stage 2: Acquire the log LSN space assignment queue via EnqueueStreamWriteOp(). This
    //          results in this instance owning LSN and physical write ordering - just like
    //          a Type::UserWriteOp operation. Use _Log->_MaxCheckPointLogRecordSize.

    ULONGLONG quantaToAcquire;

    BOOLEAN useSparseFile = ((_Log->_CreationFlags & RvdLogManager::AsyncCreateLog::FlagSparseFile) == RvdLogManager::AsyncCreateLog::FlagSparseFile) ? TRUE : FALSE;
    if (useSparseFile)
    {
        //
        // Acquire the entire region of chaos so that we get to a point where no more writes are being issued and all
        // are drained.
        //
        quantaToAcquire = _Log->_Config.GetMaxQueuedWriteDepth();
    }
    else
    {
        // Acquire max quanta from log level allowed outstanding write quota gate - max of MaxQueuedWriteDepth.
        KInvariant(_Log->_MaxCheckPointLogRecordSize <= _Log->_Config.GetMaxQueuedWriteDepth());
        quantaToAcquire = _Log->_MaxCheckPointLogRecordSize;
    }

    _QuantaAcquireOp->StartAcquire(quantaToAcquire, this, _QuantaAcquiredCompletion);

    // Continued @ OnTruncationQuantaAcquired
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnTruncationQuantaAcquired()
//  Continued from OnStartTruncation()
{
    NTSTATUS status = _QuantaAcquireOp->Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoTruncationComplete(status);
        return;
    }

    //** Worst case Quanta has been acquired the log's max outstanding write quota - record it
    KInvariant(_QuantaAcquired == 0);
    _QuantaAcquired = _QuantaAcquireOp->GetDesiredQuanta();
    _QuantaAcquireOp->Reuse();

    status = _Log->EnqueueStreamWriteOp(*this);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoTruncationComplete(status);
        return;
    }
    // continued @OnContinueTruncation() once this instance owns the LSN spatial control
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::OnContinueTruncation()
//  Continued from OnTruncationQuantaAcquired()
{
    KInvariant(_Type == AsyncWriteStream::Type::TruncateWriteOp);
    KInvariant(_Log->IsIncomingStreamWriteOpQueueDispatched());

    // must be below any LSN space ever used by the stream
    KInvariant(_LsnTruncationPointForStream <= _StreamInfo.NextLsn);

    // Phase 3: The LSN space for the log is protected - this continuance owns the write op stream. Allocate the
    //          CP record (optionally) and adjust all impacted LSN spatial markers (aka cursors) as needed.

    // We always truncate the current stream's LSN space here (now) and via the LsnIndex.Truncate() call below (after
    // the optional CP write is scheduled). The the actual CP record will only be attempted if this given stream level
    // truncation would actaually result in a gain in free LSN space.
    //
    // In the failure case where a stream in memory state is update and CP is not written, recovery will
    // via its own truncation after both phys and stream level recovery - the worst case is over recovery which can always
    // happen.
    _StreamInfo.LowestLsn.SetIfLarger(_LsnTruncationPointForStream);
    if (_StreamInfo.LowestLsn > _StreamInfo.HighestLsn)
    {
        // All records in this stream need to be truncated.
        KInvariant(_StreamInfo.LowestLsn == _StreamInfo.NextLsn);
        _StreamInfo.HighestLsn = _StreamInfo.LowestLsn;
    }

    // Trim all space from the log's checkpoint stream - we are going to write
    // another up-to-date physical cp record into that stream as part of this
    // truncate op but want to make sure that physical truncation of that cp stream
    // is considered in the following max stream phy space used scan (ComputeLowestUsedLsn).
    // This implies that after the CP record is written below or this change reversed, the
    // CP stream can have up to two CP records in it.
    RvdLogLsn   savedCPLowestLSN = _Log->_CheckPointStream->Info.LowestLsn;
    _Log->_CheckPointStream->Info.LowestLsn = _Log->_CheckPointStream->Info.HighestLsn;

    RvdLogLsn newLowestUsedLsn = _Log->ComputeLowestUsedLsn();
    KInvariant(newLowestUsedLsn < RvdLogLsn::Max());

    ULONGLONG spaceFreed = newLowestUsedLsn.Get() - _Log->_LowestLsn.Get();
    ULONGLONG maxCPSizeOnDisk = RvdLogPhysicalCheckpointRecord::ComputeMaxSizeOnDisk(_Log->_Config);

    if ((_ForceTruncation) || (spaceFreed >= maxCPSizeOnDisk))
    {
        // Net gain would occur by writing a CP or a checkpoint is
        // needed due to a stream delete - attempt it
        ULONGLONG expectedFreeSpace = _Log->_LogFileFreeSpace + spaceFreed;

        if (expectedFreeSpace < maxCPSizeOnDisk)
        {
            // Log full even when allowing use of the CP reserve - not good!!!
            _Log->_CheckPointStream->Info.LowestLsn = savedCPLowestLSN;
            if (!_Log->_DebugDisableAssertOnLogFull)
            {
                KInvariant(FALSE);
            }

            KTraceFailedAsyncRequest(STATUS_LOG_FULL, this, 0, 0);
            DoTruncationComplete(STATUS_LOG_FULL);
            return STATUS_SUCCESS;
        }

        //** No failures beyond this point

#ifdef EXTRA_TRACING
        KDbgCheckpointWDataInformational((KActivityId)(_StreamInfo.LogStreamId.Get().Data1), "TRUNC** PerformCheckpoint", STATUS_SUCCESS,
                            (ULONGLONG)spaceFreed,
                            (ULONGLONG)_Log->_LowestLsn.Get(),
                            (ULONGLONG)newLowestUsedLsn.Get(),
                            (ULONGLONG)_Log->_NextLsnToWrite.Get());
#endif
        
        // Truncate log's LSN space (in-memory state)
        _Log->_LowestLsn = newLowestUsedLsn;
        KInvariant(_Log->_LowestLsn <= _Log->_NextLsnToWrite);

        _Log->_LogFileFreeSpace += spaceFreed;
        KInvariant(_Log->_LogFileFreeSpace == (_Log->_LogFileLsnSpace -
                                            _Log->_LogFileReservedSpace -
                                            (_Log->_NextLsnToWrite.Get() - _Log->_LowestLsn.Get())));

        if (_Log->_DebugDisableTruncateCheckpointing)
        {
            _AsyncActivitiesLeft = 1;
        }
        else
        {
            NTSTATUS status;

            BOOLEAN useSparseFile = ((_Log->_CreationFlags & RvdLogManager::AsyncCreateLog::FlagSparseFile) ==
                                     RvdLogManager::AsyncCreateLog::FlagSparseFile) ? TRUE : FALSE;

            // Prep for the CP record write and start the IO continuation fork
            ULONGLONG sizeOfCheckpointRecord;
            RvdLogLsn checkpointRecordLsn;

            // NOTE: sizeOfCheckpointRecord will reflect the physical checkpoint record overhead in the _CheckPointStream
            _TruncateCheckPointOp->PrepareForPhysicalCheckPointWrite(
                    *_Log,
                    _Log->_NextLsnToWrite,
                    _Log->_HighestCompletedLsn,
                    _Log->_CheckPointStream->Info.HighestLsn,        // LastCheckPointLsn
                    _Log->_CheckPointStream->Info.HighestLsn,        // PrevLsnInLogStream
                    sizeOfCheckpointRecord,
                    checkpointRecordLsn);
            KInvariant(_Log->_NextLsnToWrite == checkpointRecordLsn);
            KInvariant(sizeOfCheckpointRecord < _Log->_LogFileFreeSpace);

            // Trim reserved quota to reflect only sizeOfCheckpointRecord.
            KInvariant(sizeOfCheckpointRecord <= _QuantaAcquired);

            if (! useSparseFile)
            {
                //
                // If this is not a sparse file then we can give back
                // the quanta we are not using. If it is a sparse file
                // then we need to keep all of the quanta to ensure
                // there is nothing that races ahead and writes to an
                // area we are trimming.
                //
                _Log->_StreamWriteQuotaGate->ReleaseQuanta(_QuantaAcquired - sizeOfCheckpointRecord);
                _QuantaAcquired = sizeOfCheckpointRecord;
            }

            //* Update the physical checkpoint stream valid LSN upper limit
            _Log->_CheckPointStream->Info.HighestLsn = _Log->_NextLsnToWrite;
            _Log->_CheckPointStream->Info.NextLsn = _Log->_NextLsnToWrite + sizeOfCheckpointRecord;

            //* Now account for the planned physical checkpoint write in the log's LSN space
            _Log->_NextLsnToWrite   += sizeOfCheckpointRecord;
            _Log->_LogFileFreeSpace -= sizeOfCheckpointRecord;
            KInvariant(_Log->_LogFileFreeSpace == (_Log->_LogFileLsnSpace -
                                                   _Log->_LogFileReservedSpace -
                                                   (_Log->_NextLsnToWrite.Get() - _Log->_LowestLsn.Get())));


            // We are forking this continuation into at least 2 forks: the CP write and some continued logic after that
            // op is kicked off. We may also have forks for trimming.
            // Note that in the case of a ForceTruncation (Stream
            // delete) we do not want to trim.
            _AsyncActivitiesLeft = 2;

            if (useSparseFile)
            {
                //
                // Perform any sparse file trimming that we can. Note
                // that we ignore any errors on trim since trimming is
                // optional. However an improvement would be to
                // determine if the failure is transient or not and if
                // not then force fail the log.
                //
                _Log->ComputeUnusedFileRanges(_TrimFrom1, _TrimTo1, _TrimFrom2, _TrimTo2);

                if ((_TrimFrom1 != 0) && (_TrimTo1 != 0))
                {
                    _AsyncActivitiesLeft++;
                    // Start the Trim - Continued @ TruncateTrimComplete
                    status = _Log->_BlockDevice->Trim(_TrimFrom1, _TrimTo1, _TruncateTrimCompletion, this, NULL);
                    if (!NT_SUCCESS(status))
                    {
                        KTraceFailedAsyncRequest(status, this, _TrimFrom1, _TrimTo1);
                        ReleaseTruncateActivity();
                    }
                }

                if ((_TrimFrom2 != 0) && (_TrimTo2 != 0))
                {
                    _AsyncActivitiesLeft++;
                    // Start the Trim - Continued @ TruncateTrimComplete
                    status = _Log->_BlockDevice->Trim(_TrimFrom2, _TrimTo2, _TruncateTrimCompletion, this, NULL);
                    if (!NT_SUCCESS(status))
                    {
                        KTraceFailedAsyncRequest(status, this, _TrimFrom2, _TrimTo2);
                        ReleaseTruncateActivity();
                    }
                }
            }

            // Start the CP write - Continued @ TruncateCheckPointWriteComplete
            _TruncateCheckPointOp->StartPhysicalCheckPointWrite(this, _TruncateCheckPointWriteCompletion);
        }
    }
    else
    {
        // No LSN space will be freed - reverse CP lowest LSN change and release Quanta held
        _AsyncActivitiesLeft = 1;       // only the compute continuation fork used
        _Log->_StreamWriteQuotaGate->ReleaseQuanta(_QuantaAcquired);
        _QuantaAcquired = 0;

        _Log->_CheckPointStream->Info.LowestLsn = savedCPLowestLSN;
    }

    // Phase 4: Write (opt) and Continuing compute paths fork

    // Truncate LSN tracker state
    _StreamDesc.LsnIndex.Truncate(_LsnTruncationPointForStream);

    ReleaseTruncateActivity();
    return STATUS_SUCCESS;
}

VOID
RvdLogStreamImp::AsyncWriteStream::TruncateCheckPointWriteComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);

#ifdef EXTRA_TRACING
    KDbgCheckpointWDataInformational((KActivityId)(_Log->_LogId.Get().Data1), "CompleteCPRecord", CompletingSubOp.Status(),
                        (ULONGLONG)0,
                        (ULONGLONG)0,
                        (ULONGLONG)0,
                        (ULONGLONG)_Log->_NextLsnToWrite.Get());
#endif

    KInvariant(Parent == this);

    if (!NT_SUCCESS(CompletingSubOp.Status()))
    {
        KTraceFailedAsyncRequest(CompletingSubOp.Status(), &CompletingSubOp, 0, 0);
    }
    CompletingSubOp.Reuse();

    ReleaseTruncateActivity();
}

VOID
RvdLogStreamImp::AsyncWriteStream::TruncateTrimComplete(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);

    // continued from OnContinueTruncation

    KInvariant(Parent == this);

    if (!NT_SUCCESS(CompletingSubOp.Status()))
    {
        KTraceFailedAsyncRequest(CompletingSubOp.Status(), &CompletingSubOp, _TrimFrom1, _TrimTo1);
        KTraceFailedAsyncRequest(CompletingSubOp.Status(), &CompletingSubOp, _TrimFrom2, _TrimTo2);
    }
    CompletingSubOp.Reuse();

    ReleaseTruncateActivity();
}

VOID
RvdLogStreamImp::AsyncWriteStream::TruncationCompleted()
{
    KInvariant(_AsyncActivitiesLeft == 0);

    // Phase 5: Write and Compute forks are joined - complete the overall truncate op
    DoTruncationComplete(STATUS_SUCCESS);
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnCompletedTruncation()              // Called from OnCompleted()
{
    if (_QuantaAcquired > 0)
    {
        _Log->_StreamWriteQuotaGate->ReleaseQuanta(_QuantaAcquired);
        _QuantaAcquired = 0;
    }

    _Log.Reset();
}

#pragma endregion AsyncWriteStream::Type::TruncateWriteOp logic


#pragma region AsyncWriteStream::Type::LogicalCheckPointWriteOp logic
//** AsyncWriteStream::Type::LogicalCheckPointWriteOp logic

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::PrepareForCheckPointRecordWrite(
    __in RvdLogManagerImp::RvdOnDiskLog& Log,
    __in RvdLogLsn WriteAt,
    __in RvdLogLsn HighestCompletedLsn,
    __in RvdLogLsn LastCheckPointStreamLsn,
    __in RvdLogLsn PreviousLsnInStream,
    __out ULONG& ResultingLsnSpace,
    __out RvdLogLsn& HighestRecordLsnGenerated)
{
    KInvariant(_Type == AsyncWriteStream::Type::LogicalCheckPointWriteOp);
    KInvariant(Status() == K_STATUS_NOT_STARTED);
    KInvariant(_Log == nullptr);
    KInvariant(!_IsInputOpQueueSuspended);
    KInvariant(_AsyncActivitiesLeft == 0);

    // Snap the data for the operation - iif ok stuff params so that StartCheckPointRecordWrite()
    // can start the actual op.
    _Log = &Log;
    _AssignedLsn = WriteAt;
    _WriteOpFailureCount = 0;

    // Capture the starting LSN space info for this write
    _CheckPointHighestCompletedLsn = HighestCompletedLsn;
    _CheckPointLastCheckPointStreamLsn = LastCheckPointStreamLsn;
    _CheckPointPreviousLsnInStream = PreviousLsnInStream;

    InterlockedIncrement(&_Log->_CountOfOutstandingWrites);     // #1

    ULONG       recordIoBufferSize;
    NTSTATUS    status = _StreamDesc.BuildStreamCheckPointRecordTables(
        GetThisAllocator(),
        *_CommonCheckpointRecordIoBuffer,
        _Log->_Config,
        recordIoBufferSize);

    if (NT_SUCCESS(status))
    {
        // Compute the work to be done
        _SizeToWrite = RvdDiskLogConstants::RoundUpToBlockBoundary(recordIoBufferSize);
        KInvariant(_SizeToWrite <= _CommonCheckpointRecordIoBuffer->QuerySize());
        ResultingLsnSpace = _SizeToWrite;

        // Compute the number of segments to write
        _SegmentsToWrite =
            (_SizeToWrite <= RvdLogUserStreamCheckPointRecordHeader::GetMaxSinglePartRecordSize(_Log->_Config))
                ? 1
                : (_SizeToWrite / RvdLogUserStreamCheckPointRecordHeader::GetMaxMultiPartRecordSize(_Log->_Config)) +
                    (((_SizeToWrite % RvdLogUserStreamCheckPointRecordHeader::GetMaxMultiPartRecordSize(_Log->_Config)) > 0)
                            ? 1
                            : 0);

        _IsInputOpQueueSuspended = (_SegmentsToWrite > 1);
        if (_IsInputOpQueueSuspended)
        {
            // In the case where we have a multi-seg write, all incoming ops are held at the log's StreamWriteOp queue until
            // this op is either canceled (CancelPrepareForCheckPointRecordWrite) or the last seg-write op is scheduled. This
            // approach allows the last seg-write to overlap incoming processing and keep the disk from starving in a loaded
            // situation.
            _Log->SuspendIncomingStreamWriteDequeuing();        // #2
        }

        HighestRecordLsnGenerated =
            _AssignedLsn.Get() +
            ((_SegmentsToWrite - 1) * RvdLogUserStreamCheckPointRecordHeader::GetMaxMultiPartRecordSize(_Log->_Config));
    }

    if (!NT_SUCCESS(status))
    {
        InterlockedDecrement(&_Log->_CountOfOutstandingWrites);     // undo #1
        ResultingLsnSpace = 0;
        _SizeToWrite = 0;
        _CommonCheckpointRecordIoBuffer->Clear();
        _Log.Reset();
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }

    return status;
}

VOID
RvdLogStreamImp::AsyncWriteStream::CancelPrepareForCheckPointRecordWrite()
{
    KInvariant(_Type == AsyncWriteStream::Type::LogicalCheckPointWriteOp);
    KInvariant(_Log);

    InterlockedDecrement(&_Log->_CountOfOutstandingWrites);
    _SizeToWrite = 0;

    if (_IsInputOpQueueSuspended)
    {
        // Release the LSN-space protection queue lock
        _Log->ResumeIncomingStreamWriteDequeuing();     // undo #2 in PrepareForCheckPointRecordWrite()
        _IsInputOpQueueSuspended = FALSE;
    }

    _Log.Reset();
}

VOID
RvdLogStreamImp::AsyncWriteStream::StartCheckPointRecordWrite(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    KInvariant(_Type == AsyncWriteStream::Type::LogicalCheckPointWriteOp);
    KInvariant(_Log);

    // Start scheduled write of logical checkpoint record (sequence) - record the
    // highest segment's base LSN that will be written in that sequence
    //
    // NOTE: This LSN-update MUST be done before control is returned to the caller as
    //       the data structures (_StreamDesc) will not be protected beyond that point
    //       as the LSN space protection gate can be released once that caller's thread
    //       is given up.
    KInvariant(_AssignedLsn > _StreamDesc.HighestLsn);
    _StreamDesc.HighestLsn =
        _AssignedLsn.Get() +
        ((_SegmentsToWrite - 1) * RvdLogUserStreamCheckPointRecordHeader::GetMaxMultiPartRecordSize(_Log->_Config));

    KInvariant(((_SegmentsToWrite - 1) * RvdLogUserStreamCheckPointRecordHeader::GetMaxMultiPartRecordSize(_Log->_Config)) <
                _SizeToWrite);

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogStreamImp::AsyncWriteStream::DoCheckPointRecordWriteComplete(__in NTSTATUS Status)
{
    KInvariant(_Type == AsyncWriteStream::Type::LogicalCheckPointWriteOp);
    KInvariant(!_IsInputOpQueueSuspended);

    InterlockedDecrement(&_Log->_CountOfOutstandingWrites);

    if (_SizeToWrite < (_CommonCheckpointRecordIoBuffer->QuerySize() / 2))
    {
        // In the normal case of repeated use of this instance for writing stream CPs into the log we
        // use a simple stratagy where we keep _CommonCheckpointRecordIoBuffer (and add memory to it
        // as needed). But if demand srinks down below 1/2 its current size, it is cleared and will
        // be exactly allocated on the next CP write. In practice this should reduce heap thrashing.
        _CommonCheckpointRecordIoBuffer->Clear();
    }

    _Log.Reset();
    DoCommonComplete(Status);
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnCheckPointRecordWriteStart()
//
// Continued from StartCheckPointRecordWrite()
{
    KInvariant(_Type == AsyncWriteStream::Type::LogicalCheckPointWriteOp);
    KInvariant(IsInApartment());

    _WriteOpFailureCount = 0;
    _NextSegmentToWrite = 0;
    _SegmentsWritten = 0;
    ScheduleNextSegmentWrite(*_CheckpointSegmentWriteOp0);

    if (_SegmentsToWrite > 1)
    {
        ScheduleNextSegmentWrite(*_CheckpointSegmentWriteOp1);
    }

    // Record write(s) are scheduled...
    // Continued @ OnCheckPointRecordWriteCompletion()
}

VOID
RvdLogStreamImp::AsyncWriteStream::ScheduleNextSegmentWrite(__in AsyncWriteStream& SegWriteOp)
{
    KInvariant(_Type == AsyncWriteStream::Type::LogicalCheckPointWriteOp);
    KInvariant(SegWriteOp._Type == AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp);
    KInvariant(_NextSegmentToWrite < _SegmentsToWrite);

    ULONG const     segmentsLeftToWrite = _SegmentsToWrite - _NextSegmentToWrite;

    // Compute the next record segment's size and location info
    ULONG const     nextSegWriteOffset =
                        _NextSegmentToWrite * RvdLogUserStreamCheckPointRecordHeader::GetMaxMultiPartRecordSize(_Log->_Config);

                    KInvariant(nextSegWriteOffset < _SizeToWrite);

    ULONG const     segSizeToWrite = (segmentsLeftToWrite == 1)
                                        ? (_SizeToWrite - nextSegWriteOffset)
                                        : RvdLogUserStreamCheckPointRecordHeader::GetMaxMultiPartRecordSize(_Log->_Config);

    RvdLogLsn const     nextLsn(_AssignedLsn.Get() + nextSegWriteOffset);

    SegWriteOp.StartCheckPointRecordSegmentWrite(
        *_Log,
        nextLsn,
        _CommonCheckpointRecordIoBuffer,
        nextSegWriteOffset,
        segSizeToWrite,
        _CheckPointHighestCompletedLsn,
        _CheckPointLastCheckPointStreamLsn,
        _CheckPointPreviousLsnInStream,
        this,
        _OnCheckpointSegmentWriteCompletion);

    // Setup for next segment write
    _CheckPointPreviousLsnInStream = nextLsn;
    _NextSegmentToWrite++;

    if ((segmentsLeftToWrite == 1) && (_IsInputOpQueueSuspended))
    {
        // Release the LSN-space protection queue locked for duration - see #2 in PrepareForCheckPointRecordWrite() -
        // in until the last segment write is scheduled. This approach allows the last seg-write to overlap
        // incoming processing once we reach this point and keep the log disk from starving in a loaded situation.
        _Log->ResumeIncomingStreamWriteDequeuing();
        _IsInputOpQueueSuspended = FALSE;
    }

    // Continued @ OnCheckpointSegmentWriteCompletion()
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnCheckpointSegmentWriteCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp)
//
// Continued from: ScheduleNextSegmentWrite()
{
    KInvariant(_Type == AsyncWriteStream::Type::LogicalCheckPointWriteOp);
    KInvariant(Parent == this);
    KInvariant(IsInApartment());

    NTSTATUS        status = CompletingSubOp.Status();
    CompletingSubOp.Reuse();

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)Parent, 0);
        _WriteOpFailureCount++;
    }

    // Compute and schedule any remaining work; complete when done
    KInvariant(_SegmentsToWrite > _SegmentsWritten);
    _SegmentsWritten++;

    if (_SegmentsWritten == _SegmentsToWrite)
    {
        // This is the last segment write to complete for the group - complete
        // this CP write Op
        DoCheckPointRecordWriteComplete((_WriteOpFailureCount > 0) ? K_STATUS_LOG_STRUCTURE_FAULT : STATUS_SUCCESS);
        return;
    }

    if (_NextSegmentToWrite == _SegmentsToWrite)
    {
        // No more segments to write
        return;
    }

    // Start the next segment write using CompletingSubOp
    AsyncWriteStream* const     nextSegWriteOp = (AsyncWriteStream*)(&CompletingSubOp);
    KInvariant(nextSegWriteOp->_Type == AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp);

    ScheduleNextSegmentWrite(*nextSegWriteOp);
    // Continued @ OnCheckpointSegmentWriteCompletion()
}


#pragma region AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp logic
//** AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp logic

VOID
RvdLogStreamImp::AsyncWriteStream::StartCheckPointRecordSegmentWrite(
    __in RvdLogManagerImp::RvdOnDiskLog& Log,
    __in RvdLogLsn WriteAt,
    __in KIoBuffer::SPtr BaseIoBuffer,
    __in ULONG BaseIoBufferOffset,
    __in ULONG SizeToWrite,
    __in RvdLogLsn HighestCompletedLsn,
    __in RvdLogLsn LastCheckPointStreamLsn,
    __in RvdLogLsn PreviousLsnInStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    KInvariant(_Type == AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp);
    KInvariant(Status() == K_STATUS_NOT_STARTED);
    KInvariant(_Log == nullptr);

    KInvariant(_AsyncActivitiesLeft == 0);

    _Log = &Log;
    _AssignedLsn = WriteAt;
    _CheckpointSegmentWriteIoBuffer = BaseIoBuffer;
    _CheckpointSegmentWriteOffset = BaseIoBufferOffset;
    _SizeToWrite = SizeToWrite;
    _WriteOpFailureCount = 0;

    // Capture the mapped LSN space for this write
    _CheckPointHighestCompletedLsn = HighestCompletedLsn;
    _CheckPointLastCheckPointStreamLsn = LastCheckPointStreamLsn;
    _CheckPointPreviousLsnInStream = PreviousLsnInStream;

    InterlockedIncrement(&_Log->_CountOfOutstandingWrites);
    Start(ParentAsyncContext, CallbackPtr);
    // Continued: OnCheckPointRecordSegmentWriteStart()
}

VOID
RvdLogStreamImp::AsyncWriteStream::DoCheckPointRecordSegmentWriteComplete(__in NTSTATUS Status)
{
    KInvariant(_Type == AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp);

    InterlockedDecrement(&_Log->_CountOfOutstandingWrites);
    _SegmentView->Clear();
    _CheckpointSegmentWriteIoBuffer.Reset();

    _Log.Reset();
    DoCommonComplete(Status);
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnCheckPointRecordSegmentWriteStart()
{
    KInvariant(_Type == AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp);

    _SegmentView->SetView(*_CheckpointSegmentWriteIoBuffer, _CheckpointSegmentWriteOffset, _SizeToWrite);

    // Finish the checkpoint record - fill in header
    RvdLogUserStreamCheckPointRecordHeader& cpRecHdr =
        *(RvdLogUserStreamCheckPointRecordHeader*)(_SegmentView->First()->GetBuffer());

    ULONG const     metadataAndHeaderSize =
        _SizeToWrite - RvdLogUserStreamCheckPointRecordHeader::ComputeRecordPaddingSize(
                                                                    _Log->_Config,
                                                                    cpRecHdr.NumberOfAsnMappingEntries,
                                                                    cpRecHdr.NumberOfLsnEntries);
    cpRecHdr.Initialize(
        RvdLogUserStreamRecordHeader::Type::CheckPointRecord,
        _Log->_LogId,
        &_Log->_LogSignature[0],
        _StreamInfo.LogStreamId,
        _StreamInfo.LogStreamType,
        _SizeToWrite,                   // Overall HeaderSize
        0,
        metadataAndHeaderSize - sizeof(RvdLogRecordHeader),
        _StreamDesc.TruncationPoint);

    CompleteHeaderAndStartWrites(*_SegmentView, metadataAndHeaderSize);

    // Record write(s) are scheduled (or error occured during write starts)
    // Continued @ OnCheckPointRecordSegmentWriteCompletion()
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::OnCheckPointRecordSegmentWriteCompletion()
//
// Continued from: OnCheckPointRecordSegmentWriteStart via RelieveAsyncActivity().
//
// This method is dispatched from the LSN OrderedGate for the Log. This
// means that the current (this) AsyncWriteStream is for the next highest LSN
// written into the physical log... Until a STATUS_SUCCESS is returned
// to the caller of this method, further write completion progress on the underlying
// Log will be held. If a failure status is retured, the Log will be marked as
// faulted.
{
    KInvariant(_Type == AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp);
    if (_WriteOpFailureCount > 0)
    {
        // Block all further stream writes once the log file has faulted.
        KTraceFailedAsyncRequest(K_STATUS_LOG_STRUCTURE_FAULT, this, 0, 0);
        DoCheckPointRecordSegmentWriteComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return K_STATUS_LOG_STRUCTURE_FAULT;
    }

    // A logical checkpoint log record has been successfully written - complete
    // the common physical log book-keeping wrt highest completed LSN information
    CommonRecordWriteCompletion();

    DoCheckPointRecordSegmentWriteComplete(STATUS_SUCCESS);
    return STATUS_SUCCESS;
}

#pragma endregion AsyncWriteStream::Type::LogicalCheckPointSegmentWriteOp logic
#pragma endregion AsyncWriteStream::Type::LogicalCheckPointWriteOp logic


#pragma region AsyncWriteStream::Type::PhysicalCheckPointWriteOp logic
//** AsyncWriteStream::Type::PhysicalCheckPointWriteOp logic

VOID
RvdLogStreamImp::AsyncWriteStream::PrepareForPhysicalCheckPointWrite(
    __in RvdLogManagerImp::RvdOnDiskLog& Log,
    __in RvdLogLsn WriteAt,
    __in RvdLogLsn HighestCompletedLsn,
    __in RvdLogLsn LastCheckPointStreamLsn,
    __in RvdLogLsn PreviousLsnInStream,
    __out ULONGLONG& ResultingLsnSpace,
    __out RvdLogLsn& HighestLsnGenerated)
{
    KInvariant(_Type == AsyncWriteStream::Type::PhysicalCheckPointWriteOp);
    KInvariant(Status() == K_STATUS_NOT_STARTED);
    KInvariant(!_Log);
    KInvariant(_AsyncActivitiesLeft == 0);

    // Capture the mapped LSN space for this write
    KInvariant(WriteAt > HighestCompletedLsn);
    KInvariant(WriteAt > LastCheckPointStreamLsn);
    KInvariant(WriteAt > PreviousLsnInStream);

    _CheckPointHighestCompletedLsn = HighestCompletedLsn;
    _CheckPointLastCheckPointStreamLsn = LastCheckPointStreamLsn;
    _CheckPointPreviousLsnInStream = PreviousLsnInStream;

    // Snap the data for the operation - if ok, stuff params so that StartPhysicalCheckPointWrite()
    // can start the actual op(s).
    _PhysicalCheckpointRecord->NumberOfLogStreams =
        Log.SnapSafeCopyOfStreamsTable(&_PhysicalCheckpointRecord->LogStreamArray[0]);

    _SizeToWrite = RvdLogPhysicalCheckpointRecord::ComputeSizeOnDisk(_PhysicalCheckpointRecord->NumberOfLogStreams);
    KInvariant(_SizeToWrite <= Log._Config.GetMaxMetadataSize());

    _AssignedLsn = WriteAt;
    ResultingLsnSpace = _SizeToWrite;

    _Log = &Log;
    _WriteOpFailureCount = 0;
    HighestLsnGenerated = _AssignedLsn;
    InterlockedIncrement(&_Log->_CountOfOutstandingWrites);
}

VOID
RvdLogStreamImp::AsyncWriteStream::CancelPrepareForPhysicalCheckPointWrite()
{
    KInvariant(_Type == AsyncWriteStream::Type::PhysicalCheckPointWriteOp);
    KInvariant(_Log);

    InterlockedDecrement(&_Log->_CountOfOutstandingWrites);
    _AssignedLsn = RvdLogLsn::Null();
    _SizeToWrite = 0;
    _Log.Reset();
}

VOID
RvdLogStreamImp::AsyncWriteStream::StartPhysicalCheckPointWrite(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    KInvariant(_Type == AsyncWriteStream::Type::PhysicalCheckPointWriteOp);
    KInvariant(_Log);

    // Start write of scheduled physical log checkpoint record
    KInvariant(_AssignedLsn > _StreamDesc.HighestLsn);
    _StreamDesc.HighestLsn = _AssignedLsn;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogStreamImp::AsyncWriteStream::DoPhysicalCheckPointWriteComplete(__in NTSTATUS Status)
{
    KInvariant(_Type == AsyncWriteStream::Type::PhysicalCheckPointWriteOp);

    InterlockedDecrement(&_Log->_CountOfOutstandingWrites);
    _CommonCheckpointRecordIoBuffer->Clear();

    _Log.Reset();
    DoCommonComplete(Status);
}

VOID
RvdLogStreamImp::AsyncWriteStream::OnStartPhysicalCheckPointWrite()
//
// Continued from StartPhysicalCheckPointWrite()
{
    KInvariant(_Type == AsyncWriteStream::Type::PhysicalCheckPointWriteOp);

    // Finish the checkpoint record - fill in header
    ULONG   metadataAndHeaderSize = _SizeToWrite - RvdLogPhysicalCheckpointRecord::ComputePaddingSize(
        _PhysicalCheckpointRecord->NumberOfLogStreams);

    _PhysicalCheckpointRecord->Initialize(
        _Log->_LogId,
        &_Log->_LogSignature[0],
        _StreamInfo.LogStreamId,
        _StreamInfo.LogStreamType,
        _SizeToWrite,                   // Overall HeaderSize
        0,
        metadataAndHeaderSize - sizeof(RvdLogRecordHeader));

    _PhysicalCheckpointRecord->Reserved1 = 0;

    KInvariant(_CommonCheckpointRecordIoBuffer->QuerySize() == 0);
    _CommonCheckpointRecordIoBuffer->AddIoBufferElement(*_PhysicalCheckpointRecordBuffer);

    CompleteHeaderAndStartWrites(*_CommonCheckpointRecordIoBuffer, metadataAndHeaderSize);

    // Record write(s) are scheduled (or error occured during write starts)
    // Continued @ OnPhysicalCheckPointWriteCompletion()
}

NTSTATUS
RvdLogStreamImp::AsyncWriteStream::OnPhysicalCheckPointWriteCompletion()
//
// Continued from: OnStartPhysicalCheckPointWrite via RelieveAsyncActivity().
//
// This method is dispatched from the LSN OrderedGate for the Log. This
// means that the current (this) AsyncWriteStream is for the next highest LSN
// written into the physical log... Until a STATUS_SUCCESS is returned
// to the caller of this method, further write completion progress on the underlying
// Log will be held. If a failure status is retured, the Log will be marked as
// faulted.
{
    KInvariant(_Type == AsyncWriteStream::Type::PhysicalCheckPointWriteOp);

    if (_WriteOpFailureCount > 0)
    {
        // Block all further stream writes once the log file has faulted.
        KTraceFailedAsyncRequest(K_STATUS_LOG_STRUCTURE_FAULT, this, 0, 0);
        DoPhysicalCheckPointWriteComplete(K_STATUS_LOG_STRUCTURE_FAULT);
        return K_STATUS_LOG_STRUCTURE_FAULT;
    }

    // A physical checkpoint log record has been successfully written - complete
    // the common physical log book-keeping wrt highest completed LSN information
    CommonRecordWriteCompletion();

    DoPhysicalCheckPointWriteComplete(STATUS_SUCCESS);
    return STATUS_SUCCESS;
}

#pragma endregion AsyncWriteStream::Type::PhysicalCheckPointWriteOp logic


#pragma region Common checkpoint-releated support methods
//** Common checkpoint-releated support methods

VOID
RvdLogStreamImp::AsyncWriteStream::CompleteHeaderAndStartWrites(
    __inout KIoBuffer&  FullRecordBuffer,
    __in ULONG       FullRecordSize)
{
    KIoBufferElement*   currentElement = FullRecordBuffer.First();
    RvdLogRecordHeader* recordHeader = (RvdLogRecordHeader*)(currentElement->GetBuffer());
    void*               currentBufferPtr = ((UCHAR*)recordHeader) + sizeof(recordHeader->LsnChksumBlock);
    ULONG               currentBufferSize = currentElement->QuerySize() - sizeof(recordHeader->LsnChksumBlock);
    ULONGLONG           chksumAccum = 0;

    recordHeader->InitializeLsnProperties(
        _AssignedLsn,                                   // Lsn of record
        _CheckPointHighestCompletedLsn,
        _CheckPointLastCheckPointStreamLsn,
        _CheckPointPreviousLsnInStream);

    // Checksum full record in FullRecordBuffer KIoBuffer
    FullRecordSize -= sizeof(recordHeader->LsnChksumBlock);
    while (FullRecordSize > 0)
    {
        ULONG todo = __min(FullRecordSize, currentBufferSize);
        chksumAccum = KChecksum::Crc64(currentBufferPtr, todo, chksumAccum);
        FullRecordSize -= todo;
        if (FullRecordSize > 0)
        {
            currentElement = FullRecordBuffer.Next(*currentElement);
            KInvariant(currentElement != nullptr);
            currentBufferPtr = (void*)currentElement->GetBuffer();
            currentBufferSize = currentElement->QuerySize();
        }
    }

    // Must chksum LsnChksumBlock last
    recordHeader->ThisBlockChecksum = KChecksum::Crc64(recordHeader, sizeof(recordHeader->LsnChksumBlock), chksumAccum);
    //* Record is ready to write

    // Compute the physical file write plan: FullRecordBuffer -> {_WorkingIoBuffer0, _WorkingIoBuffer1}
    NTSTATUS    status;
    ULONGLONG   firstSegmentFileOffset;
    ULONGLONG   secondSegmentFileOffset;

    status = ComputePhysicalWritePlan(FullRecordBuffer, firstSegmentFileOffset, secondSegmentFileOffset);

    // _AsyncActivitiesLeft represents the number of logical parallel activities ongoing in this instance.
    // When this count reaches zero, RelieveAsyncActivity() will cause continuation thru the Log's
    // LSN's ordered gate: See ContinueRecordWriteCompletion().
    _AsyncActivitiesLeft = (secondSegmentFileOffset == 0) ? 1 : 2;
    _WriteOpFailureCount = 0;

    if (NT_SUCCESS(status))
    {
        // Start 1st Write of the record
        status = _Log->_BlockDevice->Write(
            KBlockFile::IoPriority::eForeground,         // Prioritize checkpoint writes
            firstSegmentFileOffset,
            *_WorkingIoBuffer0,
            _WriteCompletion,
            this,
            _WriteOp0);
        // Continued @ LogFileWriteCompletion() and eventually On*WriteCompletion() 
    }
        
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        InterlockedIncrement(&_WriteOpFailureCount);

        // Allow continuation @ On*WriteCompletion()
        RelieveAsyncActivity();
        if (secondSegmentFileOffset != 0)
        {
            InterlockedIncrement(&_WriteOpFailureCount);        // Second wrap-around write not started
            RelieveAsyncActivity();
        }

        return;
    }

    if (secondSegmentFileOffset != 0)
    {
        // Wrapped write - fire off second write at the front of the file
        NTSTATUS status1 = _Log->_BlockDevice->Write(
            KBlockFile::IoPriority::eForeground,               // Prioritize checkpoint writes
            secondSegmentFileOffset,
            *_WorkingIoBuffer1,
            _WriteCompletion,
            this,
            _WriteOp1);

        if (!NT_SUCCESS(status1))
        {
            KTraceFailedAsyncRequest(status1, this, 0, 0);
            InterlockedIncrement(&_WriteOpFailureCount);
            RelieveAsyncActivity();             // Allow continuation @ On*WriteCompletion()
            return;
        }
    }
}
#pragma endregion Common checkpoint-releated support methods
