/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLogManager.AsyncOpenLog.cpp

    Description:
      RvdLogManager::AsyncOpenLog implementation.

    History:
      PengLi        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"

//** OpenLog
class RvdLogManagerImp::AsyncOpenLogImp : public RvdLogManager::AsyncOpenLog
{
public:
    AsyncOpenLogImp(RvdLogManagerImp* Owner);

    VOID 
    StartOpenLog(
        __in const KGuid& DiskId, 
        __in const RvdLogId& LogId,
        __out RvdLog::SPtr& ResultLog,
        __in_opt KAsyncContextBase* const ParentAsyncContext, 
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;
    
    VOID
    StartOpenLog(
        __in const KStringView& FullyQualifiedLogFilePath,
        __out RvdLog::SPtr& Log,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

private:
    ~AsyncOpenLogImp();
        
    VOID 
    MountCompletionCallback(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& CompletingSubOp);

    VOID
    OnReuse();

    VOID
    OnStart();

    VOID
    OnStartOpenByPath();

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
    KWString                                _FullyQualifiedLogFilename;
    KWString                                _RootPath;
    KWString                                _RelativePath;
    KGuid                                   _DiskId;
    RvdLogId                                _LogId;
    RvdLog::SPtr*                           _ResultLogPtr;
    RvdLogManagerImp::RvdOnDiskLog::SPtr    _OnDiskLog;
};        

NTSTATUS 
RvdLogManagerImp::CreateAsyncOpenLogContext(__out AsyncOpenLog::SPtr& Context)
{
    Context = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncOpenLogImp(this);
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

RvdLogManagerImp::AsyncOpenLog::AsyncOpenLog()
{
}

RvdLogManagerImp::AsyncOpenLog::~AsyncOpenLog()
{
}

RvdLogManagerImp::AsyncOpenLogImp::AsyncOpenLogImp(RvdLogManagerImp* Owner)
    :   _LogManager(Owner),
        _FullyQualifiedLogFilename(GetThisAllocator()),
        _RootPath(GetThisAllocator()),
        _RelativePath(GetThisAllocator())
{
}

RvdLogManagerImp::AsyncOpenLogImp::~AsyncOpenLogImp()
{
}

VOID 
RvdLogManagerImp::AsyncOpenLogImp::StartOpenLog(
    __in KGuid const& DiskId, 
    __in RvdLogId const& LogId,
    __out RvdLog::SPtr& ResultLog,
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _DiskId = DiskId;
    _LogId = LogId;
    _ResultLogPtr = &ResultLog;
    ResultLog.Reset();
    KAssert(!_OnDiskLog);

    Start(ParentAsyncContext, CallbackPtr);
}

//* Open a log file via a FQ path and name
VOID
RvdLogManagerImp::AsyncOpenLogImp::StartOpenLog(
    __in const KStringView& FullyQualifiedLogFilePath,
    __out RvdLog::SPtr& ResultLog,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _FullyQualifiedLogFilename = FullyQualifiedLogFilePath;
    _ResultLogPtr = &ResultLog;
    ResultLog.Reset();
    KAssert(!_OnDiskLog);

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogManagerImp::AsyncOpenLogImp::OnReuse()
{
    _FullyQualifiedLogFilename = L"";
    _RelativePath = L"";
}

VOID 
RvdLogManagerImp::AsyncOpenLogImp::DoComplete(NTSTATUS Status)
{
    _LogManager->ReleaseActivity();     // Tell hosting log manager we are done with it
                                        // See OnStart()
    _OnDiskLog.Reset();
    Complete(Status);
}

// Continued from RvdLogManagerImp::AsyncOpenLogImp::StartOpenLog()
VOID 
RvdLogManagerImp::AsyncOpenLogImp::OnStart()
//
// Continuation from StartOpenLog
{
    NTSTATUS status;

    status = _FullyQualifiedLogFilename.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    if (_FullyQualifiedLogFilename.Length() > 0)
    {
        //
        // We are doing an open via filename
        //
        OnStartOpenByPath();
        return;
    }

    OnStartContinue(); 
}

VOID
RvdLogManagerImp::AsyncOpenLogImp::OnStartOpenByPath()
{
    NTSTATUS status;
    AsyncParseLogFilename::SPtr parseAsync;

    status = AsyncParseLogFilename::Create(GetThisAllocationTag(), GetThisAllocator(), parseAsync);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, 0, 0, 0);
        Complete(status);
        return;
    }

    KAsyncContextBase::CompletionCallback completion(this, &RvdLogManagerImp::AsyncOpenLogImp::ParseFileCompletion);
    parseAsync->StartParseFilename(_FullyQualifiedLogFilename,
                                   _DiskId,
                                   _RootPath,
                                   _RelativePath,
                                   _LogId,
                                   this,
                                   completion);
    // Continued at ParseFileCompletion
}

VOID 
RvdLogManagerImp::AsyncOpenLogImp::ParseFileCompletion(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletingSubOp)
{
    // Continued from OnStartOpenByPath
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status;

    status = CompletingSubOp.Status();
    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    OnStartContinue();

    // Continued at OnStartContinue
}

VOID
RvdLogManagerImp::AsyncOpenLogImp::OnStartContinue()
{
    // Continued from ParseFileCompletion or OnStart

    NTSTATUS status;
    RvdOnDiskLog::SPtr logObject;

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


    status = _LogManager->FindOrCreateLog(FALSE, _DiskId, _LogId, _RelativePath, logObject);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    // Open the log asynchronously.
    RvdLogManagerImp::RvdOnDiskLog::AsyncMountLog::SPtr openContext;
    status = logObject->CreateAsyncMountLog(openContext);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    // We are going to start the async open - pass along the constructed RvdOnDiskLog sptr
    _OnDiskLog = Ktl::Move(logObject);

    KAsyncContextBase::CompletionCallback callback;
    callback.Bind(this, &AsyncOpenLogImp::MountCompletionCallback);

    openContext->StartMount(this, callback);
}

VOID 
RvdLogManagerImp::AsyncOpenLogImp::MountCompletionCallback(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletingSubOp)
{
    UNREFERENCED_PARAMETER(Parent);
    
    if (!NT_SUCCESS(CompletingSubOp.Status()))
    {
        DoComplete(CompletingSubOp.Status());
        return;
    }

    (*_ResultLogPtr) = Ktl::Move(reinterpret_cast<RvdLog::SPtr&>(_OnDiskLog));       // return resulting sptr to RvdLog
    DoComplete(STATUS_SUCCESS);
}

