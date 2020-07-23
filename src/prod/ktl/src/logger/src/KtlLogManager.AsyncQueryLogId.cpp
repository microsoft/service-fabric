/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    RvdLogManager.AsyncQueryLogId.cpp

    Description:
      RvdLogManager::AsyncQueryLogId implementation.

    History:
      PengLi        22-Oct-2010         Initial version.

--*/

#include "InternalKtlLogger.h"

//** QueryLogId
class RvdLogManagerImp::AsyncQueryLogIdImp : public RvdLogManager::AsyncQueryLogId
{
public:
    AsyncQueryLogIdImp(__in RvdLogManagerImp& Owner);

	VOID
	StartQueryLogId(
		__in const KStringView& FullyQualifiedLogFilePath,
		__out RvdLogId& LogId,
		__in_opt KAsyncContextBase* const ParentAsyncContext,
		__in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
	
private:
    ~AsyncQueryLogIdImp();
        
    VOID
    OnReuse();

    VOID
    OnStart();

    VOID
    ParseFileCompletion(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& CompletingSubOp);
    
    VOID
    DoComplete(NTSTATUS Status);

private:
    RvdLogManagerImp::SPtr                  _LogManager;


    // Per operation state:
    KWString                                _FullyQualifiedLogFilename;
    KWString                                _RootPath;
    KWString                                _RelativePath;
    KGuid                                   _DiskId;
    RvdLogId*                               _LogId;
};        

NTSTATUS 
RvdLogManagerImp::CreateAsyncQueryLogIdContext(__out AsyncQueryLogId::SPtr& Context)
{
    Context = _new(KTL_TAG_LOGGER, GetThisAllocator()) AsyncQueryLogIdImp(*this);
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

RvdLogManagerImp::AsyncQueryLogId::AsyncQueryLogId()
{
}

RvdLogManagerImp::AsyncQueryLogId::~AsyncQueryLogId()
{
}

RvdLogManagerImp::AsyncQueryLogIdImp::AsyncQueryLogIdImp(__in RvdLogManagerImp& Owner)
    :   _LogManager(&Owner),
        _FullyQualifiedLogFilename(GetThisAllocator()),
        _RootPath(GetThisAllocator()),
        _RelativePath(GetThisAllocator())
{
}

RvdLogManagerImp::AsyncQueryLogIdImp::~AsyncQueryLogIdImp()
{
}

//* Open a log file via a FQ path and name
VOID
RvdLogManagerImp::AsyncQueryLogIdImp::StartQueryLogId(
    __in const KStringView& FullyQualifiedLogFilePath,
    __out RvdLogId& LogId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _FullyQualifiedLogFilename = FullyQualifiedLogFilePath;
    _LogId = &LogId;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
RvdLogManagerImp::AsyncQueryLogIdImp::OnReuse()
{
    _FullyQualifiedLogFilename = L"";
    _RelativePath = L"";
}

VOID 
RvdLogManagerImp::AsyncQueryLogIdImp::DoComplete(NTSTATUS Status)
{
    _LogManager->ReleaseActivity();     // Tell hosting log manager we are done with it
                                        // See OnStart()
    Complete(Status);
}

// Continued from RvdLogManagerImp::AsyncQueryLogIdImp::StartQueryLogId()
VOID 
RvdLogManagerImp::AsyncQueryLogIdImp::OnStart()
//
// Continuation from StartQueryLogId
{
    NTSTATUS status;
    AsyncParseLogFilename::SPtr parseAsync;

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
	
    status = _FullyQualifiedLogFilename.Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    status = AsyncParseLogFilename::Create(GetThisAllocationTag(), GetThisAllocator(), parseAsync);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    KAsyncContextBase::CompletionCallback completion(this, &RvdLogManagerImp::AsyncQueryLogIdImp::ParseFileCompletion);
    parseAsync->StartParseFilename(_FullyQualifiedLogFilename,
                                   _DiskId,
                                   _RootPath,
                                   _RelativePath,
                                   *_LogId,
                                   this,
                                   completion);
    // Continued at ParseFileCompletion
}

VOID 
RvdLogManagerImp::AsyncQueryLogIdImp::ParseFileCompletion(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& CompletingSubOp)
{
    // Continued from OnStartOpenByPath
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status;

    status = CompletingSubOp.Status();

	DoComplete(status);
}

