/*++

Copyright (c) 2012  Microsoft Corporation

Module Name:

    ksynchronizer.cpp

Abstract:

    This file implements the KSynchronizer class.

Author:

    Peng Li (pengli)    12-March-2012

Environment:

    Kernel Mode or User Mode

Notes:

Revision History:

--*/

#include <ktl.h>
    
KSynchronizer::KSynchronizer(
    __in BOOLEAN IsManualReset
    ) : 
    _Event(
        IsManualReset,      // IsManualReset
        FALSE               // InitialState
        ),
    _CompletionStatus(STATUS_SUCCESS)
{
    _Callback.Bind(this, &KSynchronizer::AsyncCompletion);
    SetConstructorStatus(_Event.Status());
}

KSynchronizer::~KSynchronizer() 
{
}

KAsyncContextBase::CompletionCallback
KSynchronizer::AsyncCompletionCallback()
{
    return _Callback;
}    

KSynchronizer::operator KAsyncContextBase::CompletionCallback()
{
    return _Callback;
}    

NTSTATUS
KSynchronizer::WaitForCompletion(
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

VOID KSynchronizer::Reset()
{
    _Event.ResetEvent();
    _CompletionStatus = STATUS_SUCCESS;
}

VOID
KSynchronizer::OnCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    UNREFERENCED_PARAMETER(Parent);
    UNREFERENCED_PARAMETER(CompletingOperation);    
}

VOID
KSynchronizer::AsyncCompletion(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    _CompletionStatus = CompletingOperation.Status();
    OnCompletion(Parent, CompletingOperation);
    
    _Event.SetEvent();
}    

