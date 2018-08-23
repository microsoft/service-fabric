

/*++

    (c) 2011-201 by Microsoft Corp. All Rights Reserved.

    KPerfCounter.cpp

    Description:
      Kernel Tempate Library (KTL): Windows Performance Counters

    History:
      raymcc          1-Dec-2011         Initial version.

--*/

#include <ktl.h>


//
//  KPerfCounterSetInstance::SetCounterAddress
//
NTSTATUS
KPerfCounterSetInstance::SetCounterAddress(
    __in ULONG CounterId,
    __in PVOID Address
    )
{
    NTSTATUS Result;
    ULONG Pos, Type;

    KFatal(_State == eSetAddresses);
    KFatal(_Parent->LookupCounter(CounterId, Pos, Type));

    // Verify this is not a duplicate.
    //
    for (ULONG i = 0; i < _Definitions.Count(); i++)
    {
        if (_Definitions[i]._Id == CounterId)
        {
            return STATUS_DUPLICATE_OBJECTID;
        }
    }

    // Add a new definition
    //
    _CounterDef NewDef;
    NewDef._Address = Address;
    NewDef._Id = CounterId;

    // We insert the mapping at the same ordinal
    // position as is defined in the parent KPerfCounterSet
    // object.  This is because kernel mode relies on
    // ordinal position of the counter source address relative
    // to that map, so all the address+id pairs must be in the same
    // ordinal positions as registred in the PCW_COUNTER_DESCRIPTORS.
    // For user mode, this is harmless, although not required.
    //
    Result = _Definitions.InsertAt(Pos, NewDef);
    if (!NT_SUCCESS(Result))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return Result;
}



#if KTL_USER_MODE

//
//  KPerfCounterSetInstance::Start
//  (User Mode)
//
NTSTATUS
KPerfCounterSetInstance::Start()
{
    // If already started no harm done.
    //
    if (_State == eStart)
    {
        return STATUS_SUCCESS;
    }

    // Can't start if no values have been mapped.
    //
    if (_Definitions.Count() == 0)
    {
        return STATUS_BUFFER_ALL_ZEROS;
    }

    // Auto-cleanup on partial failure.
    //
    KFinally([&]() {  if (_State != eStart) { Stop(); } });

    // Create the instance.
    //
    _CounterSetInst = PerfCreateInstance(_Parent->_Parent->GetProvider(), LPCGUID(&_Parent->_CounterSetGuid), PWSTR(_InstanceName), _InstanceId);
    if (_CounterSetInst == NULL)
    {
        _State = eStop;
        return STATUS_INTERNAL_ERROR;
    }

    // Map the addresses of the counter values.
    //
    for (ULONG i = 0; i < _Definitions.Count(); i++)
    {
        ULONG Status = PerfSetCounterRefValue(_Parent->_Parent->GetProvider(), _CounterSetInst, _Definitions[i]._Id,  _Definitions[i]._Address);
        if (Status != ERROR_SUCCESS)
        {
            _State = eStop;
            return STATUS_INTERNAL_ERROR;
        }
    }

    _State = eStart;
    return STATUS_SUCCESS;
}
#else

//
// KPerfCounterSetInstance::Start
// (Kernel mode)
//
NTSTATUS
KPerfCounterSetInstance::Start()
{
    NTSTATUS Result;
    PPCW_DATA pData = nullptr;

    // No harm done if we are already started.
    //
    if (_State == eStart)
    {
        return STATUS_SUCCESS;
    }

    if (_Definitions.Count() == 0)
    {
        return STATUS_BUFFER_ALL_ZEROS;
    }

    // Map the data fields.
    //
    pData = _new(KTL_TAG_PERFCTR, GetThisAllocator()) PCW_DATA[_Definitions.Count()];
    if (!pData)
    {
        _State = eStop;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Auto-cleanup on partial failure.
    //
    KFinally([&]()
        {
           if (_State != eStart) { Stop(); }
          _delete(pData);
        }
        );

    // This loop relies on the precondition that the counter value addresses are laid
    // out in the same order as in PcwRegister.  The ordering is enforced in SetCounterAddress,
    // so it is not necessary here.
    //
    for (ULONG i = 0; i < _Definitions.Count(); i++)
    {
        ULONG Size = 0;
        ULONG Type, Pos;

        if (!_Parent->LookupCounter(_Definitions[i]._Id, Pos, Type))
        {
            _State = eStop;
            return STATUS_INTERNAL_ERROR;
        }

        switch (Type)
        {
            case KPerfCounterSet::Type_ULONG : Size = sizeof(ULONG); break;
            case KPerfCounterSet::Type_ULONGLONG: Size = sizeof(ULONGLONG); break;
        }

        pData[i].Data = _Definitions[i]._Address;
        pData[i].Size = Size;
    }

    // Create the instance.
    //
    Result = PcwCreateInstance(&_PcwInstance, _Parent->_Registration, PUNICODE_STRING(_InstanceName), _Definitions.Count(), pData);
    if (!NT_SUCCESS(Result))
    {
        _State = eStop;
        return Result;
    }

    _State = eStart;
    return STATUS_SUCCESS;
}
#endif


//
//  KPerfCounterSetInstance::Stop
//
NTSTATUS
KPerfCounterSetInstance::Stop()
{
    Cleanup();
    _State = eStop;
    return STATUS_SUCCESS;
}


//
//  KPerfCounterSetInstance::~KPerfCounterSetInstance
//
KPerfCounterSetInstance::~KPerfCounterSetInstance()
{
    Stop();
}

#if KTL_USER_MODE
//
// KPerfCounterSetInstance::Cleanup
//
VOID
KPerfCounterSetInstance::Cleanup()
{
    if (_CounterSetInst)
    {
        PerfDeleteInstance(_Parent->_Parent->GetProvider(), _CounterSetInst);
        _CounterSetInst = NULL;
    }
}
#else

//
// KPerfCounterSetInstance::Cleanup
//
VOID
KPerfCounterSetInstance::Cleanup()
{
    if (_PcwInstance)
    {
        PcwCloseInstance(_PcwInstance);
        _PcwInstance = nullptr;
    }
}


#endif

//
//  KPerfCounterSet::CommitDefinition
//
NTSTATUS
KPerfCounterSet::CommitDefinition()
{
    NTSTATUS Result;

    KFatal(_State == eDefining);
    _State = eCommitted;

    Result = AllocateAndMap();
    return Result;
}

//
//  KPerfCounterSet::DefineCounter
//
NTSTATUS
KPerfCounterSet::DefineCounter(
    __in ULONG CounterId,
    __in ULONG Type
    )
{
    NTSTATUS Res;
    KFatal(_State == eDefining);

    ULONG ExistingType, ExistingPosition;

    if (LookupCounter(CounterId, ExistingPosition, ExistingType))
    {
        return STATUS_DUPLICATE_OBJECTID;
    }
    _CounterDef Def;
    Def._Id = CounterId;
    Def._Type = Type;
    Res = _Definitions.Append(Def);
    return Res;
}


//
//  KPerfCounterSet::LookupCounter
//
BOOLEAN
KPerfCounterSet::LookupCounter(
    __in ULONG Id,
    __out ULONG& OrdinalPos,
    __out ULONG& Type
    )
{
    for (ULONG i = 0; i < _Definitions.Count(); i++)
    {
        if (Id == _Definitions[i]._Id)
        {
            OrdinalPos = i;
            Type = _Definitions[i]._Type;
            return TRUE;
        }
    }

    OrdinalPos = ULONG(-1);
    Type = Type_NULL;
    return FALSE;
}


//
//  KPerfCounterSet::AllocateAndMap
//  (User mode)
//  This maps the counter set (the 'template' or type of perf counter, not instances.
//
//
#if KTL_USER_MODE

NTSTATUS
KPerfCounterSet::AllocateAndMap()
{
    // Allocate the perf-related structs, back to back.
    //
    ULONG SetInfSize = sizeof(PERF_COUNTERSET_INFO);
    ULONG CtrDefSize = sizeof(PERF_COUNTER_INFO);

	ULONG AllocSize;
	HRESULT hr;
	hr = ULongMult(CtrDefSize, _Definitions.Count(), &AllocSize);
	KInvariant(SUCCEEDED(hr));
	hr = ULongAdd(AllocSize, SetInfSize, &AllocSize);
	KInvariant(SUCCEEDED(hr));

	PVOID NewMem = _new(KTL_TAG_PERFCTR, _Allocator) UCHAR[AllocSize];

    if (!NewMem)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(NewMem, AllocSize);

    _PCounterSetInf = (PERF_COUNTERSET_INFO *) NewMem;
    _PCounterArray =  (PPERF_COUNTER_INFO) (PUCHAR(NewMem) + SetInfSize);


    // Now initialize everything.
    //
    _PCounterSetInf->CounterSetGuid = _CounterSetGuid;
    _PCounterSetInf->ProviderGuid = _Parent->_ProviderGuid;
    _PCounterSetInf->NumCounters = _Definitions.Count();
    _PCounterSetInf->InstanceType =  _CSetType == CounterSetSingleton ? PERF_COUNTERSET_SINGLE_INSTANCE :  PERF_COUNTERSET_MULTI_INSTANCES;

    for (ULONG ix = 0; ix < _Definitions.Count(); ix++)
    {
        PERF_COUNTER_INFO* Tmp = &_PCounterArray[ix];

        ULONG Size = 0;
        switch (_Definitions[ix]._Type)
        {
            case Type_ULONG : Size = sizeof(ULONG); break;
            case Type_ULONGLONG: Size = sizeof(ULONGLONG); break;
        }

        Tmp->CounterId = _Definitions[ix]._Id;
        Tmp->Type = PERF_COUNTER_RAWCOUNT;
        Tmp->Attrib = PERF_ATTRIB_BY_REFERENCE;
        Tmp->Size = Size;
        Tmp->DetailLevel = PERF_DETAIL_NOVICE;
        Tmp->Scale = 0;    // TBD: See if this can override what is in the manifest or if we care
        Tmp->Offset = 0;
    }

    ULONG Res = PerfSetCounterSetInfo(_Parent->_hThisProvider, _PCounterSetInf, AllocSize);
    if (Res != 0)
    {
        _delete(_PCounterSetInf);
        _PCounterSetInf = nullptr;
         return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}



#else   // Kernel



//
//  KPerfCounterSet::AllocateAndMap
//
//  (Kernel mode)
//
NTSTATUS
KPerfCounterSet::AllocateAndMap()
{
    NTSTATUS Result;
    PCW_REGISTRATION_INFORMATION  RegInfo;
    RtlZeroMemory(&RegInfo, sizeof(PCW_REGISTRATION_INFORMATION));

    // Allocate and populate the descriptors.
    //
    PPCW_COUNTER_DESCRIPTOR Descriptors = _new(KTL_TAG_PERFCTR, _Allocator) PCW_COUNTER_DESCRIPTOR[_Definitions.Count()];
    if (!Descriptors)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    KFinally([&]() { _delete(Descriptors); } );

    for (ULONG i = 0; i < _Definitions.Count(); i++)
    {
        ULONG Size = 0;
        switch (_Definitions[i]._Type)
        {
            case Type_ULONG : Size = sizeof(ULONG); break;
            case Type_ULONGLONG: Size = sizeof(ULONGLONG); break;
        }

        Descriptors[i].Id = USHORT(_Definitions[i]._Id);
        Descriptors[i].StructIndex = (USHORT) i;
        Descriptors[i].Offset = 0;
        Descriptors[i].Size = (USHORT) Size;
    }


    // Register everything.
    //
    RegInfo.Version = PCW_CURRENT_VERSION;
    RegInfo.Counters = Descriptors;
    RegInfo.CounterCount = _Definitions.Count();
    RegInfo.Callback = nullptr;
    RegInfo.CallbackContext = nullptr;
    RegInfo.Name = PUNICODE_STRING(_Name);

    Result = PcwRegister(&_Registration, &RegInfo);
    return Result;
}

#endif



//
//  KPerfCounterSet::SpawnInstance
//
//  InstanceName not required for singletons and can be NULL.
//
NTSTATUS
KPerfCounterSet::SpawnInstance(
    __in  LPCWSTR InstanceName,
    __out KPerfCounterSetInstance::SPtr& NewInst
    )
{
    KWString Tmp(_Allocator);
    KFatal(_State == eCommitted);

    if (InstanceName == nullptr)
    {
        if (_CSetType == CounterSetMultiInstance)
        {
            return STATUS_OBJECT_TYPE_MISMATCH;
        }
        else
        {
            Tmp = L"<singleton>";
        }
    }
    else
    {
        Tmp = InstanceName;
    }

    ULONG NextId = InterlockedIncrement(PLONG(&_NextInstanceId));
    NewInst = _new(KTL_TAG_PERFCTR, _Allocator) KPerfCounterSetInstance(Tmp, this, NextId);
    if (!NewInst)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}


//
//  KPerfCounterSet::~KPerfCounterSet
//
KPerfCounterSet::~KPerfCounterSet()
{
    Cleanup();
}

//
//  KPerfCounterSet::Cleanup
//
VOID
KPerfCounterSet::Cleanup()
{
#if KTL_USER_MODE
    _delete(_PCounterSetInf);
    _PCounterSetInf = nullptr;
#else
    PcwUnregister(_Registration);
    _Registration = nullptr;
#endif
}


//
//  KPerfCounterManager::Create
//
NTSTATUS
KPerfCounterManager::Create(
    __in  const GUID& ProviderGuid,
    __in  KAllocator& Alloc,
    __out KPerfCounterManager::SPtr& Mgr
    )
{
    Mgr = _new (KTL_TAG_PERFCTR, Alloc) KPerfCounterManager(ProviderGuid, Alloc);
    if (!Mgr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS Res = Mgr->Initialize();
    return Res;
}


//
//  KPerfCounterManager::Initialize
//
#if KTL_USER_MODE
NTSTATUS
KPerfCounterManager::Initialize()
{
    PERF_PROVIDER_CONTEXT ProviderContext;

    ZeroMemory(&ProviderContext, sizeof(PERF_PROVIDER_CONTEXT));
    ProviderContext.ContextSize = sizeof(PERF_PROVIDER_CONTEXT);
    ProviderContext.ControlCallback = nullptr;
    ProviderContext.MemAllocRoutine = nullptr;
    ProviderContext.MemFreeRoutine = nullptr;
    ProviderContext.pMemContext = nullptr;

    ULONG Status = PerfStartProviderEx(&_ProviderGuid,
                                 &ProviderContext,
                                 &_hThisProvider);

    if (Status != ERROR_SUCCESS)
    {
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}
#else

//
//  KPerfCounterManager::Initialize
//
//  In kernel mode, there is no provider-level ininitialization.
//
NTSTATUS
KPerfCounterManager::Initialize()
{
    return STATUS_SUCCESS;
}

#endif


//
// KPerfCounterManager::~KPerfCounterManager
//
KPerfCounterManager::~KPerfCounterManager()
{
    Cleanup();
}

//
//  KPerfCounterManager::Cleanup
//

#if KTL_USER_MODE
VOID
KPerfCounterManager::Cleanup()
{
    if (_hThisProvider)
    {
        PerfStopProvider(_hThisProvider);
        _hThisProvider = NULL;
    }
}
#else
VOID KPerfCounterManager::Cleanup()
{

}

#endif


//
//  KPerfCounterManager::CreatePerfCounterSet
//
NTSTATUS
KPerfCounterManager::CreatePerfCounterSet(
    __in const KWString& CounterSetName,
    __in const GUID& CounterSetId,
    __in KPerfCounterSet::CounterSetType Type,
    __out KPerfCounterSet::SPtr& NewSet
    )
{
    if (!(Type == KPerfCounterSet::CounterSetSingleton || Type == KPerfCounterSet::CounterSetMultiInstance))
    {
        return STATUS_INVALID_PARAMETER;
    }

    NewSet = _new(KTL_TAG_PERFCTR, _Allocator) KPerfCounterSet(CounterSetId, CounterSetName, this, Type, _Allocator);

    if (!NewSet)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}




