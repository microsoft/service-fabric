#include <KtlSystemCoreImp.h>

static KtlSystemCoreImp* DefaultKtlSystemCoreImpSingleton = nullptr;

#if KTL_USER_MODE
KSpinLock KtlSystemCoreImp::gs_SingletonLock;
#endif

KtlSystemCore&
KtlSystemCoreImp::GetDefaultKtlSystemCore()
{
    KAssert(DefaultKtlSystemCoreImpSingleton != nullptr);
    return *DefaultKtlSystemCoreImpSingleton;
}

#if KTL_USER_MODE
KtlSystemCore*
KtlSystemCoreImp::TryGetDefaultKtlSystemCore()
{
    KtlSystemCoreImp* system = nullptr;
    K_LOCK_BLOCK(KtlSystemCoreImp::gs_SingletonLock)
    {
        system = DefaultKtlSystemCoreImpSingleton;
        if (system)
        {
            break;
        }

        NTSTATUS status = KtlSystemCoreImp::Create(system);
        if (!NT_SUCCESS(status))
        {
            system = nullptr;
            break;
        }

        DefaultKtlSystemCoreImpSingleton = system;
    }

    return system;
}
#else
KtlSystemCore*
KtlSystemCoreImp::TryGetDefaultKtlSystemCore()
{
    return DefaultKtlSystemCoreImpSingleton;
}
#endif

NTSTATUS
KtlSystemCoreImp::Create(__out KtlSystemCoreImp*& ResultSystem)
{
    KtlSystemCoreImp* result = nullptr;
    ResultSystem = nullptr;

    result = (KtlSystemCoreImp*)ExAllocatePoolWithTag(
#if KTL_USER_MODE
        GetProcessHeap(),
#endif
        NonPagedPool,
        sizeof(KtlSystemCoreImp),
        KTL_TAG_BASE);

    NTSTATUS status;
    if (result == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        result->KtlSystemCoreImp::KtlSystemCoreImp();
        status = result->Status();
        if (!NT_SUCCESS(status))
        {
            result->KtlSystemCoreImp::~KtlSystemCoreImp();
#if KTL_USER_MODE
            ExFreePool(
                GetProcessHeap(),
                result);
#else
            ExFreePool(result);
#endif
            result = nullptr;
        }
        else
        {
            ResultSystem = result;
        }
    }

    return status;
}

KAllocator&
KtlSystemCoreImp::NonPagedAllocator()
{
    KFatal(Status() == STATUS_SUCCESS);
    return *_NonPagedAllocator;
}

KAllocator&
KtlSystemCoreImp::PagedAllocator()
{
    KFatal(Status() == STATUS_SUCCESS);
    return *_PagedAllocator;
}

KAllocator&
KtlSystemCoreImp::PageAlignedRawAllocator()
{
    KFatal(Status() == STATUS_SUCCESS);
    return *_RawPageAlignedAllocator;
}

//** KtlSystemCoreImp Implementation
KtlSystemCoreImp::KtlSystemCoreImp()
{
#if KTL_USER_MODE
    _HeapHandle = nullptr;
    //
    // For user mode, create our own heap dedicated to this KtlSystem
    //
    _HeapHandle = HeapCreate(0,          // No Options
                             0,          // start with 1 page
                             0);         // No maximum size

    if (_HeapHandle == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
#endif

    // Create the allocators
    _NonPagedAllocator = nullptr;
    _PagedAllocator = nullptr;
    _RawPageAlignedAllocator = nullptr;

    // This "first" allocation must be done from an external heap - not using any of the "new"
    // plumbing.
    _SystemHeap = (NonPagedKAllocator*)ExAllocatePoolWithTag(
#if KTL_USER_MODE
        _HeapHandle,
#endif
        NonPagedPool,
        sizeof(NonPagedKAllocator),
        KTL_TAG_BASE);

    if (_SystemHeap == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
    _SystemHeap->KtlCoreNonPageAlignedAllocator<NonPagedPool>::KtlCoreNonPageAlignedAllocator(
#if KTL_USER_MODE
        _HeapHandle,
#endif
        nullptr);    // CTOR

    // Now we can use _SystemHeap for the internal KtlSystemCoreImp state - using the normal
    // "new" plumbing
    _NonPagedAllocator = _new(KTL_TAG_BASE, *_SystemHeap) NonPagedKAllocator(
#if KTL_USER_MODE
        _HeapHandle,
#endif
        nullptr);
    if (_NonPagedAllocator == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    _PagedAllocator = _new(KTL_TAG_BASE, *_SystemHeap) PagedKAllocator(
#if KTL_USER_MODE
        _HeapHandle,
#endif
        nullptr);
    if (_PagedAllocator == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    _RawPageAlignedAllocator = _new(KTL_TAG_BASE, *_SystemHeap) KtlCorePageAlignedAllocator(nullptr);
    if (_RawPageAlignedAllocator == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
}

KtlSystemCoreImp::~KtlSystemCoreImp()
{
    // dtor the allocators
    if (_RawPageAlignedAllocator != nullptr)
    {
        _delete(_RawPageAlignedAllocator);
        _RawPageAlignedAllocator = nullptr;
    }
    if (_PagedAllocator != nullptr)
    {
        _delete(_PagedAllocator);
        _PagedAllocator = nullptr;
    }
    if (_NonPagedAllocator != nullptr)
    {
        _delete(_NonPagedAllocator);
        _NonPagedAllocator = nullptr;
    }

    // Do last
    if (_SystemHeap != nullptr)
    {
        _SystemHeap->~KtlCoreNonPageAlignedAllocator();
#if KTL_USER_MODE
        ExFreePool(_HeapHandle, _SystemHeap);
#else
        ExFreePool(_SystemHeap);
#endif
        _SystemHeap = nullptr;
    }

#if KTL_USER_MODE
    if (_HeapHandle != nullptr)
    {
        HeapDestroy(_HeapHandle);
        _HeapHandle = nullptr;
    }
#endif
}

#if KTL_USER_MODE
    void 
    KtlSystemCoreImp::SetEnableAllocationCounters(bool ToEnabled)
    {
        _PagedAllocator->SetEnableAllocationCounters(ToEnabled);
        _NonPagedAllocator->SetEnableAllocationCounters(ToEnabled);
    }
#endif
