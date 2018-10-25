/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    KAsyncService.cpp

    Description:
      Kernel Template Library (KTL): Async Service Implementation

    History:
      richhas          25-Jun-2012        Initial version.

--*/

#include "ktl.h"
#include "KAsyncService.h"

//*** Utility class used for internal Open and Close operations
class KAsyncServiceBase::OpenCloseContext : public KAsyncContextBase
{
    K_FORCE_SHARED(OpenCloseContext);
    
    VOID
    OnStart() override
    {
        // Override the automatic completion
    }

protected:
    friend class KAsyncServiceBase;
};

KAsyncServiceBase::OpenCloseContext::OpenCloseContext() {}
KAsyncServiceBase::OpenCloseContext::~OpenCloseContext() {}



//*** KAsyncServiceBase imp
KAsyncServiceBase::KAsyncServiceBase()
    :   _IsOpen(FALSE),
        _OpenStatus(STATUS_SUCCESS),
        _CloseParent(nullptr),
        _IsOpenedGate(TRUE, FALSE),
        _OpenCompleted(FALSE)
{
}

KAsyncServiceBase::~KAsyncServiceBase()
{
    KInvariant(_DeferredCloseState._PendingActivities == 0);
}

//** Service Open logic
NTSTATUS
KAsyncServiceBase::StartOpen(      
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt OpenCompletionCallback CallbackPtr,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride)
{
    // Allocate all resources needed to succeed both an Open and Close - or fail the StartOpen
    OpenCloseContext::SPtr  closeContext;
    OpenCloseContext::SPtr  openContext = _new(KTL_TAG_ASYNC_SERVICE, GetThisAllocator()) OpenCloseContext();
    if (openContext == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS                status = openContext->Status();
    if (NT_SUCCESS(status))
    {
        closeContext = _new(KTL_TAG_ASYNC_SERVICE, GetThisAllocator()) OpenCloseContext();
        if (closeContext == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = closeContext->Status();
    }

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    K_LOCK_BLOCK(_ThisLock)
    {
        if (_IsOpen)
        {
            return STATUS_SHARING_VIOLATION;
        }
        _IsOpen = TRUE;
    }
        
    KAssert(_OpenContext == nullptr);
    KAssert(_CloseContext == nullptr);
    KAssert(!_IsOpenedGate.IsSignaled());       // Closes will be held

    _OpenContext = Ktl::Move(openContext);
    _CloseContext = Ktl::Move(closeContext);
    _OpenCallback = CallbackPtr;
        
    // Start the open op - comps in parent's appt - see OnOpenCallback()
    _OpenContext->Start(ParentAsyncContext, KAsyncContextBase::CompletionCallback(this, &KAsyncServiceBase::OnOpenCallback));   
    AddRef();                   // #3 - Keep this svc instance alive until OnOpenCallback is called. 

    // Start the service
    KAsyncContextBase::Start(
        nullptr, 
        KAsyncContextBase::CompletionCallback(this, &KAsyncServiceBase::OnInternalCompletion), 
        GlobalContextOverride);

    return STATUS_PENDING;
}

VOID
KAsyncServiceBase::OnStart()
{
    AcquireActivities(2);       // #1 hold actual completion until CompleteClose() is called or open fails
                                // #2 hold actual completion until CompleteOpen() is called 

    OnServiceOpen();            // Inform derivation it is being opened

    // Continues at CompleteOpen()
}

VOID 
KAsyncServiceBase::OnServiceOpen()
{
    //* Default Imp
    CompleteOpen(STATUS_SUCCESS);
}

BOOLEAN
KAsyncServiceBase::CompleteOpen(NTSTATUS OpenStatus, void* OpenCompleteCtx)
{
    // May be called multiple times - first wins
    BOOLEAN     result = FALSE;

    K_LOCK_BLOCK(_ThisLock)
    {
        if (_OpenCompleted)
        {
            return FALSE;
        }

        _OpenCompleted = TRUE;
    }

    // Can be called from any thread
    if (!NT_SUCCESS(OpenStatus))
    {
        // Open failed - trigger completion of this service - prop status to OnInternalCompletion()

        // NOTE: OnInternalCompletion() will detect open is still pending (implying an open failure) and
        //       trigger completion of that open; continuing @ OnOpenCallback()

        result = KAsyncContextBase::Complete(
            CompletingCallback(this, &KAsyncServiceBase::OnUnsafeOpenCompleteCalled), 
            OpenCompleteCtx, 
            OpenStatus);

        ReleaseActivities();                // Reverse #1 in OnStart() - CompleteClose will not be called
    }
    else
    {
        // Open worked - inform continuation of StartOpen() caller of open completion (successfully)
        result = _OpenContext->Complete(
            CompletingCallback(this, &KAsyncServiceBase::OnUnsafeOpenCompleteCalled), 
            OpenCompleteCtx, 
            OpenStatus);

        // Continued @ OnOpenCallback()
    }

    KInvariant(result);             // Complete()s must succeed; API is locked

    ReleaseActivities();        // Reverse #2 in OnStart() - allow actual completion of this instance
    return TRUE;
}

VOID
KAsyncServiceBase::OnOpenCallback(KAsyncContextBase* const Parent, KAsyncContextBase& OpenContext)
{
    // Continued from CompleteOpen() or OnInternalCompletion() because open failed

    KAssert(_OpenCompleted);

    // Open the API gate, set open status state, and call optional callback in StartOpen()'s Parent apt
    _OpenStatus = OpenContext.Status();
    if (NT_SUCCESS(_OpenStatus))
    {
        _OpenStatus = STATUS_SUCCESS;
    }

    KInvariant(!_IsOpenedGate.IsSignaled());
    _IsOpenedGate.SetEvent();   // Unlock the API

    if (_OpenCallback)
    {
        _OpenCallback(Parent, *this, _OpenStatus);
    }

    Release();                      // reverse #3 in StartOpen() 

    // Warning: This instance state cannot be considered stable after this point
}   



//** Service Close logic
NTSTATUS
KAsyncServiceBase::StartClose(      
    __in_opt KAsyncContextBase* const ParentAsyncContext, 
    __in_opt CloseCompletionCallback CallbackPtr)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        if(!_IsOpen)
        {
            return STATUS_UNSUCCESSFUL;
        }
        _IsOpen = FALSE;
    }

    AddRef();           // #4 Hold dtor'ing until OnCloseCallback() is completed - or an error (below)
    _CloseCallback = CallbackPtr;
    _CloseParent = ParentAsyncContext;

    if (GetOpenStatus() == STATUS_PENDING)
    {
        // Open still active - delay the Close logic until open completes
        KAsyncEvent::WaitContext::SPtr      waitCtx;
        NTSTATUS        status = _IsOpenedGate.CreateWaitContext(KTL_TAG_ASYNC_SERVICE, GetThisAllocator(), waitCtx);
        if (NT_SUCCESS(status))
        {
            waitCtx->StartWaitUntilSet(nullptr, KAsyncContextBase::CompletionCallback(this, &KAsyncServiceBase::OnCloseContinuation));
        }
        else
        {
            Release();      // reverse #4 - above
            return status;
        }

        return STATUS_PENDING;
        // Continued @ OnCloseContinuation()
    }

    if (!KAsyncContextBase::Cancel())
    {
        // Open must have (or is in the process of being) failed
        NTSTATUS        status = GetOpenStatus();
        KInvariant(!NT_SUCCESS(status));
        
        Release();      // reverse #4 - above
        return status;
    }

    return STATUS_PENDING;
}

VOID
KAsyncServiceBase::OnCloseContinuation(KAsyncContextBase* const, KAsyncContextBase&)
{
    // Continued from StartClose()
    if (!KAsyncContextBase::Cancel())
    {
        // Open must have failed
        NTSTATUS        status = GetOpenStatus();
        KInvariant(!NT_SUCCESS(status));

        // Cause open failure status to prop to the close callback
        _CloseContext->Start(_CloseParent, KAsyncContextBase::CompletionCallback(this, &KAsyncServiceBase::OnCloseCallback));
        KInvariant(_CloseContext->Complete(status));    // Cause OnCloseCallback to be invoked in closer's
                                                    // apt
        // Continued @ OnCloseCallback()

        // Warning: This instance state cannot be considered stable after this point
        return;
    }

    // Continued @ OnCancel()
}

VOID
KAsyncServiceBase::OnCancel()
{
    // Continued from StartClose() or OnCloseContinuation()

    if (_DeferredCloseState._IsDeferredCloseEnabled)
    {
        OnDeferredClosing();        // Inform the derivation that it may start its internal deferred closure steps
    }

    _CloseContext->Start(_CloseParent, KAsyncContextBase::CompletionCallback(this, &KAsyncServiceBase::OnCloseCallback));

    ScheduleOnServiceClose();       // Inform the derivation to shutdown

    // Continued @ CompleteClose()
}

VOID KAsyncServiceBase::OnDeferredClosing()     // default
{
}

VOID
KAsyncServiceBase::OnServiceClose()             // default
{
    // Default Imp...
    CompleteClose(STATUS_SUCCESS);
}

BOOLEAN
KAsyncServiceBase::CompleteClose(NTSTATUS CloseStatus, void* CloseCompleteCtx)
{
    // Continued from OnCancel() or OnCloseContinuation()
    
    KInvariant((_CloseContext->Status() == STATUS_PENDING) &&               // Validate proper closing state
               (!_DeferredCloseState._IsDeferredCloseEnabled || (_DeferredCloseState._IsClosePending)));

    // Can be called from any thread; also allows for multiple calls - first wins
    BOOLEAN     result = KAsyncContextBase::Complete(
        CompletingCallback(this, &KAsyncServiceBase::OnUnsafeCloseCompleteCalled), 
        CloseCompleteCtx, 
        CloseStatus);

    if (result)
    {
        ReleaseActivities();        // Reverse #1 in OnStart() - Enable continuation @ OnInternalCompletion()
    }

    return result;
}

VOID
KAsyncServiceBase::OnCloseCallback(KAsyncContextBase* const Parent, KAsyncContextBase& CloseContext)
{
    // Continued from OnInternalCompletion() or OnCloseContinuation()

    // Set open status state, call optional callback in StartOpen() Parent apt, and open the API gate
    if (_CloseCallback)
    {
        _CloseCallback(Parent, *this, CloseContext.Status());
    }

    Release();              // reverse #4 in StartClose()

    // Warning: This instance state cannot be considered stable after this point
}   


//** Common completion logic for the service instance
VOID
KAsyncServiceBase::OnInternalCompletion(KAsyncContextBase* const Parent, KAsyncContextBase& This)
{
    UNREFERENCED_PARAMETER(Parent);        
    
    // Continued from CompleteClose() or CompleteOpen() because of a failed open
    KAssert(_OpenCompleted);
    KAssert(&This == this);
#if !DBG
    UNREFERENCED_PARAMETER(This);
#endif        

    // This logic guarentees (plus OnCloseCallback() and OnOpenCallback()) that this KAsyncServiceBase 
    // instance will be completed before the supplied Open/Close passed callback is called

    if (GetOpenStatus() == STATUS_PENDING)
    {
        // Open is failing - complete the open context with status of failed service open
        KInvariant(_OpenContext->Complete(Status()));
        return;

        // Continued @ OnOpenCallback()
    }
    else
    {
        // Normal Close triggered completion
        // Inform continuation of StartClose() caller via OnCloseCallback()
        KInvariant(_CloseContext->Complete(Status()));
        return;

        // Continued @ OnCloseCallback()
    }
}

//** Service Reuse() imp
VOID
KAsyncServiceBase::OnReuse()
{
    _IsOpenedGate.ResetEvent();
    _CloseContext = nullptr;
    _OpenContext = nullptr;
    _IsOpen = FALSE;
    _OpenCompleted = FALSE;
    KInvariant(_DeferredCloseState._PendingActivities == 0);
    _DeferredCloseState._Value = 0;
    OnServiceReuse();
}

VOID
KAsyncServiceBase::OnServiceReuse()
{
}

//** API delay support
KAsyncServiceBase::DeferredCloseState::DeferredCloseState()
    :   _Value(0)
{
}

KAsyncServiceBase::DeferredCloseState::DeferredCloseState(const DeferredCloseState& From)
    :   _Value(From._Value)
{
}


NTSTATUS
KAsyncServiceBase::GetOpenStatus()
{
    return (!_IsOpenedGate.IsSignaled()) ? STATUS_PENDING : _OpenStatus;
}

BOOLEAN 
KAsyncServiceBase::IsOpen()
{
    BOOLEAN     result = FALSE;

    K_LOCK_BLOCK(_ThisLock)
    {
        NTSTATUS    openStatus = GetOpenStatus();

        result = (openStatus != STATUS_PENDING) && NT_SUCCESS(openStatus) && _IsOpen;
    }

    return result;
}

VOID
KAsyncServiceBase::SetDeferredCloseBehavior()
{
    DeferredCloseState      mustBeBefore;
    DeferredCloseState      mustBeAfter;

    mustBeAfter._IsDeferredCloseEnabled = true;
    KInvariant(_InterlockedCompareExchange(
                    &_DeferredCloseState._Value, 
                    mustBeAfter._Value, 
                    mustBeBefore._Value) ==  mustBeBefore._Value);
}

// Test (DEBUG) only 
#if DBG
    LONG volatile KAsyncServiceBase::Test_TryAcquireServiceActivitySpinCount = 0;
    LONG volatile KAsyncServiceBase::Test_ScheduleOnServiceCloseSpinCount = 0;
    LONG volatile KAsyncServiceBase::Test_ReleaseServiceActivitySpinCount = 0;
#endif


BOOLEAN
KAsyncServiceBase::TryAcquireServiceActivity()
{
    KInvariant(_DeferredCloseState._IsDeferredCloseEnabled);

    if (TryAcquireActivities())
    {
        if (!IsOpen())
        {
            ReleaseActivities();
            return FALSE;
        }

        // Try acquire ServiceActivity by incrementing _PendingActivities
        DeferredCloseState  currentValue;
        LONG                icxResult;

        do
        {
            currentValue._Value = _DeferredCloseState._Value;   // snapshot current
            if (currentValue._IsClosePending)
            {
                // Close is occuring
                ReleaseActivities();
                return FALSE;
            }

            DeferredCloseState  newValue(currentValue);
            newValue._PendingActivities++;
            KAssert(newValue._PendingActivities > 0);

            icxResult = InterlockedCompareExchange(&_DeferredCloseState._Value, newValue._Value, currentValue._Value);
            #if DBG
                if (icxResult !=  currentValue._Value)
                {
                    InterlockedIncrement(&KAsyncServiceBase::Test_TryAcquireServiceActivitySpinCount);
                }
            #endif
        } while (icxResult != currentValue._Value);

        return TRUE;
    }

    return FALSE;
}

BOOLEAN
KAsyncServiceBase::ReleaseServiceActivity()
{
    KInvariant(_DeferredCloseState._IsDeferredCloseEnabled);

    // Release Service Activity by decrementing _PendingActivities; calling OnServiceClosed when
    // last release is done while _IsClosePending

    DeferredCloseState  currentValue;
    LONG                icxResult;

    do
    {
        currentValue._Value = _DeferredCloseState._Value;   // snapshot current
        KInvariant(currentValue._PendingActivities > 0);

        DeferredCloseState  newValue(currentValue);
        newValue._PendingActivities--;

        icxResult = InterlockedCompareExchange(&_DeferredCloseState._Value, newValue._Value, currentValue._Value);
        #if DBG
            if (icxResult !=  currentValue._Value)
            {
                InterlockedIncrement(&KAsyncServiceBase::Test_ReleaseServiceActivitySpinCount);
            }
        #endif
    } while (icxResult != currentValue._Value);

    ReleaseActivities();

    if (currentValue._IsClosePending && (currentValue._PendingActivities == 1))
    {
        OnServiceClose();
    }
    return (currentValue._PendingActivities == 1);
}

VOID
KAsyncServiceBase::ScheduleOnServiceClose()
{
    if (_DeferredCloseState._IsDeferredCloseEnabled)
    {
        // Set _IsClosePending; calling OnServiceClose() if no outstanding activities
        DeferredCloseState  currentValue;
        LONG                icxResult;

        do
        {
            currentValue._Value = _DeferredCloseState._Value;   // snapshot current
            KInvariant(!_DeferredCloseState._IsClosePending);

            DeferredCloseState  newValue(currentValue);
            newValue._IsClosePending = true;

            icxResult = InterlockedCompareExchange(&_DeferredCloseState._Value, newValue._Value, currentValue._Value);
            #if DBG
                if (icxResult !=  currentValue._Value)
                {
                    InterlockedIncrement(&KAsyncServiceBase::Test_ScheduleOnServiceCloseSpinCount);
                }
            #endif
        } while (icxResult != currentValue._Value);

        if (currentValue._PendingActivities > 0)
        {
            return;
        }
    }

    OnServiceClose();
}


//
// Service open/close synchronization implementation
//
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

VOID
KServiceSynchronizer::Reset()
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

#if defined(K_UseResumable)
//** CTOR/DTOR for Open/CloseAwaiter
namespace ktl
{
    namespace kservice
    {
        OpenAwaiter::OpenAwaiter() {}
        OpenAwaiter::~OpenAwaiter()
        {
            KInvariant(!_awaiterHandle);
        }

        CloseAwaiter::CloseAwaiter() {}
        CloseAwaiter::~CloseAwaiter()
        {
            KInvariant(!_awaiterHandle);
        }
    }
}
#endif
