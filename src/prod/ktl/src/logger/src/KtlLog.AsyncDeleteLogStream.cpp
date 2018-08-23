/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLog.AsyncDeleteLogStream.cpp

    Description:
      RvdLogManagerImp::RvdOnDiskLog::AsyncDeleteLogStreamContext implementation.

    History:
      PengLi        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"


//** AsyncDeleteLogStreamContext Implementation
class AsyncDeleteLogStreamContextImp : public RvdLog::AsyncDeleteLogStreamContext
{
    K_FORCE_SHARED(AsyncDeleteLogStreamContextImp);

public:
    VOID
    StartDeleteLogStream(
        __in const RvdLogStreamId& LogStreamId,
        __in_opt KAsyncContextBase* const ParentAsyncContext, 
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

    AsyncDeleteLogStreamContextImp(RvdLogManagerImp::RvdOnDiskLog* const Log);

private:
    VOID
    OnStart();

    VOID
    DoComplete(NTSTATUS Status);

    VOID
    GateAcquireComplete(BOOLEAN IsAcquired, KAsyncLock& LockAttempted);

    VOID
    CheckpointWriteCompletion(KAsyncContextBase* const Parent, KAsyncContextBase& CompletingSubOp);

    VOID
    FinishStreamDelete();

    VOID
    ContinueAfterStreamUse();

private:
    KSharedPtr<RvdLogManagerImp::RvdOnDiskLog>  _Log;

    // Per operation state:
    RvdLogStreamImp::AsyncWriteStream::SPtr     _TruncateAndCheckpointOp;
    RvdLogStreamId                              _LogStreamId;
    KSharedPtr<RvdLogStreamImp>                 _LogStream;
    LogStreamDescriptor*                        _LogStreamInfoPtr;
    RvdLogStreamType                            _SavedStreamType;           // Used to back out a failed delete
};  


NTSTATUS 
RvdLogManagerImp::RvdOnDiskLog::CreateAsyncDeleteLogStreamContext(__out AsyncDeleteLogStreamContext::SPtr& Result)
{
    Result = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncDeleteLogStreamContextImp(this);
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

RvdLog::AsyncDeleteLogStreamContext::AsyncDeleteLogStreamContext()
{
}

RvdLog::AsyncDeleteLogStreamContext::~AsyncDeleteLogStreamContext()
{
}

AsyncDeleteLogStreamContextImp::AsyncDeleteLogStreamContextImp(RvdLogManagerImp::RvdOnDiskLog* const Log)
    : _Log(Log)
{
}

AsyncDeleteLogStreamContextImp::~AsyncDeleteLogStreamContextImp()
{
}

VOID
AsyncDeleteLogStreamContextImp::StartDeleteLogStream(
    __in const RvdLogStreamId& LogStreamId, 
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _LogStreamId = LogStreamId;
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
AsyncDeleteLogStreamContextImp::OnStart()
//
// Continued from StartDeleteLogStream
{
    if (_LogStreamId == RvdDiskLogConstants::CheckpointStreamId())
    {
        Complete(STATUS_NOT_FOUND);
        return;
    }

    NTSTATUS status = _Log->StartDeleteStream(_LogStreamId, _LogStream, _LogStreamInfoPtr);
    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    KDbgCheckpointWDataInformational(GetActivityId(),
                                     "DeleteStream Start", status,
                                     (ULONGLONG)_LogStreamId.Get().Data1,
                                     (ULONGLONG)_LogStream,
                                     (ULONGLONG)_LogStreamInfoPtr,
                                     (ULONGLONG)0);
    
    // NOTE: _LogStream may or may not reference a current RvdLogStreamImp depending if there is an "open" stream 
    //       object in use by the application when StartDeleteStream() was called above. If once does not exist, the 
    //       deletion will be done in any case. If one exists, this deletion op will schedule a callback 
    //       (ContinueAfterStreamUse) after the app releases its ref to that stream - this is not a typical use case.
    //       If _LogStream null, _LogStreamInfoPtr->State == LogStreamDescriptor::OpenState::Deleting) by 
    //       StartDeleteStream().

    // At this point access to *_LogStreamInfoPtr is guaranteed - take an async 
    // lock and check for proper _OpenStatus state on the other side of the acquire
    KAssert(_LogStreamInfoPtr != nullptr);
    AcquireLock(
        _LogStreamInfoPtr->OpenCloseDeleteLock, 
        KAsyncContextBase::LockAcquiredCallback(this, &AsyncDeleteLogStreamContextImp::GateAcquireComplete));

    // Continued @GateAcquireComplete
}

VOID
AsyncDeleteLogStreamContextImp::DoComplete(NTSTATUS Status)
{
    ReleaseLock(_LogStreamInfoPtr->OpenCloseDeleteLock);        // let next open/create/delete op in

    _LogStreamInfoPtr = nullptr;
    _LogStream.Reset();
    _TruncateAndCheckpointOp.Reset();

    KDbgCheckpointWDataInformational(GetActivityId(),
        "DeleteStream Completed", Status,
        (ULONGLONG)_LogStreamId.Get().Data1,
        (ULONGLONG)_LogStream,
        (ULONGLONG)_LogStreamInfoPtr,
        (ULONGLONG)0);
    
    Complete(Status);
}

VOID
AsyncDeleteLogStreamContextImp::GateAcquireComplete(
    BOOLEAN IsAcquired,
    KAsyncLock& LockAttempted)
//  Continued from OnStart()
{
    UNREFERENCED_PARAMETER(LockAttempted);

    if (!IsAcquired)
    {
        AcquireLock(
            _LogStreamInfoPtr->OpenCloseDeleteLock, 
            KAsyncContextBase::LockAcquiredCallback(this, &AsyncDeleteLogStreamContextImp::GateAcquireComplete));

        return;
    }

    // *_LogStreamInfoPtr->CreateOpenDeleteGate has been acquired - we may have lost the race to do the delete however
    if ((_LogStream != nullptr) && (_LogStreamInfoPtr->State == LogStreamDescriptor::OpenState::Deleting))
    {
        // Lost the race - deleted from under us - ok - reject request as delete pending
        DoComplete(STATUS_DELETE_PENDING);
        return;
    }

    KAssert((_LogStream == nullptr) || (_LogStreamInfoPtr->State == LogStreamDescriptor::OpenState::Opened));
    KAssert((_LogStream != nullptr) || (_LogStreamInfoPtr->State == LogStreamDescriptor::OpenState::Deleting));

    KAssert(_LogStreamId != RvdDiskLogConstants::CheckpointStreamId());

    // Trigger a CP write via a truncate to reflect that the stream being deleted - this form of write op is not bound
    // to an RvdLogStreamImp owner.
    NTSTATUS            status;
    status = RvdLogStreamImp::AsyncWriteStream::CreateTruncationAsyncWriteContext(
        GetThisAllocator(), 
        *_Log, 
        *_LogStreamInfoPtr, 
        _TruncateAndCheckpointOp);

    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    // Mark as deleting - setting this value is safe because at this point *_LogStreamInfoPtr is being 
    // kept alive by our state and so any other open/close/delete (logical) threads will take the same 
    // path but will block on _LogStreamInfoPtr->OpenCloseDeleteLock until the ReleaseLock below...
    // OR in DoComplete().
    _LogStreamInfoPtr->State = LogStreamDescriptor::OpenState::Deleting;

    // Mark the stream info (on disk in the CP) as not being valid from this point because it's being deleted -
    // (saving away the type to back out failes deletes).
    _SavedStreamType = _LogStreamInfoPtr->Info.LogStreamType;
    _LogStreamInfoPtr->Info.LogStreamType = RvdDiskLogConstants::DeletingStreamType();

    // Cause a write of a checkpoint record to be written this created stream - this is done via a truncate op
    _TruncateAndCheckpointOp->StartTruncation(
        TRUE,                               // This checkpoint must be written
        _LogStreamInfoPtr->Info.NextLsn,
        _Log,
        this,
        KAsyncContextBase::CompletionCallback(this, &AsyncDeleteLogStreamContextImp::CheckpointWriteCompletion));

    // Continued @ CheckpointWriteCompletion()
}

VOID
AsyncDeleteLogStreamContextImp::CheckpointWriteCompletion(
    KAsyncContextBase* const Parent, 
    KAsyncContextBase& CompletingSubOp)
//  Continued from GateAcquireComplete()
{
    UNREFERENCED_PARAMETER(Parent);

    KAssert(&CompletingSubOp == _TruncateAndCheckpointOp.RawPtr());
    NTSTATUS status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        // BUG, richhas, xxxxx, ADD TRACE: write of cp record during failed!!!

        // The write of the CP record failed - undo the delete attempt and report an error
        _LogStreamInfoPtr->Info.LogStreamType = _SavedStreamType;
        if (!_LogStream)
        {
            _LogStreamInfoPtr->State = LogStreamDescriptor::OpenState::Closed;
        }
        else
        {
            _LogStreamInfoPtr->State = LogStreamDescriptor::OpenState::Opened;
        }

        DoComplete(status);
        return;
    }

    FinishStreamDelete();
}

VOID
AsyncDeleteLogStreamContextImp::FinishStreamDelete()
//  Continued from CheckpointWriteCompletion() or GateAcquireComplete()
{
    if (!_LogStream)
    {
        // There is not an RvdLogStreamImp associated with this operation, can complete this op now.
        ReleaseLock(_LogStreamInfoPtr->OpenCloseDeleteLock);        // let next open/create/delete op in
        _Log->FinishDeleteStream(_LogStreamInfoPtr);
        _LogStreamInfoPtr = nullptr;
        _LogStream.Reset();
        _TruncateAndCheckpointOp.Reset();

        KDbgCheckpointWDataInformational(GetActivityId(),
            "DeleteStream Completed", STATUS_SUCCESS,
            (ULONGLONG)_LogStreamId.Get().Data1,
            (ULONGLONG)_LogStream,
            (ULONGLONG)_LogStreamInfoPtr,
            (ULONGLONG)0);
        
        Complete(STATUS_SUCCESS);
        return;
    }

    KDbgCheckpointWDataInformational(GetActivityId(),
        "DeleteStream SetDestructionCallback", STATUS_SUCCESS,
        (ULONGLONG)_LogStreamId.Get().Data1,
        (ULONGLONG)_LogStream,
        (ULONGLONG)_LogStreamInfoPtr,
        (ULONGLONG)0);
    
    // Cause this op to continue @ ContinueAfterStreamUse() at the time of the underlying _LogStream dtor
    _LogStream->SetStreamDestructionCallback(
        RvdLogStreamImp::StreamDestructionCallback(this, &AsyncDeleteLogStreamContextImp::ContinueAfterStreamUse));

    this->AddRef();     // #1 Hold a reference on this object for LogStreamImp::SetStreamDestructionCallback()

    // let other del/open/create ops come in - they will see delete occuring and will reject in error
    ReleaseLock(_LogStreamInfoPtr->OpenCloseDeleteLock);  

    // NOTE: Only raw Complete s/b called beyond this point.
    
    // Allow *_LogStream to destruct - will continue in this instance @ContinueAfterStreamUse at dtor time 
    // by that LogStream instance.
    _LogStream.Reset();       

    // Continued @ContinueAfterStreamUse
}

VOID
AsyncDeleteLogStreamContextImp::ContinueAfterStreamUse()
//
// Continued from GateAcquireComplete() - called by the RvdLogStreamImp::dtor
{
    _Log->FinishDeleteStream(_LogStreamInfoPtr);
    _LogStreamInfoPtr = nullptr;
    _TruncateAndCheckpointOp.Reset();

	KDbgCheckpointWDataInformational(GetActivityId(),
		"DeleteStream Completed", STATUS_SUCCESS,
		(ULONGLONG)_LogStreamId.Get().Data1,
		(ULONGLONG)_LogStream,
		(ULONGLONG)_LogStreamInfoPtr,
		(ULONGLONG)0);
	
    Complete(STATUS_SUCCESS);
    this->Release();    // Undo #1 above in AsyncDeleteLogStreamContextImp::FinishStreamDelete()
}

