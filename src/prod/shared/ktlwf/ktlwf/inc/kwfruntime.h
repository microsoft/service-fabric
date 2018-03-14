// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <kwftypes.h>

//
// Forward declarations
//

class KWfStatelessServicePartition;
class KWfStatelessServiceInstance;
class KWfStatelessServiceFactory;

class KWfStatefulServiceFactory;
class KWfStatefulServicePartition;
class KWfStatefulServiceReplica;
class KWfStateProvider;

class KWfStateReplicator;
class KWfReplicator;
class KWfPrimaryReplicator;

class KWfOperationData;
class KWfOperationDataAsyncEnumerator;

class KWfOperation;
class KWfOperationStream;

//
// The equivalent of IFabricStatelessServicePartition
//

class KWfStatelessServicePartition : public KObject<KWfStatelessServicePartition>, public KShared<KWfStatelessServicePartition>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfStatelessServicePartition);

public:

    virtual NTSTATUS
    GetPartitionInfo(
        __out const FABRIC_SERVICE_PARTITION_INFORMATION** BufferedValue
        ) = 0;

    virtual NTSTATUS 
    ReportLoad(
        __in ULONG MetricCount,
        __in_ecount(MetricCount) const FABRIC_LOAD_METRIC* Metrics
        ) = 0;
};

inline KWfStatelessServicePartition::KWfStatelessServicePartition() {}
inline KWfStatelessServicePartition::~KWfStatelessServicePartition() {}

//
// The equivalent of IFabricStatelessServiceInstance
//

class KWfStatelessServiceInstance : public KObject<KWfStatelessServiceInstance>, public KShared<KWfStatelessServiceInstance>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfStatelessServiceInstance);

public:

    //
    // Methods to pre-allocate async context objects
    //

    virtual
    NTSTATUS
    AllocateOpenContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_WINFAB
        ) = 0;

    virtual
    NTSTATUS
    AllocateCloseContext(
        __out KAsyncContextBase::SPtr& Async,
        __in ULONG AllocationTag = KTL_TAG_WINFAB
        ) = 0;

    //
    // Service instance APIs.
    //

    virtual NTSTATUS
    Open(
        __in KWfStatelessServicePartition::SPtr Partition,
        __out KWString& ServiceAddress,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;

    virtual NTSTATUS
    Close(
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;

    virtual VOID Abort() = 0;
};

inline KWfStatelessServiceInstance::KWfStatelessServiceInstance() {}
inline KWfStatelessServiceInstance::~KWfStatelessServiceInstance() {}

//
// The equivalent of IFabricStatelessServiceFactory
//

class KWfStatelessServiceFactory : public KObject<KWfStatelessServiceFactory>, public KShared<KWfStatelessServiceFactory>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfStatelessServiceFactory);

public:

    virtual NTSTATUS
    CreateInstance(
        __in LPCWSTR ServiceType,
        __in FABRIC_URI ServiceName,
        __in ULONG InitializationDataLength,
        __in_bcount(InitializationDataLength) const byte *InitializationData,
        __in FABRIC_PARTITION_ID PartitionId,        
        __in FABRIC_INSTANCE_ID InstanceId,
        __out_opt KWfStatelessServiceInstance::SPtr* ServiceInstance
        ) = 0;
};

inline KWfStatelessServiceFactory::KWfStatelessServiceFactory() {}
inline KWfStatelessServiceFactory::~KWfStatelessServiceFactory() {}

//
// The equivalent of IFabricStatefulServiceReplica
//

class KWfStatefulServiceReplica : public KObject<KWfStatefulServiceReplica>, public KShared<KWfStatefulServiceReplica>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfStatefulServiceReplica);

public:

    virtual NTSTATUS
    Open(
        __in FABRIC_REPLICA_OPEN_MODE OpenMode,
        __in KWfStatefulServicePartition* Partition,        
        __out KSharedPtr<KWfReplicator>& Replicator,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;

    virtual NTSTATUS
    ChangeRole(
        __in FABRIC_REPLICA_ROLE NewRole,
        __out KWString& ServiceAddress,        
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;    

    virtual NTSTATUS
    Close(
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;

    virtual VOID Abort() = 0;        
};

inline KWfStatefulServiceReplica::KWfStatefulServiceReplica() {}
inline KWfStatefulServiceReplica::~KWfStatefulServiceReplica() {}

//
// The equivalent of IFabricStatefulServiceFactory
//

class KWfStatefulServiceFactory : public KObject<KWfStatefulServiceFactory>, public KShared<KWfStatefulServiceFactory>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfStatefulServiceFactory);

public:

    virtual NTSTATUS
    CreateReplica(
        __in LPCWSTR ServiceType,
        __in FABRIC_URI ServiceName,
        __in ULONG InitializationDataLength,
        __in_bcount(InitializationDataLength) const byte *InitializationData,
        __in FABRIC_PARTITION_ID PartitionId,
        __in FABRIC_REPLICA_ID ReplicaId,
        __out_opt KWfStatefulServiceReplica::SPtr* ServiceReplica
        ) = 0;
};

inline KWfStatefulServiceFactory::KWfStatefulServiceFactory() {}
inline KWfStatefulServiceFactory::~KWfStatefulServiceFactory() {}


//
// The equivalent of IFabricStatefulServicePartition
//

class KWfStatefulServicePartition : public KObject<KWfStatefulServicePartition>, public KShared<KWfStatefulServicePartition>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfStatefulServicePartition);

public:

    virtual NTSTATUS 
    GetPartitionInfo( 
        __out const FABRIC_SERVICE_PARTITION_INFORMATION** BufferedValue
        ) = 0;
    
    virtual NTSTATUS
    GetReadStatus(
        __out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* ReadStatus
        ) = 0;

    virtual NTSTATUS
    GetWriteStatus(
        __out FABRIC_SERVICE_PARTITION_ACCESS_STATUS* WriteStatus
        ) = 0;    

    virtual NTSTATUS 
    CreateReplicator( 
        __in KWfStateProvider* StateProvider,
        __in const FABRIC_REPLICATOR_SETTINGS* ReplicatorSettings,
        __out KSharedPtr<KWfReplicator>& Replicator,
        __out KSharedPtr<KWfStateReplicator>& StateReplicator
        ) = 0;    

    virtual NTSTATUS 
    ReportLoad(
        __in ULONG MetricCount,
        __in_ecount(MetricCount) const FABRIC_LOAD_METRIC* Metrics
        ) = 0;
    
    virtual NTSTATUS
    ReportFault( 
        __in FABRIC_FAULT_TYPE FaultType
        ) = 0;    
};

inline KWfStatefulServicePartition::KWfStatefulServicePartition() {}
inline KWfStatefulServicePartition::~KWfStatefulServicePartition() {}

//
// The equivalent of IFabricStateProvider
//

class KWfStateProvider : public KObject<KWfStateProvider>, public KShared<KWfStateProvider>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfStateProvider);

public:

    virtual NTSTATUS 
    UpdateEpoch( 
        __in const FABRIC_EPOCH* Epoch,
        __in FABRIC_SEQUENCE_NUMBER PreviousEpochLastSequenceNumber,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;

    virtual NTSTATUS
    GetLastCommittedSequenceNumber( 
        __out FABRIC_SEQUENCE_NUMBER* SequenceNumber
        ) = 0;

    virtual NTSTATUS
    OnDataLoss(
        __out PBOOLEAN IsStateChanged,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr    
        ) = 0;
    
    virtual NTSTATUS 
    GetCopyContext( 
        __out KSharedPtr<KWfOperationDataAsyncEnumerator>& CopyContextEnumerator
        ) = 0;
    
    virtual NTSTATUS
    GetCopyState( 
        __in FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
        __in KWfOperationDataAsyncEnumerator* CopyContextEnumerator,
        __out KSharedPtr<KWfOperationDataAsyncEnumerator>& CopyStateEnumerator
        ) = 0;        
};

inline KWfStateProvider::KWfStateProvider() {}
inline KWfStateProvider::~KWfStateProvider() {}

//
// The equivalent of IFabricStateReplicator
//

class KWfStateReplicator : public KObject<KWfStateReplicator>, public KShared<KWfStateReplicator>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfStateReplicator);

public:

    virtual NTSTATUS
    Replicate(
        __in KWfOperationData* OperationData,
        __out FABRIC_SEQUENCE_NUMBER* SequenceNumber,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;

    virtual NTSTATUS
    GetReplicationStream(
        __out KSharedPtr<KWfOperationStream>& Stream
        ) = 0;
    
    virtual NTSTATUS
    GetCopyStream(
        __out KSharedPtr<KWfOperationStream>& Stream
        ) = 0;

    virtual NTSTATUS
    UpdateReplicatorSettings( 
        __in const FABRIC_REPLICATOR_SETTINGS* ReplicatorSettings
        ) = 0;
};

inline KWfStateReplicator::KWfStateReplicator() {}
inline KWfStateReplicator::~KWfStateReplicator() {}

//
// The equivalent of IFabricReplicator
//

class KWfReplicator : public KRTT, public KObject<KWfReplicator>, public KShared<KWfReplicator>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfReplicator);
    K_RUNTIME_TYPED(KWfReplicator);

public:

    virtual NTSTATUS
    Open(
        __out KWString& ReplicationAddress,            
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;

    virtual NTSTATUS
    ChangeRole(
        __in const FABRIC_EPOCH* Epoch,
        __in FABRIC_REPLICA_ROLE NewRole,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;    

    virtual NTSTATUS
    UpdateEpoch(
        __in const FABRIC_EPOCH* Epoch,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;    

    virtual NTSTATUS
    Close(
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;

    virtual VOID Abort() = 0;        
    
    virtual NTSTATUS
    GetCurrentProgress( 
        __out FABRIC_SEQUENCE_NUMBER* LastSequenceNumber
        ) = 0;
    
    virtual NTSTATUS 
    GetCatchUpCapability( 
        __out FABRIC_SEQUENCE_NUMBER* FromSequenceNumber
        ) = 0;        
};

inline KWfReplicator::KWfReplicator() {}
inline KWfReplicator::~KWfReplicator() {}

//
// The equivalent of IFabricPrimaryReplicator
//

class KWfPrimaryReplicator : public KWfReplicator
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfPrimaryReplicator);
    K_RUNTIME_TYPED(KWfPrimaryReplicator);

public:
    
    virtual NTSTATUS
    OnDataLoss(
        __out PBOOLEAN IsStateChanged,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr    
        ) = 0;

    virtual NTSTATUS
    UpdateCatchUpReplicaSetConfiguration(
        __in const FABRIC_REPLICA_SET_CONFIGURATION* CurrentConfiguration,
        __in const FABRIC_REPLICA_SET_CONFIGURATION* PreviousConfiguration
        ) = 0;
    
    virtual NTSTATUS
    WaitForCatchUpQuorum(
        __in FABRIC_REPLICA_SET_QUORUM_MODE CatchUpMode,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr    
        ) = 0;

    virtual NTSTATUS
    UpdateCurrentReplicaSetConfiguration(
        __in const FABRIC_REPLICA_SET_CONFIGURATION* CurrentConfiguration
        ) = 0;
    
    virtual NTSTATUS
    BuildReplica(
        __in const FABRIC_REPLICA_INFORMATION* Replica,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr    
        ) = 0;

    virtual NTSTATUS
    RemoveReplica(
        __in FABRIC_REPLICA_ID ReplicaId
        ) = 0;        
};

inline KWfPrimaryReplicator::KWfPrimaryReplicator() {}
inline KWfPrimaryReplicator::~KWfPrimaryReplicator() {}

//
// The equivalent of IFabricOperation
//

class KWfOperation : public KObject<KWfOperation>, public KShared<KWfOperation>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfOperation);

public:

    virtual const FABRIC_OPERATION_METADATA* 
    get_Metadata() = 0;    

    virtual NTSTATUS 
    GetData( 
        __out PULONG Count,
        __deref_out_ecount(*Count) const FABRIC_OPERATION_DATA_BUFFER** Buffers
        ) = 0;

    virtual NTSTATUS 
    Acknowledge() = 0;    
};

inline KWfOperation::KWfOperation() {}
inline KWfOperation::~KWfOperation() {}

//
// The equivalent of IFabricOperationData
//

class KWfOperationData : public KObject<KWfOperationData>, public KShared<KWfOperationData>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfOperationData);

public:

    virtual NTSTATUS 
    GetData( 
        __out PULONG Count,
        __deref_out_ecount(*Count) const FABRIC_OPERATION_DATA_BUFFER** Buffers
        ) = 0;
};

inline KWfOperationData::KWfOperationData() {}
inline KWfOperationData::~KWfOperationData() {}

//
// The equivalent of IFabricOperationStream
//

class KWfOperationStream : public KObject<KWfOperationStream>, public KShared<KWfOperationStream>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfOperationStream);

public:

    virtual NTSTATUS 
    GetOperation( 
        __out KSharedPtr<KWfOperation>& Operation,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;
};

inline KWfOperationStream::KWfOperationStream() {}
inline KWfOperationStream::~KWfOperationStream() {}

//
// The equivalent of IFabricOperationDataAsyncEnumerator
//

class KWfOperationDataAsyncEnumerator : public KObject<KWfOperationDataAsyncEnumerator>, public KShared<KWfOperationDataAsyncEnumerator>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfOperationDataAsyncEnumerator);

public:

    virtual NTSTATUS 
    GetNext( 
        __out KSharedPtr<KWfOperationData>& OperationData,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr
        ) = 0;
};

inline KWfOperationDataAsyncEnumerator::KWfOperationDataAsyncEnumerator() {}
inline KWfOperationDataAsyncEnumerator::~KWfOperationDataAsyncEnumerator() {}

//
// The equivalent of IFabricRuntime
//

class KWfRuntime : public KObject<KWfRuntime>, public KShared<KWfRuntime>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfRuntime);

public:

    static
    NTSTATUS
    Create(
        __in KAllocator& Allocator,
        __out KWfRuntime::SPtr& Runtime,
        __in ULONG AllocationTag = KTL_TAG_WINFAB,
        __in_opt GUID const * IID = nullptr
        );

    static
    NTSTATUS
    AsyncCreate(
        __in KAllocator& Allocator,
        __out KWfRuntime::SPtr& Runtime,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __inout_opt KAsyncContextBase::SPtr *ChildAsync = nullptr,
        __in ULONG AllocationTag = KTL_TAG_WINFAB,
        __in_opt GUID const * IID = nullptr
        );


    virtual NTSTATUS 
    RegisterStatelessServiceFactory( 
        __in LPCWSTR ServiceType,
        __in KWfStatelessServiceFactory::SPtr Factory,
        __in ULONG TimeoutMilliseconds,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr
        ) = 0;

    virtual NTSTATUS 
    RegisterStatefulServiceFactory( 
        __in LPCWSTR ServiceType,
        __in KWfStatefulServiceFactory::SPtr Factory,
        __in ULONG TimeoutMilliseconds,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr
        ) = 0;
};

inline KWfRuntime::KWfRuntime() {}
inline KWfRuntime::~KWfRuntime() {}

//
// The equivalent of IFabricCodePackage
//

class KWfCodePackage : public KObject<KWfCodePackage>, public KShared<KWfCodePackage>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfCodePackage);    

public:

    virtual const FABRIC_CODE_PACKAGE_DESCRIPTION*
    get_Description(
        ) = 0;

    virtual LPCWSTR
    get_Path(
        ) = 0;
};

inline KWfCodePackage::KWfCodePackage() {}
inline KWfCodePackage::~KWfCodePackage() {}

//
// The equivalent of IFabricConfigurationPackage
//

class KWfConfigurationPackage : public KObject<KWfConfigurationPackage>, public KShared<KWfConfigurationPackage>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfConfigurationPackage);    

public:

    virtual const FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION*
    get_Description(
        ) = 0;

    virtual LPCWSTR
    get_Path(
        ) = 0;    

    virtual const FABRIC_CONFIGURATION_SETTINGS*
    get_Settings(
        ) = 0;
    
    virtual NTSTATUS
    GetSection(
        __in_z LPCWSTR SectionName,
        __out const FABRIC_CONFIGURATION_SECTION** BufferedValue
        ) = 0;
    
    virtual NTSTATUS
    GetValue(
        __in_z LPCWSTR SectionName,
        __in_z LPCWSTR ParameterName,
        __out BOOLEAN* IsEncrypted,        
        __out KWString& BufferedValue
        ) = 0;
};

inline KWfConfigurationPackage::KWfConfigurationPackage() {}
inline KWfConfigurationPackage::~KWfConfigurationPackage() {}


//
// The equivalent of IFabricDataPackage
//

class KWfDataPackage : public KObject<KWfDataPackage>, public KShared<KWfDataPackage>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfDataPackage);    

public:

    virtual const FABRIC_DATA_PACKAGE_DESCRIPTION*
    get_Description(
        ) = 0;

    virtual LPCWSTR
    get_Path(
        ) = 0;    
};

inline KWfDataPackage::KWfDataPackage() {}
inline KWfDataPackage::~KWfDataPackage() {}


//
// The equivalent of IFabricCodePackageActivationContext
//

class KWfCodePackageActivationContext : public KObject<KWfCodePackageActivationContext>, public KShared<KWfCodePackageActivationContext>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KWfCodePackageActivationContext);

public:

    static
    NTSTATUS
    Create(
        __in KAllocator& Allocator,
        __out KWfCodePackageActivationContext::SPtr& Context,
        __in ULONG AllocationTag = KTL_TAG_WINFAB,
        __in_opt GUID const * IID = nullptr
        );    

    virtual LPCWSTR 
    get_ContextId(
        ) = 0;

    virtual LPCWSTR
    get_CodePackageName(
        ) = 0;
    
    virtual LPCWSTR 
    get_WorkDirectory(
        ) = 0;

    virtual LPCWSTR 
    get_LogDirectory(
        ) = 0;

    virtual NTSTATUS 
    GetCodePackage(
        __in_z LPCWSTR CodePackageName,
        __out KWfCodePackage::SPtr& CodePackage
        ) = 0;    

    virtual NTSTATUS 
    GetConfigurationPackage(
        __in_z LPCWSTR ConfigPackageName,
        __out KWfConfigurationPackage::SPtr& ConfigPackage
        ) = 0;    
    
    virtual NTSTATUS 
    GetDataPackage(
        __in_z LPCWSTR DataPackageName,
        __out KWfDataPackage::SPtr& DataPackage
        ) = 0;    
                
    //
    // BUGBUG This interface is incomplete. Add the following additional methods.
    //

    #if 0   

    virtual HRESULT STDMETHODCALLTYPE GetServiceManifestDescription( 
        /* [retval][out] */ IFabricServiceManifestDescriptionResult **serviceManifest) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE RegisterCodePackageChangeHandler( 
        /* [in] */ IFabricCodePackageChangeHandler *callback,
        /* [retval][out] */ LONGLONG *callbackHandle) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE UnregisterCodePackageChangeHandler( 
        /* [in] */ LONGLONG callbackHandle) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE RegisterConfigurationPackageChangeHandler( 
        /* [in] */ IFabricConfigurationPackageChangeHandler *callback,
        /* [retval][out] */ LONGLONG *callbackHandle) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE UnregisterConfigurationPackageChangeHandler( 
        /* [in] */ LONGLONG callbackHandle) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE RegisterDataPackageChangeHandler( 
        /* [in] */ IFabricDataPackageChangeHandler *callback,
        /* [retval][out] */ LONGLONG *callbackHandle) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE UnregisterDataPackageChangeHandler( 
        /* [in] */ LONGLONG callbackHandle) = 0;

    #endif
    
};

inline KWfCodePackageActivationContext::KWfCodePackageActivationContext() {}
inline KWfCodePackageActivationContext::~KWfCodePackageActivationContext() {}


