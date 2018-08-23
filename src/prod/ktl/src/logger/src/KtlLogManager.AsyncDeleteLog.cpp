/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLogManager.AsyncDeleteLog.cpp

    Description:
      RvdLogManager::AsyncDeleteLog implementation.

    History:
      PengLi        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"

//** DeleteLog
class RvdLogManagerImp::AsyncDeleteLogImp : public RvdLogManager::AsyncDeleteLog
{
public:
    AsyncDeleteLogImp(RvdLogManagerImp* Owner);

    VOID 
    StartDeleteLog(
        __in const KGuid& DiskId, 
        __in const RvdLogId& LogId,
        __in_opt KAsyncContextBase* const ParentAsyncContext, 
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;
	
    VOID
    StartDeleteLog(
        __in const KStringView& FullyQualifiedLogFilePath,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

    ~AsyncDeleteLogImp();

private:
    VOID 
    DeleteCompletionCallback(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& CompletingSubOp);    

    VOID
    OnStart();

    VOID
    ParseFileCompletion(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& CompletingSubOp);
	
    VOID
    OnStartContinue();

    VOID
    DoComplete(NTSTATUS Status);

private:
    RvdLogManagerImp::SPtr                  _LogManager;

    // Per operation state:
    KWString                                _FullyQualifiedLogFilePath;
    KWString                                _RootPath;
    KWString                                _RelativePath;
    BOOLEAN                                 _DoingDeleteByName;
    KGuid                                   _DiskId;
    RvdLogId                                _LogId;
    RvdLogManagerImp::RvdOnDiskLog::SPtr    _OnDiskLog;
};

NTSTATUS 
RvdLogManagerImp::CreateAsyncDeleteLogContext(__out AsyncDeleteLog::SPtr& Context)
{
    Context = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncDeleteLogImp(this);
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

RvdLogManagerImp::AsyncDeleteLog::AsyncDeleteLog()
{
}

RvdLogManagerImp::AsyncDeleteLog::~AsyncDeleteLog()
{
}

RvdLogManagerImp::AsyncDeleteLogImp::AsyncDeleteLogImp(RvdLogManagerImp* Owner)
    :   _LogManager(Owner),
        _FullyQualifiedLogFilePath(GetThisAllocator()),
        _RootPath(GetThisAllocator()),
        _RelativePath(GetThisAllocator())
{
}

RvdLogManagerImp::AsyncDeleteLogImp::~AsyncDeleteLogImp()
{
}

VOID 
RvdLogManagerImp::AsyncDeleteLogImp::StartDeleteLog(
    __in KGuid const& DiskId, 
    __in RvdLogId const& LogId,
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _DoingDeleteByName = FALSE;

    _DiskId = DiskId;
    _LogId = LogId;
    KAssert(!_OnDiskLog);

    Start(ParentAsyncContext, CallbackPtr);
}

VOID 
RvdLogManagerImp::AsyncDeleteLogImp::StartDeleteLog(
    __in const KStringView& FullyQualifiedLogFilePath,
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _DoingDeleteByName = TRUE;

    _FullyQualifiedLogFilePath = FullyQualifiedLogFilePath;
    KAssert(!_OnDiskLog);

    Start(ParentAsyncContext, CallbackPtr);
}


VOID
RvdLogManagerImp::AsyncDeleteLogImp::DoComplete(NTSTATUS Status)
{
    _LogManager->ReleaseActivity();     // Tell hosting log manager we are done with it
                                        // See OnStart()
    _OnDiskLog.Reset();
    Complete(Status);
}

VOID 
RvdLogManagerImp::AsyncDeleteLogImp::OnStart()
//
// Continuation from StartOpenLog
{
    NTSTATUS           status = STATUS_SUCCESS;

    // Acquire an activity slot (if possible) - keeps _LogManager from 
    // completing if successful - otherwise there's a shutdown condition occuring
    // in the _LogManager.
    if (!_LogManager->TryAcquireActivity())
    {
        status = K_STATUS_SHUTDOWN_PENDING;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    // NOTE: Beyond this point all completion is done thru DoComplete() to
    //       Release the acquired activity slot for this operation.

    if (_DoingDeleteByName)
    {
        status = _FullyQualifiedLogFilePath.Status();
        if (!NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            DoComplete(status);
            return;
        }

        AsyncParseLogFilename::SPtr parseAsync;

        status = AsyncParseLogFilename::Create(GetThisAllocationTag(), GetThisAllocator(), parseAsync);
        if (! NT_SUCCESS(status))
        {
            KTraceOutOfMemory(0, status, 0, 0, 0);
            DoComplete(status);
            return;
        }

        KAsyncContextBase::CompletionCallback completion(this, &RvdLogManagerImp::AsyncDeleteLogImp::ParseFileCompletion);
        parseAsync->StartParseFilename(_FullyQualifiedLogFilePath,
                                       _DiskId,
                                       _RootPath,
                                       _RelativePath,
                                       _LogId,
                                       this,
                                       completion);
        return;
        // Continued at ParseFileCompletion
    }
	
    OnStartContinue();
    // Continued at OnStartContinue
}

VOID 
RvdLogManagerImp::AsyncDeleteLogImp::ParseFileCompletion(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletingSubOp)
{
    // Continued from OnStart
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status;

    status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    OnStartContinue();

    // Continued at OnStartContinue
}

VOID
RvdLogManagerImp::AsyncDeleteLogImp::OnStartContinue()
{
    // continued from OnStart(), ParseFileCompletion
    NTSTATUS status;
    RvdOnDiskLog::SPtr logObject;

    status = _LogManager->FindOrCreateLog(FALSE, _DiskId, _LogId, _RelativePath, logObject);
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    // Delete the log asynchronously.
    RvdLogManagerImp::RvdOnDiskLog::AsyncDeleteLog::SPtr deleteContext;
    status = logObject->CreateAsyncDeleteLog(deleteContext);
    if (!NT_SUCCESS(status))
    {
        DoComplete(status);
        return;
    }

    // We are going to start the async delete - pass along the constructed RvdOnDiskLog sptr
    _OnDiskLog = Ktl::Move(logObject);

    KAsyncContextBase::CompletionCallback callback;
    callback.Bind(this, &AsyncDeleteLogImp::DeleteCompletionCallback);

    deleteContext->StartDelete(this, callback);
}

VOID 
RvdLogManagerImp::AsyncDeleteLogImp::DeleteCompletionCallback(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);
    
    DoComplete(CompletingSubOp.Status());
}

