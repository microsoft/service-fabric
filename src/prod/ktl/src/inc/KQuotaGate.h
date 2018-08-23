/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    KQuotaGate.h

    Description:
      Kernel Template Library (KTL): Async QuotaGate/Semaphore Support Definitions

    History:
      richhas          27-May-2011         Initial version.

--*/

#pragma once

//** KQuotaGate class
//
// Description:
//      This class is a uService that allows an available value - called the FreeQuanta - to be acquired in
//      terms of some portion of the FreeQuanta - the DesiredQuanta - via instances AcquireContext. Instances
//      of KQuotaGate, as a uService, follow the Activate()/Deactivate() pattern. A Quanta from a KQuotaGate
//      instance is acquired via AcquireContext::StartAcquire() thru which the DesiredQuanta is passed. Calls
//      to StartAcquire() are serviced in FIFO order by the owning KQuotaGate. Only when a given AcquireContext
//      is the next instance in line to be satisfied AND there is enough FreeQuanta to meet the DesiredQuanta
//      of that next-to-be-serviced AcquireContext will that acquiring AcquireContext instance be completed -
//      according to the completion related parameters passed to StartAcquire(). Once a AcquireContext has been
//      completed, the FreeQuanta of the corresponding KQuotaGate has been reduced by the DesiredQuanta of that
//      completing AcquireContext. It is the using application's responsibility to return acquired quanta back
//      to the KQuotaGate via its ReleaseQuanta() method.
//
//      Any AcquireContext that has been queued before Deactivate() is called will be completed with a status
//      of K_STATUS_SHUTDOWN_PENDING. Any attempt to issue a StartAcquire() on a AcquireContext that has a
//      parent KQuotaGate that's being shutdown will also complete with a status of K_STATUS_SHUTDOWN_PENDING.
//
//      Any number of acquirers (AcquireContext instances) can be started. They may be canceled via the normal
//      KAsyncContextBase::Cancel() method; resulting in a completion status of STATUS_CANCELLED.
//
//      KQuotaGate and AcquireContext instances may both be Reuse()'d. A KQuotaGate instance may be Reuse()'d
//      only after Deactivation on that instance has finished - each Activate/Deactivation cycle causes a new
//      version of that KQuotaGate to be established. An AcquireContext instance may only be reused within the
//      same KQuotaGate version that it was created under - else all attempts to StartAcquire() will result in
//      a completion status of K_STATUS_SHUTDOWN_PENDING.
//
class KQuotaGate : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KQuotaGate);

public:
    //* Factory for creation of KQuotaGate Service instances.
    //
    //  Parameters:
    //      AllocationTag, Allocator        - Standard memory allocation component and tag
    //                                        data member in IType to be used for queuing instances
    //      Context                         - SPtr that points to an allocated KQuotaGate instance
    //
    //  Returns:
    //      STATUS_SUCCESS                  - Context points to an allocated KQuotaGate instance
    //      STATUS_INSUFFICIENT_RESOURCES   - Out of memory
    //
    static NTSTATUS
    Create(
        __in ULONG AllocationTag,
        __in KAllocator& Allocator,
        __out KSharedPtr<KQuotaGate>& Context);

    //* Activate an instance of a KQuotaGate Service.
    //
    // KQuotaGate Service instances that are useful are an KAsyncContext in the operational state.
    // This means that the supplied callback (Callback) will be invoked once Deactivate() is called
    // and the instance has finished its shutdown process - See Deactivate().
    //
    //  Parameters:
    //      InitialFreeQuanta               - Initial quanta value of the allocated KQuotaGate.
    //
    // Returns: STATUS_PENDING - Service successful started. Once this status is returned, that
    //                           instance will accept other operations until Deactive() is called.
    //
    NTSTATUS
    Activate(
        __in ULONGLONG InitialFreeQuanta,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback Callback);

    //* Trigger shutdown on an active KQuotaGate Service instance.
    //
    // Once called, a Deactivated (or deactivating) KQuotaGate will reject any further operations with
    // a Status or result of K_STATUS_SHUTDOWN_PENDING. Any existing queued AcquireContext will be completed
    // with a status of K_STATUS_SHUTDOWN_PENDING.
    //
    VOID
    Deactivate();

    //* Increment the FreeQuanta value of the KQuotaGate by QuantaToRelease; releasing (completing)
    //  as many AcquireContext's as possible in the order in which they were queued via their
    //  StartAcquire() method.
    //
    VOID
    ReleaseQuanta(__in ULONGLONG QuantaToRelease);

    //* Current Quanta value accessor
    ULONGLONG
    GetFreeQuanta()
    {
        return _FreeQuanta;
    }

    class AcquireContext : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(AcquireContext);

    public:

        // Schedule an async wait callback.
        //
        // This method will result in *CallbackPtr being invoked when either the
        // related KAsyncEvent is in a signaled state or Cancel() is called. If
        // Cancel() is called, a value of STATUS_CANCELLED will be returned from
        // Status() to indicate the completion is a result of that cancel. If the
        // parent KQuotaGate is dtor'd, K_STATUS_SHUTDOWN_PENDING will be returned.
        // Otherwise STATUS_SUCCESS is always returned.
        VOID
        StartAcquire(
            __in ULONGLONG DesiredQuanta,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

        #if defined(K_UseResumable)
        // Start an awaitable AcquireContext operation
        //
        // Parameters:
        //      Parent      - Parent KAsyncContext's appt to complete the Callback - optional
        //
        //	Usage: 
        //			NTSTATUS status = co_await myAcquireCtx->StartAcquireAsync(qToAcquire, nullptr)
        //
        ktl::Awaitable<NTSTATUS> StartAcquireAsync(
            __in ULONGLONG DesiredQuanta,
            __in_opt KAsyncContextBase* const Parent) noexcept
        {
            _DesiredQuanta = DesiredQuanta;
            co_return co_await ktl::kasync::InplaceStartAwaiter(*this, Parent, nullptr);
        }
        #endif

        __inline ULONGLONG
        GetDesiredQuanta()
        {
            return _DesiredQuanta;
        }

    protected:
        friend class KQuotaGate;
        AcquireContext(KQuotaGate& Owner, ULONG const OwnerVersion);

        VOID
        OnStart();

        VOID
        OnCancel();

    protected:
        KListEntry          _Links;
        KQuotaGate::SPtr    _OwningQuotaGate;
        ULONG               _OwnerVersion;
        ULONGLONG           _DesiredQuanta;
    };

    //* Factory method for creating instances of AcquireContext
    virtual NTSTATUS
    CreateAcquireContext(__out AcquireContext::SPtr& Context);

    #if defined(K_UseResumable)
    // Start an awaitable AcquireContext operation
    //
    // Parameters:
    //      Parent      - Parent KAsyncContext's appt to complete the Callback - optional
    //
    //	Usage: 
    //			NTSTATUS status = co_await myGate->StartAcquireAsync(nullptr)
    //
    ktl::Awaitable<NTSTATUS> StartAcquireAsync(
        __in ULONGLONG DesiredQuanta,
        __in_opt KAsyncContextBase* const Parent) noexcept
    {
        AcquireContext::SPtr    ctx;
        NTSTATUS    status = CreateAcquireContext(ctx);
        if (!NT_SUCCESS(status))
        {
            co_return status;
        }
        co_return co_await ctx->StartAcquireAsync(DesiredQuanta, Parent);
    }
    #endif

protected:
    friend class AcquireContext;
    using KAsyncContextBase::Cancel;

    VOID
    StartWait(AcquireContext& Waiter);

    VOID
    CancelWait(AcquireContext& Waiter);

    VOID
    OnStart() override;

    VOID
    OnCancel() override;

    VOID
    OnReuse() override;

    VOID
    UnsafeReleaseActivity(ULONG ByCountOf = 1);

    VOID
    UnsafeAcquireActivity();

protected:
    KSpinLock                       _ThisLock;          // Protects:
        ULONG                           _ThisVersion;   // Incremented on each Completion
        BOOLEAN                         _IsActive;
        KNodeList<AcquireContext>       _Waiters;
        __declspec(align(8))ULONGLONG   _FreeQuanta;
        ULONG                           _ActivityCount;
};

#if defined(K_UseResumable)
class KCoQuotaGate : public KQuotaGate
{
    K_FORCE_SHARED(KCoQuotaGate);

public:
    //* Factory for creation of KCoQuotaGate Corotuine Service instances.
    //
    //  Parameters:
    //      AllocationTag, Allocator        - Standard memory allocation component and tag
    //                                        data member in IType to be used for queuing instances
    //      Context                         - SPtr that points to an allocated KQuotaGate instance
    //
    //  Returns:
    //      STATUS_SUCCESS                  - Context points to an allocated KQuotaGate instance
    //      STATUS_INSUFFICIENT_RESOURCES   - Out of memory
    //
    static NTSTATUS Create(__in ULONG AllocationTag, __in KAllocator& Allocator, __out KSharedPtr<KCoQuotaGate>& Context);

    ktl::Awaitable<NTSTATUS> ActivateAsync(__in ULONGLONG InitialFreeQuanta)
    {
        // Prime our internal start awaiter
        _awaiter.Reuse(*this, nullptr, nullptr);
        _awaiter.SetDoStart(FALSE);
        co_return Activate(InitialFreeQuanta, nullptr, _awaiter.GetInternalCallback());
    }

    ktl::Awaitable<void> DeactivateAsync()
    {
        Deactivate();
        co_await _awaiter;
    }

private:
    ktl::kasync::InplaceStartAwaiter _awaiter;
};
#endif