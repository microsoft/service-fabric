// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

//
// This template can be used to wrap a KAsyncContext and have its
// OnStart() immediately complete with a specific status code.
//
template<class T>
class CompleteOnStartInterceptor : public KAsyncContextInterceptor
{
    K_FORCE_SHARED_WITH_INHERITANCE(CompleteOnStartInterceptor);

public:
    CompleteOnStartInterceptor(__in NTSTATUS CompletionStatus)
    {
        _CompletionStatus = CompletionStatus;
    }
    
    static NTSTATUS
    Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __in NTSTATUS CompletionStatus,
        __out KSharedPtr<CompleteOnStartInterceptor>& Context)
    {
        NTSTATUS status;

        Context = _new(AllocationTag, Allocator) CompleteOnStartInterceptor(CompletionStatus);
        if (Context == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = Context->Status();
        if (! NT_SUCCESS(status))
        {
            Context = nullptr;
        }

        return status;  
    }
            
protected:
    //
    // When interceptor is enabled, the OnStart(), OnCancel(),
    // OnCompleted() and OnReuse() are redirected into the interceptor
    // class within the context of the async being intercepted. If
    // access to the intercepted class is needed, the GetIntercepted()
    // method is available. This may be useful for forwarding on the
    // call.
    //
    void OnStartIntercepted()
    {
        T* async = static_cast<T*>(&(GetIntercepted()));
        async->CompleteRequest(_CompletionStatus);
    }
    
    void OnCancelIntercepted() {}
    void OnCompletedIntercepted() {}
    void OnReuseIntercepted() {}

protected:
    NTSTATUS _CompletionStatus;
};

template<class T>CompleteOnStartInterceptor<T>::~CompleteOnStartInterceptor<T>()
{
}


//
// This template can be used to wrap a KAsyncContext and have its
// OnStart() delayed by a specific number of milliseconds.
//
template<class T>
class DelayStartInterceptor : public KAsyncContextInterceptor
{
    K_FORCE_SHARED_WITH_INHERITANCE(DelayStartInterceptor);

    
public:
    DelayStartInterceptor(__in ULONG DelayInMs)
    {
        NTSTATUS status;

        status = KTimer::Create(_Timer, GetThisAllocator(), GetThisAllocationTag());
        if (! NT_SUCCESS(status))
        {
            SetConstructorStatus(status);
            return;
        }
        
        _DelayInMs = DelayInMs;     
    }
    
    static NTSTATUS
    Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __in ULONG DelayInMs,
        __out KSharedPtr<DelayStartInterceptor>& Context)
    {
        NTSTATUS status;

        Context = _new(AllocationTag, Allocator) DelayStartInterceptor(DelayInMs);
        if (Context == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = Context->Status();
        if (! NT_SUCCESS(status))
        {
            Context = nullptr;
        }

        return status;  
    }
            
protected:
    //
    // When interceptor is enabled, the OnStart(), OnCancel(),
    // OnCompleted() and OnReuse() are redirected into the interceptor
    // class within the context of the async being intercepted. If
    // access to the intercepted class is needed, the GetIntercepted()
    // method is available. This may be useful for forwarding on the
    // call.
    //
    VOID TimerCompletion(
        __in_opt KAsyncContextBase* const ParentAsync,
        __in KAsyncContextBase& Async
        )
    {
        //
        // When timer is completed then forward the OnStart() call to
        // the underlying async
        //
        _Timer->Reuse();
        ForwardOnStart();
    }

    
    void OnStartIntercepted()
    {
        //
        // Start the timer to hold up the async.
        //
        // CONSIDER: Does this need to be started in the underlying
        //           async's apartment ?
        //
        KAsyncContextBase::CompletionCallback completion(this, TimerCompletion);        
        _Timer->StartTimer(_DelayInMs, nullptr, completion);
    }
    
    void OnCancelIntercepted() 
    {
        ForwardOnCancel();
    }

    void OnCompletedIntercepted() 
    {
        ForwardOnCompleted();
    }

    void OnReuseIntercepted() 
    {
        ForwardOnReuse();
    }

protected:
    ULONG _DelayInMs;
    KTimer::SPtr _Timer;
};

template<class T>DelayStartInterceptor<T>::~DelayStartInterceptor<T>()
{
}

//
// This template can be used to wrap a KAsyncContext and have its
// OnStart() held indefinitely until an event is set.
//
template<class T>
class WaitEventInterceptor : public KAsyncContextInterceptor
{
    K_FORCE_SHARED_WITH_INHERITANCE(WaitEventInterceptor);
    
public:
    WaitEventInterceptor(
        __in_opt KAsyncEvent* const OnStartWaitEvent,
        __in_opt KAsyncEvent* const OnCompletedWaitEvent,
        __in_opt KAsyncEvent* const OnStartEvent,
        __in_opt KAsyncEvent* const OnCompletedEvent
        )
    {
        NTSTATUS status;

        _OnStartWaitCompletion.Bind(this,
                         &WaitEventInterceptor<T>::OnStartWaitCompletion);

        _OnCompletedWaitCompletion.Bind(this,
                         &WaitEventInterceptor<T>::OnCompletedWaitCompletion);

        if (OnStartWaitEvent != NULL)
        {
            status = OnStartWaitEvent->CreateWaitContext(GetThisAllocationTag(), GetThisAllocator(), _OnStartWaitEvent);
            if (! NT_SUCCESS(status))
            {
                SetConstructorStatus(status);
                return;
            }
        } else {
            _OnStartWaitEvent = nullptr;
        }

        if (OnCompletedWaitEvent != NULL)
        {
            status = OnCompletedWaitEvent->CreateWaitContext(GetThisAllocationTag(), GetThisAllocator(), _OnCompletedWaitEvent);
            if (!NT_SUCCESS(status))
            {
                SetConstructorStatus(status);
                return;
            }
        }
        else {
            _OnCompletedWaitEvent = nullptr;
        }

        _OnStartEvent = OnStartEvent;
        _OnCompletedEvent = OnCompletedEvent;
    }
    
    static NTSTATUS
    Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __in_opt KAsyncEvent* const OnStartWaitEvent,
        __in_opt KAsyncEvent* const OnCompletedWaitEvent,
        __in_opt KAsyncEvent* const OnStartEvent,
        __in_opt KAsyncEvent* const OnCompletedEvent,
        __out KSharedPtr<WaitEventInterceptor>& Context)
    {
        NTSTATUS status;

        Context = _new(AllocationTag, Allocator) WaitEventInterceptor(OnStartWaitEvent, OnCompletedWaitEvent, OnStartEvent, OnCompletedEvent);
        if (Context == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = Context->Status();
        if (! NT_SUCCESS(status))
        {
            Context = nullptr;
        }

        return status;  
    }
            
protected:
    //
    // When interceptor is enabled, the OnStart(), OnCancel(),
    // OnCompleted() and OnReuse() are redirected into the interceptor
    // class within the context of the async being intercepted. If
    // access to the intercepted class is needed, the GetIntercepted()
    // method is available. This may be useful for forwarding on the
    // call.
    //
    VOID OnStartWaitCompletion(
        __in_opt KAsyncContextBase* const,
        __in KAsyncContextBase&
        )
    {
        //
        // When event signalled then forward the OnStart() call to
        // the underlying async
        //
        _OnStartWaitEvent->Reuse();
        ForwardOnStart();
    }

    VOID OnCompletedWaitCompletion(
        __in_opt KAsyncContextBase* const,
        __in KAsyncContextBase&
        )
    {
        //
        // When event signalled then forward the OnStart() call to
        // the underlying async
        //
        _OnCompletedWaitEvent->Reuse();
        ForwardOnCompleted();
    }
    
    void OnStartIntercepted()
    {
        //
        // Set the OnStartEvent if one is specified
        //
        if (_OnStartEvent)
        {
            _OnStartEvent->SetEvent();
        }
        
        //
        // Start the wait for event to hold up the async if one is
        // specified else kick off the async right away
        //
        if (_OnStartWaitEvent)
        {
            printf("\nOnStartIntercepted Wait\n\n");
            //
            // CONSIDER: Does this need to be started in the underlying
            //           async's apartment ?
            //
            _OnStartWaitEvent->StartWaitUntilSet(nullptr, _OnStartWaitCompletion);
        } else {
            ForwardOnStart();
        }
    }

    void OnCompletedIntercepted()
    {
        //
        // Set the OnCompletedEvent if one is specified
        //
        if (_OnCompletedEvent)
        {
            _OnCompletedEvent->SetEvent();
        }

        //
        // Start the wait for event to hold up the async if one is
        // specified else kick off the async right away
        //
        if (_OnCompletedWaitEvent)
        {
            //
            // CONSIDER: Does this need to be started in the underlying
            //           async's apartment ?
            //
            _OnCompletedWaitEvent->StartWaitUntilSet(nullptr, _OnCompletedWaitCompletion);
        }
        else {
            ForwardOnCompleted();
        }
    }

    void OnCancelIntercepted() 
    {
        ForwardOnCancel();
    }

    void OnReuseIntercepted() 
    {
        ForwardOnReuse();
    }

protected:
    KAsyncContextBase::CompletionCallback _OnStartWaitCompletion;
    KAsyncContextBase::CompletionCallback _OnCompletedWaitCompletion;
    KAsyncEvent::WaitContext::SPtr _OnStartWaitEvent;
    KAsyncEvent::WaitContext::SPtr _OnCompletedWaitEvent;
    KAsyncEvent* _OnStartEvent;
    KAsyncEvent* _OnCompletedEvent;
};

template<class T>WaitEventInterceptor<T>::~WaitEventInterceptor<T>()
{
}
