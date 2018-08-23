
/*++

    (c) 2011-201 by Microsoft Corp. All Rights Reserved.

    KPerfCounter.h

    Description:
      Kernel Tempate Library (KTL): Windows Performance Counters

    How to add perfcounters to your application:

    1. Create performance counter manifests for user and kernel modes.
       The manifests should look very similar but must have different
       guids and different ApplicationIdentities. See
       http://msdn.microsoft.com/en-us/library/windows/desktop/aa373092(v=vs.85).aspx
       for more details on the performance counter schema.

    2. Use the ctrpp tool (http://msdn.microsoft.com/en-us/library/windows/desktop/aa372128(v=vs.85).aspx)
       to validate and convert the manifest into a .RC file. Include
       this .RC file into the binary. Note that this will now cause the
       binary to create MUI files and thus require localization.

    3. Update the sources file to binplace the manifest so that it can
       be shipped and cause the .RC file included in the binary.

    4. Update your code to include implementations of
       KPerfCounterManager, KPerfCounterSet and KPerfCounterSetInstance
       as appropriate.


    How to test/deploy performance counters with your application:

    1. On the target machine deploy the user mode manifest in the same
       directory as the user mode binary and the kernel manifest in the
       same directory as the kernel mode binary. Also deploy the .mui
       file for the binary in the en-US subdirectory from where the
       binary is located.

    2. On the target machine you need to run lodctr.exe to load the
       manifests into the registry and specify on the lodctr command
       line the path to the binary:

       lodctr /m:manifest.um.man %PathToBinary%\MyBinary.Exe


    Note that the name attribute of the CounterSet element in the
    manifest must exactly match the Name parameter to
    CreatePerfCounterSet() for kernel mode as this is how PCW matches
    counter sets. In user mode this does not matter as matching is done
    by GUID.


    History:
      raymcc          1-Dec-2011         Initial version.
      raymcc         13-Dec-2011         Revised version with more permissible Ids.
      alanwar        26-Nov-2013         Documented how to use these classes

--*/

#pragma once


class KPerfCounterSet;
class KPerfCounterManager;
class KPerfCounterSetInstance;

//
//  class KPerfCounterSet
//
//  Used to define a perf counter set, or 'class' of counters. This corresponds to
//  a <counterSet> defintion in the Windows XML-based perf counter V2 manifest.
//
//  Instances of this are created via KPerfCounterManager.
//
class KPerfCounterSet : public KShared<KPerfCounterSet>, public KObject<KPerfCounterSet>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KPerfCounterSet);
    friend class KPerfCounterSetInstance;
    friend class KPerfCounterManager;
public:
    enum CounterSetType
    {
        CounterSetSingleton = 1,
        CounterSetMultiInstance = 2
    };

    enum CounterDataType
    {
        Type_NULL       = 0,
        Type_ULONG      = 1,
        Type_ULONGLONG  = 2
    };

    //
    //  DefineCounter
    //
    //  Defines a perf counter for this counter set.  This is called once
    //  for each counter that goes with this counter set.  Once SpawnInstance
    //  is called, it is no longer possible to make calls to this, as the counter
    //  set definition is 'locked'.
    //
    //  Parameters:
    //      CounterId       The Id of this counter.  This may be any unsigned value,
    //                      but must correspond to the perf counter manifest definition
    //                      for this counter.
    //      Type            One of { Type_ULONG | Type_ULONGLONG }
    //
    NTSTATUS
    DefineCounter(
        __in ULONG CounterId,
        __in ULONG Type
        );


    //
    //  CommitDefinition
    //
    //  Must be called after defining all the counters, but before spawning instances.
    //  This computes the internal required memory layout for the perf lib and locks
    //  it into place.
    //
    //
    NTSTATUS
    CommitDefinition();

    // SpawnInstance
    //
    // Creates an instance of the counter set.  This cannot be called until CommitDefinition()
    // has completed successfully.
    //
    // Parameters:
    //      InstanceName            The name of this instance.  This must not duplicate any existing
    //                              active instance.  May be null if the counterset is a singleton.
    //
    //      NewInst                 Receives the newly created instance.  The instance does not immediately
    //                              begin publishing values to perfmon.  See KPerfCounterSetInstance.
    //
    NTSTATUS
    SpawnInstance(
        __in  LPCWSTR InstanceName,
        __out KSharedPtr<KPerfCounterSetInstance>& NewInst
        );



private:
    enum { eDefining = 1, eCommitted = 2 };

    VOID
    Cleanup();

    BOOLEAN
    LookupCounter(
        __in ULONG Id,
        __out ULONG& OrdinalPos,
        __out ULONG& Type
        );


    //
    //  Constructor
    //
    KPerfCounterSet(
        __in const GUID& CounterSetGuid,
        __in const KWString& CounterSetName,
        __in KSharedPtr<KPerfCounterManager> Parent,
        __in CounterSetType CSetType,
        __in KAllocator& Alloc
        )
        : _Definitions(Alloc), _Allocator(Alloc), _Name(Alloc)
    {
        _CounterSetGuid = CounterSetGuid;
        _Parent = Parent;
        _CSetType = CSetType;
        _State = eDefining;
        _Name = CounterSetName;
        _NextInstanceId = 0;

#if KTL_USER_MODE
        _PCounterSetInf = nullptr;
        _PCounterArray = nullptr;
#else
        _Registration = nullptr;
#endif
    }

private:
    struct _CounterDef
    {
        ULONG _Id;
        ULONG _Type;
    };

    KArray<_CounterDef>               _Definitions;
    KAllocator&                       _Allocator;
    KWString                          _Name;
    KGuid                             _CounterSetGuid;
    KSharedPtr<KPerfCounterManager>   _Parent;
    CounterSetType                    _CSetType;
    ULONG                             _State;
    ULONG                             _NextInstanceId;

#if KTL_USER_MODE
    NTSTATUS
    AllocateAndMap();

    PERF_COUNTERSET_INFO *          _PCounterSetInf;
    PERF_COUNTER_INFO    *          _PCounterArray;

#else   // Kernel mode
    NTSTATUS
    AllocateAndMap();

    PPCW_REGISTRATION               _Registration;

#endif
};



//
//  class KPerfCounterSetInstance
//
//  Models a single instance of a perf counter set.  Instances are obtained by calling
//  KPerfCounterSet::SpawnInstance.
//

class KPerfCounterSetInstance : public KShared<KPerfCounterSetInstance>, public KObject<KPerfCounterSetInstance>
{
    friend class KPerfCounterSet;

    K_FORCE_SHARED_WITH_INHERITANCE(KPerfCounterSetInstance);

public:


    // SetCounterAddress
    //
    // Sets the address of the counter value based on its Id.  This must be
    // called once for each counter in order to set the source address for the counter value.
    // After completing all the addresses, Start() is used to begin publishing data.
    //
    // It is legal not to supply addresses for all the counter values in the counter set.
    //
    // Parameters:
    //      CounterId           The Id of the counter whose address is being mapped.
    //      Address             The address of the raw counter value. This must match in data
    //                          type to the counter Id registered using KPerfCounter::DefineCounter.
    //                          This address must remain valid for the duration of this instance.
    //
    NTSTATUS
    SetCounterAddress(
        __in ULONG CounterId,
        __in PVOID Address
        );

    //
    //  Start
    //
    //  Exposes the instance to perfmon and other Windows perf counter tools.  Until this
    //  call is made, the instance is not available.
    //
    //
    NTSTATUS
    Start();

    //
    //  Stop
    //
    //  Suspends publishing perf data for this instance and removes the instance.
    //  This will be called automatically if this instance goes out of scope, so it is
    //  not required.
    //
    //  Start() may be called again after Stop() if there is a need to show the instance
    //  coming and going in perfmon.
    //
    NTSTATUS
    Stop();

private:

    KPerfCounterSetInstance(
        __in LPCWSTR InstanceName,
        __in KSharedPtr<KPerfCounterSet> Parent,
        __in ULONG InstanceId
        )
        :   _Definitions(GetThisAllocator()),
            _InstanceName(GetThisAllocator(), InstanceName)
    {
        _Parent = Parent;
        _InstanceId = InstanceId;
        _State = eSetAddresses;

#if KTL_USER_MODE
        _CounterSetInst = nullptr;
#else
        _PcwInstance = 0;
#endif
    };

    VOID
    Cleanup();


private:
    enum { eSetAddresses = 1, eStart = 2, eStop = 3  };
    struct _CounterDef
    {
        ULONG  _Id;
        PVOID  _Address;    // May be a ULONG* or ULONGLONG*
    };

    KSharedPtr<KPerfCounterSet>  _Parent;

    KArray<_CounterDef>          _Definitions;
    ULONG                        _State;
    KWString                     _InstanceName;
    ULONG                        _InstanceId;

#if KTL_USER_MODE
    PERF_COUNTERSET_INSTANCE*    _CounterSetInst;
#else
    PPCW_INSTANCE                _PcwInstance;
#endif
};



//
//  KPerfCounterManager
//
//  The rootmost object which is used to initialize the system and begin creating and mapping perf counter sets.
//
//  In user mode, this hosts the actual perf provider object.  In kernel mode, this acts as a convenient
//  root object for creating perf counter sets.
//
class KPerfCounterManager : public KObject<KPerfCounterManager>, public KShared<KPerfCounterManager>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KPerfCounterManager);
    friend class KPerfCounterSet;
    friend class KPerfCounterSetInstance;

public:

    //  Create
    //
    //  Creats an instance of the manager.  There should only be one instance perf EXE or SYS file.
    //
    //  Paramaters:
    //      ProviderGuid        The GUID of the provider. This must correspond to the providerGuid in the
    //                          event counter manifest.
    //      Alloc               Allocator to use and pass down through the perf counter object chain.
    //      Mgr                 Receives the newly created instance of the manager.
    //
    //
    static NTSTATUS
    Create(
        __in  const GUID& ProviderGuid,
        __in  KAllocator& Alloc,
        __out KPerfCounterManager::SPtr& Mgr
        );

    //  CreatePerfCounterSet
    //
    //  Used to create a set or "class" of performance counters.
    //
    //  Parameters:
    //      Name            The name of the counter set.  This must correspond precisely to the counterSet/name value
    //                      in the manifest.
    //      CounterSetId    The GUID for this counter set.  This must correspond precisely to the counterSet/guid value in
    //                      the manifest.
    //      Type            KPerfCounterSet::CounterSetSingleton or KPerfCounterSet::CounterSetMultiInstance. This must
    //                      correspond to counterSet/instances in the manifest.
    //      NewSet          Receives the new counter set object. This object is unformatted and the caller must begin defining
    //                      each counter to match the definitions in the manifest.
    //
    NTSTATUS
    CreatePerfCounterSet(
        __in  const KWString& Name,
        __in  const GUID& CounterSetId,
        __in  KPerfCounterSet::CounterSetType Type,
        __out KPerfCounterSet::SPtr& NewSet
        );

private:

#if KTL_USER_MODE
    HANDLE
    GetProvider()
    {
        return _hThisProvider;
    }
#endif

    NTSTATUS
    Initialize();

    VOID
    Cleanup();

    KPerfCounterManager(
        __in const GUID& ProviderGuid,
        __in KAllocator& Alloc
        )
        : _Allocator(Alloc)
    {
        _ProviderGuid = ProviderGuid;
#if KTL_USER_MODE
        _hThisProvider = NULL;
#endif
    }

private:

#if KTL_USER_MODE
    HANDLE      _hThisProvider;
#endif
    KAllocator& _Allocator;
    KGuid       _ProviderGuid;
};


