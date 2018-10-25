/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLog.AsyncCreateLogStream.cpp

    Description:
      RvdLogManagerImp::RvdOnDiskLog::AsyncCreateLogStreamContext implementation.

    History:
      PengLi        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"


//** AsyncCreateLogStreamContext Implementation
class AsyncCreateLogStreamContextImp : public RvdLog::AsyncCreateLogStreamContext
{
    K_FORCE_SHARED(AsyncCreateLogStreamContextImp);

public:
    VOID
    StartCreateLogStream(
        __in const RvdLogStreamId& LogStreamId,
        __in const RvdLogStreamType& LogStreamType,
        __out RvdLogStream::SPtr& LogStream,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    AsyncCreateLogStreamContextImp(RvdLogManagerImp::RvdOnDiskLog* const Log);

private:
    VOID
    OnStart();

    VOID
    DoComplete(NTSTATUS Status);

    VOID
    GateAcquireComplete(BOOLEAN IsAcquired, KAsyncLock& Lock);

    VOID
    FinishStreamCreate();

    VOID
    RevertStreamCreate(NTSTATUS Status);

    VOID
    CheckpointWriteCompletion(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& CompletingSubOp);

    VOID
    ContinueAfterStreamUse();

private:
    RvdLogManagerImp::RvdOnDiskLog::SPtr    _Log;
    RvdLogStreamImp::AsyncWriteStream::SPtr _CheckpointWriteOp;
    NTSTATUS                                _CheckpointWriteOpStatus;

    // Per operation state:
    RvdLogStreamId                          _LogStreamId;
    RvdLogStreamType                        _LogStreamType;
    RvdLogStream::SPtr*                     _ResultLogStream;
    RvdLogStreamImp::SPtr                   _LogStream;
    LogStreamDescriptor*                    _StreamInfoPtr;
};

NTSTATUS
RvdLogManagerImp::RvdOnDiskLog::CreateAsyncCreateLogStreamContext(__out AsyncCreateLogStreamContext::SPtr& Result)
{
    Result = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncCreateLogStreamContextImp(this);
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = Result->Status();
    if (!NT_SUCCESS(status))
    {
        Result.Reset();
    }

    return status;
}

RvdLog::AsyncCreateLogStreamContext::AsyncCreateLogStreamContext()
{
}

RvdLog::AsyncCreateLogStreamContext::~AsyncCreateLogStreamContext()
{
}

AsyncCreateLogStreamContextImp::AsyncCreateLogStreamContextImp(
    RvdLogManagerImp::RvdOnDiskLog* const Log)
    :   _Log(Log)
{
}

AsyncCreateLogStreamContextImp::~AsyncCreateLogStreamContextImp()
{
}

VOID
AsyncCreateLogStreamContextImp::StartCreateLogStream(
    __in const RvdLogStreamId& LogStreamId,
    __in const RvdLogStreamType& LogStreamType,
    __out RvdLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _LogStreamId = LogStreamId;
    _LogStreamType = LogStreamType;
    _ResultLogStream = &LogStream;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
AsyncCreateLogStreamContextImp::DoComplete(NTSTATUS Status)
{
    ReleaseLock(_StreamInfoPtr->OpenCloseDeleteLock);

    _StreamInfoPtr = nullptr;       // Not valid after _LogStream.Reset
    _LogStream.Reset();
    _CheckpointWriteOp.Reset();

    Complete(Status);
}

VOID
AsyncCreateLogStreamContextImp::OnStart()
//
// Continued from StartCreateLogStream
{
    if ((_LogStreamId == RvdDiskLogConstants::CheckpointStreamId()) ||
        (_LogStreamType == RvdDiskLogConstants::CheckpointStreamType()))
    {
        // Disallow use of reserved streams
        Complete(STATUS_OBJECT_NAME_COLLISION);
        return;
    }

    NTSTATUS status = _Log->FindOrCreateStream(TRUE, _LogStreamId, _LogStreamType, _LogStream, _StreamInfoPtr);
    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    // At this point access to _LogStream is guaranteed - take a lock and check for
    // _State == Opened/Deleted on the other side of the acquire
    AcquireLock(
        _StreamInfoPtr->OpenCloseDeleteLock,
        KAsyncContextBase::LockAcquiredCallback(this, &AsyncCreateLogStreamContextImp::GateAcquireComplete));

    // Continued @GateAcquireComplete
}

VOID
AsyncCreateLogStreamContextImp::GateAcquireComplete(BOOLEAN IsAcquired, KAsyncLock& Lock)
{
   UNREFERENCED_PARAMETER(Lock);

    if (!IsAcquired)
    {
        AcquireLock(
            _StreamInfoPtr->OpenCloseDeleteLock,
            KAsyncContextBase::LockAcquiredCallback(this, &AsyncCreateLogStreamContextImp::GateAcquireComplete));

        return;
    }

    // CreateOpenDeleteGate has been acquired - we may have lost the race to do the create however
    if (_StreamInfoPtr->State == LogStreamDescriptor::OpenState::Opened)
    {
        // Lost the race - already exists
        DoComplete(STATUS_OBJECT_NAME_COLLISION);
        return;
    }

    if (_StreamInfoPtr->State == LogStreamDescriptor::OpenState::Deleting)
    {
        // Lost the race - deleted from under us
        DoComplete(STATUS_DELETE_PENDING);
        return;
    }

    // We can continue with the creation of the stream
    KAssert(_StreamInfoPtr->State == LogStreamDescriptor::OpenState::Closed);
    _StreamInfoPtr->State = LogStreamDescriptor::OpenState::Opened;

    if (_LogStreamId != RvdDiskLogConstants::CheckpointStreamId())
    {
        _CheckpointWriteOp = _new(KTL_TAG_LOGGER, GetThisAllocator()) RvdLogStreamImp::AsyncWriteStream(_LogStream.RawPtr());
        if (!_CheckpointWriteOp)
        {
            RevertStreamCreate(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }

        NTSTATUS status = _CheckpointWriteOp->Status();
        if (!NT_SUCCESS(status))
        {
            RevertStreamCreate(status);
            return;
        }

        // Cause a write of a checkpoint record to record this created stream if not creating the cp stream itself
        _CheckpointWriteOp->StartForcedCheckpointWrite(
            this,
            KAsyncContextBase::CompletionCallback(this, &AsyncCreateLogStreamContextImp::CheckpointWriteCompletion));

        // Continued @ CheckpointWriteCompletion()
        return;
    }

    FinishStreamCreate();
    // Continued @ FinishStreamCreate();
}

VOID
AsyncCreateLogStreamContextImp::CheckpointWriteCompletion(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletingSubOp)
//  Continued from GateAcquireComplete()
{
   UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        // BUG, richhas, xxxxx, ADD TRACE: write of cp record during failed!!!

        // The write of the CP record failed, cause deletion of the partially created stream
        RevertStreamCreate(status);
        return;
    }

    FinishStreamCreate();
    // Continued @ FinishStreamCreate();
}

VOID
AsyncCreateLogStreamContextImp::FinishStreamCreate()
//  Continued from CheckpointWriteCompletion() or GateAcquireComplete()
{
    (*_ResultLogStream) = Ktl::Move(reinterpret_cast<RvdLogStream::SPtr&>(_LogStream));
    DoComplete(STATUS_SUCCESS);
}

VOID
AsyncCreateLogStreamContextImp::RevertStreamCreate(NTSTATUS Status)
{
    _CheckpointWriteOpStatus = Status;
    _StreamInfoPtr->State = LogStreamDescriptor::OpenState::Deleting;

    // Cause this op to continue @ ContinueAfterStreamUse at time of the underlying _LogStream dtor
    _LogStream->SetStreamDestructionCallback(
        RvdLogStreamImp::StreamDestructionCallback(this, &AsyncCreateLogStreamContextImp::ContinueAfterStreamUse));

    this->AddRef();     // #1 Hold a reference on this object for LogStreamImp::SetStreamDestructionCallback()

    // let other del/open/create ops come in - they will see delete occuring and will reject in error
    ReleaseLock(_StreamInfoPtr->OpenCloseDeleteLock);
    // NOTE: Only raw Complete s/b called beyond this point.

    // Allow *_LogStream to destruct - will continue in this instance @ContinueAfterStreamUse at dtor time
    // by that LogStream instance.
    _LogStream.Reset();
	_CheckpointWriteOp.Reset();

    // Continued @ContinueAfterStreamUse
}

VOID
AsyncCreateLogStreamContextImp::ContinueAfterStreamUse()
//
// Continued from RevertStreamCreate() - can be called by the RvdLogStreamImp::dtor
{
    _Log->FinishDeleteStream(_StreamInfoPtr);
    _StreamInfoPtr = nullptr;

    Complete(_CheckpointWriteOpStatus);
    this->Release();    // Undo #1 above
}
