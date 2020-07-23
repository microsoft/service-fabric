#include <ktl.h>
#include <KAsyncService.h>

#include "ServiceSync.h"

KServiceSynchronizer::KServiceSynchronizer(__in BOOLEAN IsManualReset) 
    :   _Event(IsManualReset, FALSE),
        _CompletionStatus(STATUS_SUCCESS)
{
    _Callback.Bind(this, &KServiceSynchronizer::AsyncCompletion);
    SetConstructorStatus(_Event.Status());
}

KServiceSynchronizer::~KServiceSynchronizer() 
{
}

KAsyncServiceBase::OpenCompletionCallback
KServiceSynchronizer::OpenCompletionCallback()
{
    return _Callback;
}    

KAsyncServiceBase::CloseCompletionCallback
KServiceSynchronizer::CloseCompletionCallback()
{
    return _Callback;
}    

NTSTATUS
KServiceSynchronizer::WaitForCompletion(
    __in_opt ULONG TimeoutInMilliseconds,
    __out_opt PBOOLEAN IsCompleted
    )
{
    BOOLEAN b = _Event.WaitUntilSet(TimeoutInMilliseconds);
    if (!b)
    {
        if (IsCompleted)
        {
            *IsCompleted = FALSE;
        }
        
        return STATUS_IO_TIMEOUT;
    }
    else
    {        
        if (IsCompleted)
        {
            *IsCompleted = TRUE;
        }
        
        return _CompletionStatus;
    }
}

VOID KServiceSynchronizer::Reset()
{
    _Event.ResetEvent();
    _CompletionStatus = STATUS_SUCCESS;
}

VOID
KServiceSynchronizer::OnCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncServiceBase& CompletingOperationService)
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(CompletingOperationService);    
}

VOID
KServiceSynchronizer::AsyncCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncServiceBase& Service,
    __in NTSTATUS Status)
{
    _CompletionStatus = Status;
    OnCompletion(Parent, Service);
    
    _Event.SetEvent();
}    
