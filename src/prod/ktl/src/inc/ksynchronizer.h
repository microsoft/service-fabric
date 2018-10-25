/*++

Copyright (c) 2012  Microsoft Corporation

Module Name:

    ksynchronizer.h

Abstract:

    This file defines a simple synchronization object to be used with KAsyncContextBase.
    It allows a user to fire an async operation then wait for it to complete.
    It is mostly useful for tests and tools where synchronous behavior is desired or is more convenient.

Author:

    Peng Li (pengli)    12-March-2012

Environment:

    Kernel Mode or User Mode

Notes:

Revision History:

--*/

#pragma once

//
// Example:
// 
// KSynchronizer syncObject;
// ayncContext->StartOperation(
//      ...,
//      syncObject,     // CompletionCallback
//      parentAsync,
//      ...
//      ); 
//
// status = syncObject.WaitForCompletion();
// ...
//
// Example:
// 
// KSynchronizer syncObject;
// ayncContext->StartOperation(
//      ...,
//      syncObject,     // CompletionCallback
//      parentAsync,
//      ...
//      ); 
//
// BOOLEAN isCompleted = FALSE;
// status = syncObject.WaitForCompletion(5000, &isCompleted);
//
// if (!isCompleted)
// {
//      KAssert(status == STATUS_IO_TIMEOUT);
//      ayncContext->Cancel();
// }
//
// status = syncObject.WaitForCompletion();
//
// The default bahavior is to use auto-reset event. So the same synchronizer object 
// can be easily reused.
// 
// Example:
//
// KSynchronizer syncObject;
//
// ayncContext1->StartOperation(
//      ...,
//      syncObject,     // CompletionCallback
//      parentAsync,
//      ...
//      ); 
//
// status = syncObject.WaitForCompletion();
// ...
//
// ayncContext2->StartOperation(
//      ...,
//      syncObject,     // CompletionCallback
//      parentAsync,
//      ...
//      ); 
//
// status = syncObject.WaitForCompletion();
// ...
//

class KSynchronizer : public KObject<KSynchronizer>
{    
    K_DENY_COPY(KSynchronizer);
        
public:

    FAILABLE
    KSynchronizer(
        __in BOOLEAN IsManualReset = FALSE
        );
    
    virtual ~KSynchronizer();

    //
    // Use the returned KDelegate as the async completion callback.
    // Then wait for completion using WaitForCompletion().
    //

    KAsyncContextBase::CompletionCallback
    AsyncCompletionCallback();

    //
    // Cast this KSynchronizer object to a KAsyncContextBase::CompletionCallback object.
    // Then wait for completion using WaitForCompletion().    
    //

    operator KAsyncContextBase::CompletionCallback();

    //
    // Wait for the async operation to complete or until it times out.
    //
    // If TimeoutInMilliseconds is not KEvent::Infinite, and the return status is STATUS_IO_TIMEOUT,
    // the user should test IsCompleted. If IsCompleted is FALSE, the user can optionally cancel 
    // the pending async operation. But it must continue calling WaitForCompletion() 
    // with infinite timeout or until IsCompleted is TRUE. The synchronizer object must remain valid
    // until the async operation completes.
    //
    
    NTSTATUS
    WaitForCompletion(
        __in_opt ULONG TimeoutInMilliseconds = KEvent::Infinite,
        __out_opt PBOOLEAN IsCompleted = nullptr
        );

    //
    // If IsManualReset is TRUE, call this method to reset the event after the async operation completes.
    // If IsManualReset is FALSE (the default), this is a no op.
    //
    
    VOID Reset();

    //
    // Extension API for derivation.
    // It gets invoked when the async operation completes.
    //

    virtual VOID
    OnCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

protected:
    
    VOID
    AsyncCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );
    
    KEvent _Event;
    NTSTATUS _CompletionStatus;
    KAsyncContextBase::CompletionCallback _Callback;
};

