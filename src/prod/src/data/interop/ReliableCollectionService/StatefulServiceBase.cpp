// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <mutex>
#include <condition_variable>

using namespace Common;
using namespace std;
using namespace TXRStatefulServiceBase;

bool ready = false;
mutex m;
condition_variable cv;
ComPointer<IFabricRuntime> fabricRuntime;

extern "C" __declspec(dllexport) HRESULT ReliableCollectionService_Initialize(
    LPCWSTR serviceTypeName, 
    ULONG port, 
    addPartitionContextCallback addCallback,
    removePartitionContextCallback removeCallback)
{
    StatefulServiceBase::SetCallback(addCallback, removeCallback);

    Helpers::EnableTracing();

    HRESULT hr;

    IfFailReturn(ReliableCollectionRuntime_Initialize(RELIABLECOLLECTION_API_VERSION));    

    auto callback =
        [](ULONG port, FABRIC_PARTITION_ID partitionId, FABRIC_REPLICA_ID replicaId, ComponentRootSPtr const & root)
    {
        ComPointer<StatefulServiceBase> service = make_com<StatefulServiceBase>(port, partitionId, replicaId, root);
        ComPointer<StatefulServiceBase> serviceBase;
        serviceBase.SetAndAddRef(service.GetRawPointer());
        return serviceBase;
    };

    shared_ptr<Factory> factory = Factory::Create(callback, port);

    ComPointer<ComFactory> comFactory = make_com<ComFactory>(serviceTypeName, *factory.get());

    IfFailReturn(FabricCreateRuntime(IID_IFabricRuntime, fabricRuntime.VoidInitializationAddress()));

    IfFailReturn(fabricRuntime->RegisterStatefulServiceFactory(
        serviceTypeName,
        comFactory.GetRawPointer()));

    unique_lock<mutex> lck(m);
    cv.wait(lck, []() { return ready; });

    return S_OK;
}

StringLiteral const TraceComponent("StatefulServiceBase");
TimeSpan const HttpServerOpenCloseTimeout = TimeSpan::FromMinutes(2);
ULONG const numberOfParallelHttpRequests = 100;

HRESULT LoadReplicatorSettings(ComPointer<IFabricReplicatorSettingsResult> &replicatorSettings);

addPartitionContextCallback StatefulServiceBase::s_addPartitionContextCallbackFnptr;
removePartitionContextCallback StatefulServiceBase::s_removePartitionContextCallbackFnptr;

StatefulServiceBase::StatefulServiceBase(
    __in ULONG httpListeningPort,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in ComponentRootSPtr const & root)
    : instanceId_(SequenceNumber::GetNext())
    , root_(root)
    , replicaId_(replicaId)
    , partitionId_(partitionId)
    , httpListenAddress_(wformatString("http://+:{0}/{1}/{2}/{3}", httpListeningPort, Guid(partitionId), replicaId, instanceId_))
    , changeRoleEndpoint_(wformatString("http://{0}:{1}", Helpers::GetFabricNodeIpAddressOrFqdn(), httpListeningPort))
    , role_(FABRIC_REPLICA_ROLE_UNKNOWN)
    , partition_()
    , primaryReplicator_()
{ 
    Trace.WriteInfo(
        TraceComponent,
        "StatefulServiceBase.Ctor. HttpListenAddress: {0}. ServiceEndpoint: {1}",
        httpListenAddress_,
        changeRoleEndpoint_);
}

// Uses default http endpoint resource. Name is a const defined in Helpers::ServiceHttpEndpointResourceName in Helpers.cpp
StatefulServiceBase::StatefulServiceBase(
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in Common::ComponentRootSPtr const & root)
    : instanceId_(SequenceNumber::GetNext())
    , root_(root)
    , replicaId_(replicaId)
    , partitionId_(partitionId)
    , httpListenAddress_(wformatString("http://+:{0}/{1}/{2}/{3}", Helpers::GetHttpPortFromConfigurationPackage(), Guid(partitionId), replicaId, instanceId_))
    , changeRoleEndpoint_(wformatString("http://{0}:{1}/{2}/{3}/{4}", Helpers::GetFabricNodeIpAddressOrFqdn(), Helpers::GetHttpPortFromConfigurationPackage(), Guid(partitionId), replicaId, instanceId_))
    , role_(FABRIC_REPLICA_ROLE_UNKNOWN)
    , partition_()
    , primaryReplicator_()
{
    Trace.WriteInfo(
        TraceComponent,
        "StatefulServiceBase.Ctor. HttpListenAddress: {0}. ServiceEndpoint: {1}",
        httpListenAddress_,
        changeRoleEndpoint_);
}

StatefulServiceBase::~StatefulServiceBase()
{
}

HRESULT StatefulServiceBase::BeginOpen( 
    /* [in] */ FABRIC_REPLICA_OPEN_MODE openMode,
    /* [in] */ IFabricStatefulServicePartition *statefulServicePartition,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(openMode);
    HRESULT hr = E_FAIL;

    if (statefulServicePartition == NULL || callback == NULL || context == NULL) { return E_POINTER; }

    ComPointer<IFabricInternalStatefulServicePartition> internalPartition;
    partition_.SetAndAddRef((IFabricStatefulServicePartition2*)statefulServicePartition);

    IfFailReturn(statefulServicePartition->QueryInterface(IID_IFabricInternalStatefulServicePartition, internalPartition.VoidInitializationAddress()));

    // TODO leaking
    IFabricDataLossHandler* dataLossHandler = new TestComProxyDataLossHandler(TRUE);

    ComPointer<IFabricReplicatorSettingsResult> settings;
    IfFailReturn(LoadReplicatorSettings(settings));

#if defined(PLATFORM_UNIX)
    KTLLOGGER_SHARED_LOG_SETTINGS sharedLogSettings = {};
    std::shared_ptr<TxnReplicator::KtlLoggerSharedLogSettings> ptr;

    auto defaultSettings = ptr->GetKeyValueStoreReplicaDefaultSettings(Helpers::GetWorkingDirectory(), L"", L"statefulservicebase.log", Common::Guid::Empty());

    auto castedLogSettings = dynamic_cast<TxnReplicator::KtlLoggerSharedLogSettings*>(defaultSettings.get());

    Common::ScopedHeap heap;

    castedLogSettings->ToPublicApi(heap, sharedLogSettings);

    IfFailReturn(GetTxnReplicator(
        statefulServicePartition,
        dataLossHandler,
        settings->get_ReplicatorSettings(),
        nullptr,
        &sharedLogSettings,
        (void**)primaryReplicator_.InitializationAddress(),
        &txReplicator_));
#else

    IfFailReturn(GetTxnReplicator(
        statefulServicePartition,
        dataLossHandler,
        settings->get_ReplicatorSettings(),
        nullptr,
        nullptr,
        (void**)primaryReplicator_.InitializationAddress(),
        &txReplicator_));
#endif
    
    LONGLONG key = GetPartitionLowKey();

    //TODO lifetime of txReplicator
    s_addPartitionContextCallbackFnptr(key, txReplicator_, statefulServicePartition, PartitionId, ReplicaId);

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    IfFailReturn(operation->Initialize(root_, callback));

    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(move(operation), context);
}

HRESULT StatefulServiceBase::EndOpen(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [out][retval] */ IFabricReplicator **replicationEngine)
{
    if (context == NULL || replicationEngine == NULL) { return E_POINTER; }

    HRESULT hr;
    IfFailReturn(ComCompletedAsyncOperationContext::End(context));

    *replicationEngine = primaryReplicator_.DetachNoRelease();
    return S_OK;
}

HRESULT StatefulServiceBase::BeginChangeRole( 
    /* [in] */ FABRIC_REPLICA_ROLE newRole,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (callback == NULL || context == NULL) { return E_POINTER; }

    role_ = newRole;

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr;
    IfFailReturn(operation->Initialize(root_, callback));

    hr = ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(move(operation), context);

    if (role_ == FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_PRIMARY && !ready)
    {
        lock_guard<mutex> lck(m);
        ready = true;
        cv.notify_one();
    }

    return hr;
}

HRESULT StatefulServiceBase::EndChangeRole( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceEndpoint)
{
    if (context == NULL || serviceEndpoint == NULL) { return E_POINTER; }

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    if (FAILED(hr)) return hr;

    ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult,IFabricStringResult>(changeRoleEndpoint_);
    *serviceEndpoint = stringResult.DetachNoRelease();
    return S_OK;
}

HRESULT StatefulServiceBase::BeginClose( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (context == NULL) { return E_POINTER; }

    LONGLONG key = GetPartitionLowKey();
    s_removePartitionContextCallbackFnptr(key);

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr;
    IfFailReturn(operation->Initialize(root_, callback));

    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(move(operation), context);
}

HRESULT StatefulServiceBase::EndClose( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    if (context == NULL) { return E_POINTER; }

    return ComCompletedAsyncOperationContext::End(context);
}

void StatefulServiceBase::Abort()
{
}

HRESULT LoadReplicatorSettings(ComPointer<IFabricReplicatorSettingsResult> &replicatorSettings)
{
    ComPointer<IFabricCodePackageActivationContext> activationContext;

    HRESULT hr;
    IfFailReturn(::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress()));

    ComPointer<IFabricStringResult> configPackageName = make_com<ComStringResult, IFabricStringResult>(L"Config");
    ComPointer<IFabricStringResult> sectionName = make_com<ComStringResult, IFabricStringResult>(L"ReplicatorConfig");

    IfFailReturn(::FabricLoadReplicatorSettings(
        activationContext.GetRawPointer(),
        configPackageName->get_String(),
        sectionName->get_String(),
        replicatorSettings.InitializationAddress()));

    return S_OK;
}

LONGLONG StatefulServiceBase::GetPartitionLowKey()
{
    const FABRIC_SERVICE_PARTITION_INFORMATION* servicePartitionInfo;
    partition_->GetPartitionInfo(&servicePartitionInfo);
    switch (servicePartitionInfo->Kind)
    {
    case FABRIC_SERVICE_PARTITION_KIND::FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
        return ((FABRIC_INT64_RANGE_PARTITION_INFORMATION *)(servicePartitionInfo->Value))->LowKey;
    default:
        Assert::CodingError("Unknown FABRIC_SERVICE_PARTITION_KIND {0}", (LONG)(servicePartitionInfo->Kind));
    }
}
