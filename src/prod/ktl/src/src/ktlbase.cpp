/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    ktlbase.cpp

    Description:
      Kernel Tempate Library (KTL)

      Implements global allocators and other system-wide entry points.

    History:
      raymcc          20-Aug-2010         Initial version.

--*/

#include <ktl.h>

//
// Global data structures used by KSystem.
//


//* Grandfathered KtlSystemDefaultImp imp - may be removed
class KtlSystemImp : public KtlSystemBase
{
public:
    static NTSTATUS
    Create(
        __out KtlSystemImp*& ResultSystem
        );
    
    FAILABLE
    KtlSystemImp()
        :   KtlSystemBase(KGuid())
    {
    }

    ~KtlSystemImp()
    {
    }

    VOID Delete(__in ULONG TimeoutInMs = 5000);    

#if KTL_USER_MODE
	static KGlobalSpinLockStorage      gs_GrandFatheredSingletonLock;
#endif
    
    BOOLEAN _EnableVNetworking;
};

KtlSystemImp* GrandFatheredKtlSystemSingleton = nullptr;

#if KTL_USER_MODE
KGlobalSpinLockStorage KtlSystemImp::gs_GrandFatheredSingletonLock;
#endif

NTSTATUS
KtlSystemImp::Create(
    __out KtlSystemImp*& ResultSystem
    )
{
    ResultSystem = nullptr;
    
    ResultSystem = (KtlSystemImp*)ExAllocatePoolWithTag(
#if KTL_USER_MODE
        //
        // Initial KTLSystem is created in the process heap - all other allocations are made in the
        // heap associated with KtlSystem. This heap is created in the KtlSystemBase constructor.
        //
        GetProcessHeap(),
#endif
        NonPagedPool,
        sizeof(KtlSystemImp),
        KTL_TAG_BASE);

    NTSTATUS status;
    if (ResultSystem == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        ResultSystem->KtlSystemImp::KtlSystemImp();
        status = ResultSystem->Status();
        if (!NT_SUCCESS(status))
        {
            ResultSystem->KtlSystemImp::~KtlSystemImp();
#if KTL_USER_MODE
            ExFreePool(
                GetProcessHeap(),
                ResultSystem);
#else
            ExFreePool(ResultSystem);
#endif
            ResultSystem = nullptr;
        }
        else
        {            
            // Grandfathered behavior
            ResultSystem->SetStrictAllocationChecks(FALSE);
            ResultSystem->Activate(KtlSystem::InfiniteTime);
            status = ResultSystem->Status();
            if (!NT_SUCCESS(status))
            {
                ResultSystem->Delete();
                ResultSystem = nullptr;
            }   
        }
    }

    return status;
}

VOID
KtlSystemImp::Delete(
    __in ULONG TimeoutInMs                   
    )
{
    ULONG       timeout = this->GetStrictAllocationChecks() ? TimeoutInMs : KtlSystem::InfiniteTime;
    this->Deactivate(timeout);
    this->KtlSystemImp::~KtlSystemImp();
#if KTL_USER_MODE
    ExFreePool(
        GetProcessHeap(),
        this);
#else
    ExFreePool(this);
#endif
}

NTSTATUS
KtlSystem::Initialize(
    __in BOOLEAN EnableVNetworking,                   
    __out_opt KtlSystem* *const OptResultSystem
    )
{
    KFatal(GrandFatheredKtlSystemSingleton == nullptr);
    NTSTATUS status = KtlSystemImp::Create(GrandFatheredKtlSystemSingleton);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (OptResultSystem != nullptr)
    {
        *OptResultSystem = GrandFatheredKtlSystemSingleton;
    }
#if !defined(PLATFORM_UNIX)
    if (EnableVNetworking)
    {
        status = VNetwork::Startup(GlobalNonPagedAllocator());
    }
#endif    
    GrandFatheredKtlSystemSingleton->_EnableVNetworking = EnableVNetworking;
    
    return status;
}

NTSTATUS
KtlSystem::Initialize(
    __out_opt KtlSystem* *const OptResultSystem
    )
{
    return Initialize(TRUE,
                      OptResultSystem);
}


VOID
KtlSystem::Shutdown(
    __in ULONG TimeoutInMs
    )
{
    if (GrandFatheredKtlSystemSingleton != nullptr)
    {
#if !defined(PLATFORM_UNIX)
        if (GrandFatheredKtlSystemSingleton->_EnableVNetworking)
        {
            VNetwork::Shutdown();
        }
#endif
        GrandFatheredKtlSystemSingleton->Delete(TimeoutInMs);
        GrandFatheredKtlSystemSingleton = nullptr;
    }
}

KtlSystem&
KtlSystem::GetDefaultKtlSystem()
{
    KAssert(GrandFatheredKtlSystemSingleton != nullptr);
    return *GrandFatheredKtlSystemSingleton;
}

