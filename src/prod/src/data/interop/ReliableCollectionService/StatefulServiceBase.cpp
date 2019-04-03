// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace TXRStatefulServiceBase;
using namespace FabricTypeHost;

StringLiteral const TraceComponent("ReliableCollectionService");
WStringLiteral const ReplicatorEndpointName(L"ReplicatorEndpoint");

IFabricRuntime *g_pFabricRuntime = nullptr;
static ReliableCollectionApis g_reliableCollectionApis;

// TODO: Make this thread-safe
//
// This method is used to explicitly release any resources used by ReliableCollectionService
// infrastructure. Any user of RCS must invoke this method when they are done using it
// to prevent any resource leaks.
extern "C" __declspec(dllexport) HRESULT ReliableCollectionService_Cleanup()
{
    if (g_pFabricRuntime != nullptr)
    {
        g_pFabricRuntime->Release();
        g_pFabricRuntime = nullptr;

        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

extern "C" __declspec(dllexport) HRESULT ReliableCollectionService_InitializeEx(
    LPCWSTR serviceTypeName, 
    ULONG port, 
    addPartitionContextCallback addCallback,
    removePartitionContextCallback removeCallback,
    changeRoleCallback changeCallback,
    abortCallback abortCallback,
    BOOL registerManifestEndpoints,
    BOOL skipUserPortEndpointRegistration,
    BOOL reportEndpointsOnlyOnPrimaryReplica)
{
    // Keep following for compat with existing code such as BlockStore for now
    UNREFERENCED_PARAMETER(skipUserPortEndpointRegistration);
    UNREFERENCED_PARAMETER(port);

    StatefulServiceBase::SetCallback(addCallback, removeCallback, changeCallback, abortCallback);
    
    Helpers::EnableTracing();

#ifdef DEBUG_KTL_CONTAINER
    // Include KTL traces in FabricTraces - debugging only
    RegisterKtlTraceCallback(Common::ServiceFabricKtlTraceCallback);
#endif

    HRESULT hr;

    Trace.WriteInfo(TraceComponent, "[ReliableCollectionService_Initialize] Calling ReliableCollectionRuntime_Initialize...");

    hr = FabricGetReliableCollectionApiTable(RELIABLECOLLECTION_API_VERSION, &g_reliableCollectionApis);
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "[ReliableCollectionService_Initialize] FabricGetReliableCollectionApiTable failed with hr={0}", hr);
        return hr;
    }    
    
    auto callback =
        [registerManifestEndpoints, 
        skipUserPortEndpointRegistration,
        reportEndpointsOnlyOnPrimaryReplica](FABRIC_PARTITION_ID partitionId, FABRIC_REPLICA_ID replicaId, ComponentRootSPtr const & root)
    {
        Trace.WriteInfo(TraceComponent, "[ReliableCollectionService_Initialize] Creating StatefulServiceBase...");

        ComPointer<StatefulServiceBase> service = make_com<StatefulServiceBase>(partitionId, replicaId, root, registerManifestEndpoints, reportEndpointsOnlyOnPrimaryReplica);
        ComPointer<StatefulServiceBase> serviceBase;
        serviceBase.SetAndAddRef(service.GetRawPointer());
        return serviceBase;
    };

    shared_ptr<Factory> factory = Factory::Create(callback);

    ComPointer<ComFactory> comFactory = make_com<ComFactory>(serviceTypeName, *factory.get());

    Trace.WriteInfo(TraceComponent, "[ReliableCollectionService_Initialize] Calling FabricCreateRuntime...");
    ComPointer<IFabricRuntime> fabricRuntime;
    hr = FabricCreateRuntime(IID_IFabricRuntime, fabricRuntime.VoidInitializationAddress());
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "[ReliableCollectionService_Initialize] FabricCreateRuntime failed with hr={0}", hr);
        return hr;
    }

    Trace.WriteInfo(TraceComponent, "[ReliableCollectionService_Initialize] Calling IFabricRuntime::RegisterStatefulServiceFactory...");
    hr = fabricRuntime->RegisterStatefulServiceFactory(
        serviceTypeName,
        comFactory.GetRawPointer());
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "[ReliableCollectionService_Initialize] IFabricRuntime::RegisterStatefulServiceFactory failed with hr={0}", hr);
        return hr;
    }

    g_pFabricRuntime = fabricRuntime.DetachNoRelease();
    Trace.WriteInfo(TraceComponent, "[ReliableCollectionService_Initialize] Succeeded.");

    return S_OK;
}

extern "C" __declspec(dllexport) HRESULT ReliableCollectionService_Initialize(
    LPCWSTR serviceTypeName, 
    ULONG port, 
    addPartitionContextCallback addCallback,
    removePartitionContextCallback removeCallback)
{
    // The default initialization is to only report user-port endpoint on all replicas.
    return ReliableCollectionService_InitializeEx(
        serviceTypeName, port, addCallback, removeCallback, nullptr, nullptr,
        TRUE,   // registerManifestEndpoints,
        FALSE,  // skipUserPortEndpointRegistration
        FALSE   // reportEndpointsOnlyOnPrimaryReplica
    );
}

TimeSpan const HttpServerOpenCloseTimeout = TimeSpan::FromMinutes(2);
ULONG const numberOfParallelHttpRequests = 100;

addPartitionContextCallback StatefulServiceBase::s_addPartitionContextCallbackFnptr;
removePartitionContextCallback StatefulServiceBase::s_removePartitionContextCallbackFnptr;
changeRoleCallback StatefulServiceBase::s_changeRoleCallbackFnPtr;
abortCallback StatefulServiceBase::s_abortCallbackFnPtr;

StatefulServiceBase::StatefulServiceBase(
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in ComponentRootSPtr const & root,
    __in BOOL registerManifestEndpoints,
    __in BOOL reportEndpointsOnlyOnPrimaryReplica)
    : instanceId_(SequenceNumber::GetNext())
    , root_(root)
    , replicaId_(replicaId)
    , partitionId_(partitionId)
    , role_(FABRIC_REPLICA_ROLE_UNKNOWN)
    , partition_()
    , primaryReplicator_()
    , registerManifestEndpoints_(registerManifestEndpoints)
    , reportEndpointsOnlyOnPrimaryReplica_(reportEndpointsOnlyOnPrimaryReplica)
    , endpoints_()
{ 
    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase.Ctor]");
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
    , role_(FABRIC_REPLICA_ROLE_UNKNOWN)
    , partition_()
    , primaryReplicator_()
    , registerManifestEndpoints_(false)
    , reportEndpointsOnlyOnPrimaryReplica_(false)
    , endpoints_()
{
    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase.Ctor]");
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

    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::BeginOpen] started");

    if (statefulServicePartition == NULL || callback == NULL || context == NULL) { return E_POINTER; }

    ComPointer<IFabricInternalStatefulServicePartition> internalPartition;
    partition_.SetAndAddRef((IFabricStatefulServicePartition2*)statefulServicePartition);

    IfFailReturn(statefulServicePartition->QueryInterface(IID_IFabricInternalStatefulServicePartition, internalPartition.VoidInitializationAddress()));

    // TODO leaking
    IFabricDataLossHandler* dataLossHandler = new TestComProxyDataLossHandler(TRUE);

    hr = g_reliableCollectionApis.GetTxnReplicator(
        ReplicaId,
        statefulServicePartition,
        dataLossHandler,
        nullptr,
        L"Config",
        L"ReplicatorConfig",
        L"",
        (void**)primaryReplicator_.InitializationAddress(),
        &txReplicator_);
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "[StatefulServiceBase::BeginOpen] GetTxnReplicator failed with hr = '{0}'", hr);
        return hr;
    }
    
    LONGLONG key;
    hr = GetPartitionLowKey(key);
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "[StatefulServiceBase::BeginOpen] GetPartitionLowKey failed with hr = '{0}'", hr);
        return hr;
    }

    //TODO lifetime of txReplicator

    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::BeginOpen] Issuing AddPartitionCallback(Key='{0}')", key);

    s_addPartitionContextCallbackFnptr(key, txReplicator_, statefulServicePartition, PartitionId, ReplicaId);

    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::BeginOpen] AddPartitionCallback Key=('{0}') complete.", key);

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    hr = operation->Initialize(root_, callback);
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "[StatefulServiceBase::EndOpen] ComCompletedAsyncOperationContext::Initialize failed with hr = '{0}'", hr);
        return hr;
    }

    hr = ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(move(operation), context);
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "[StatefulServiceBase::EndOpen] ComAsyncOperationContext::StartAndDetach failed with hr = '{0}'", hr);
        return hr;
    }

    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::BeginOpen] succeeded");

    return S_OK;
}

HRESULT StatefulServiceBase::EndOpen(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [out][retval] */ IFabricReplicator **replicationEngine)
{
    if (context == NULL || replicationEngine == NULL) { return E_POINTER; }

    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::EndOpen] started");

    HRESULT hr;
    hr = ComCompletedAsyncOperationContext::End(context);
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "[StatefulServiceBase::EndOpen] ComCompletedAsyncOperationContext::End failed with hr = '{0}'", hr);
        return hr;
    }

    *replicationEngine = primaryReplicator_.DetachNoRelease();

    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::EndOpen] succeeded");

    return S_OK;
}

HRESULT StatefulServiceBase::BeginChangeRole( 
    /* [in] */ FABRIC_REPLICA_ROLE newRole,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (callback == NULL || context == NULL) { return E_POINTER; }

    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::BeginChangeRole] started. NewRole = {0} ", newRole);

    role_ = newRole;

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr;
    IfFailReturn(operation->Initialize(root_, callback));

    hr = ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(move(operation), context);


    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::BeginChangeRole] succeeded.");

    return hr;
}

HRESULT StatefulServiceBase::EndChangeRole( 
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][out] */ IFabricStringResult **serviceEndpoints)
{
    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::EndChangeRole] started");

    if (context == NULL || serviceEndpoints == NULL) { return E_POINTER; }

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);
    if (FAILED(hr)) return hr;

    // Initialize the endpoints to be reported
    endpoints_ = wstring(L"");
    hr = this->SetupEndpointAddresses();
    if (FAILED(hr)) { return hr; }
    
    if (endpoints_.empty())
    {
        Trace.WriteInfo(
        TraceComponent,
        "StatefulServiceBase.EndChangeRole: No endpoints available for reporting.");
        
        ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult,IFabricStringResult>(wstring(L""));
        *serviceEndpoints = stringResult.DetachNoRelease();
    }
    else
    {
        ComPointer<IFabricStringResult> stringResult = make_com<ComStringResult,IFabricStringResult>(endpoints_);
        *serviceEndpoints = stringResult.DetachNoRelease();
    }

    // Deliver the change role callback if one has been specified
    if (s_changeRoleCallbackFnPtr != nullptr)
    {
        LONGLONG key;
        hr = GetPartitionLowKey(key);
        if (FAILED(hr))
        {
            Trace.WriteError(TraceComponent, "[StatefulServiceBase::EndChangeRole] GetPartitionLowKey failed with hr = '{0}'", hr);
            return hr;
        }
                     
        s_changeRoleCallbackFnPtr(key, (int32_t)role_);
    }
    
    return S_OK;
}

// Gathers the list of the service endpoints to be reported per specifid policies.
HRESULT StatefulServiceBase::SetupEndpointAddresses()
{
    ComPointer<IFabricRuntime> fabricRuntimeCPtr;
    ComPointer<IFabricCodePackageActivationContext> activationContextCPtr;

    HRESULT hrContext = ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContextCPtr.VoidInitializationAddress());
    if (FAILED(hrContext))
    {
        Trace.WriteError(
        TraceComponent,
        "StatefulServiceBase.SetupEndpointAddresses: Failed to get CodePackageActivationContext. Error:{0}",
        hrContext);

        return hrContext;
    }

    // By default, we will report (one or more) endpoints.
    bool fReportEndpoints = true;

    // Are we expected to report endpoints only on primary replica?
    if (RegisterEndpointsOnlyOnPrimaryReplica)
    {
        if (Role != FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_PRIMARY)
        {
            fReportEndpoints = false;

            Trace.WriteInfo(
            TraceComponent,
            "StatefulServiceBase.SetupEndpointAddresses: Skipping endpoint registration as it can only happen on a Primary replica and not the current role {0}.",
            Role);
        }    
    }

    HRESULT hrRetVal = S_OK;

    if (fReportEndpoints)
    {
        EndpointsDescription endpointsDescription;

        // Should we register endpoints from the manifest?
        if (RegisterManifestEndpoints)
        {
            const FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST * endpoints = activationContextCPtr->get_ServiceEndpointResources();
            ULONG manifestEndpointCount = 0;

            if (!(endpoints == NULL ||
                endpoints->Count == 0 ||
                endpoints->Items == NULL))
            {
                manifestEndpointCount = endpoints->Count;
            }
	
            Trace.WriteInfo(
            TraceComponent,
            "StatefulServiceBase.SetupEndpointAddresses: Reportable endpoint count is {0}.",
            manifestEndpointCount);

            for (ULONG i = 0; i < manifestEndpointCount; ++i)
            {
                const auto & endpoint = endpoints->Items[i];
                
                wstring uriScheme;
                wstring pathSuffix;

                if (endpoint.Reserved == nullptr)
                {
                    Trace.WriteError(
                    TraceComponent,
                    "StatefulServiceBase.SetupEndpointAddresses: Failed to get EndpointDescriptionEx1");

                    return E_INVALIDARG;
                }

                auto endpointEx1Ptr = (FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_EX1*)endpoint.Reserved;
                uriScheme = endpointEx1Ptr->UriScheme;
                pathSuffix = endpointEx1Ptr->PathSuffix;

                if (endpointEx1Ptr->Reserved == nullptr)
                {
                    Trace.WriteError(
                    TraceComponent,
                    "StatefulServiceBase.SetupEndpointAddresses: Failed to get EndpointDescriptionEx2");
                    return E_INVALIDARG;
                }

                auto endpointEx2Ptr = (FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_EX2*)endpointEx1Ptr->Reserved;		
                wstring ipAddressOrFqdn = endpointEx2Ptr->IpAddressOrFqdn;
                
                if (ipAddressOrFqdn.empty())
                {
                    Trace.WriteError(
                    TraceComponent,
                    "StatefulServiceBase.SetupEndpointAddresses: IpAddressOrFqdn is empty for endpoint resource={0}",
                    endpoint.Name);

                    ASSERT_IF(true, "IpAddressOrFqdn is empty for endpoint resource={0}", endpoint.Name);
                }
                
                if (!StringUtility::AreEqualCaseInsensitive(endpoint.Name, L"ReplicatorEndpoint"))
                {
                    wstring listenerAddress;
                    if (!uriScheme.empty())
                    {
                        listenerAddress = wformatString("{0}://{1}:{2}/{3}", uriScheme, ipAddressOrFqdn, endpoint.Port, pathSuffix);
                    }
                    else
                    {
                        listenerAddress = wformatString("{0}:{1}", ipAddressOrFqdn, endpoint.Port);
                    }

                    Trace.WriteInfo(
                        TraceComponent,
                        "StatefulServiceBase.SetupEndpointAddresses: EndpointName={0}, ListenerAddress={1}.", 
                        endpoint.Name, 
                        listenerAddress);

                    endpointsDescription.Endpoints.insert(make_pair(endpoint.Name, listenerAddress));
                }
                else
                {
                    // Don't publish replicator end point as we don't want:
                    // 1. ReplicatorEndpoint becoming the default user end point (if it is the first one)
                    // 2. ReplicatorEndpoint get registered with reverse proxy and reachable
                    Trace.WriteInfo(
                        TraceComponent,
                        "StatefulServiceBase.SetupEndpointAddresses: Skipping reporting ReplicatorEndpoint.");
                }
            }
        }

        if (!endpointsDescription.Endpoints.empty())
        {
            auto error = JsonHelper::Serialize<EndpointsDescription>(endpointsDescription, endpoints_);
            hrRetVal = error.ToHResult();
        }
    }

    return hrRetVal;
}

HRESULT StatefulServiceBase::BeginClose( 
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::BeginClose] started");

    if (context == NULL) { return E_POINTER; }

    LONGLONG key;
    HRESULT hr = GetPartitionLowKey(key);
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "[StatefulServiceBase::BeginClose] GetPartitionLowKey failed with hr = '{0}'", hr);
        return hr;
    }

    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::BeginClose] Issuing RemovePartitionCallback(Key='{0}')", key);

    s_removePartitionContextCallbackFnptr(key);

    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::BeginClose] Issuing RemovePartitionCallback(Key='{0}')", key);

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    IfFailReturn(operation->Initialize(root_, callback));

    IfFailReturn(ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(move(operation), context));

    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::BeginClose] succeeded");

    return S_OK;
}

HRESULT StatefulServiceBase::EndClose( 
    /* [in] */ IFabricAsyncOperationContext *context)
{
    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::EndClose]");

    if (context == NULL) { return E_POINTER; }

    HRESULT hr;

    IfFailReturn(ComCompletedAsyncOperationContext::End(context));

    Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::EndClose] succeeded");

    return S_OK;
}

void StatefulServiceBase::Abort()
{
    if (s_abortCallbackFnPtr != nullptr)
    {
        LONGLONG key;
        HRESULT hr = GetPartitionLowKey(key);
        if (FAILED(hr))
        {
            Trace.WriteError(TraceComponent, "[StatefulServiceBase::EndChangeRole] GetPartitionLowKey failed with hr = '{0}'", hr);
        }
        else
        {
            key = 0;
        }
        //call abort even if it fails to get the key.
        s_abortCallbackFnPtr(key);
    }
}

HRESULT StatefulServiceBase::GetPartitionLowKey(LONGLONG &lowKey)
{
    const FABRIC_SERVICE_PARTITION_INFORMATION* servicePartitionInfo;

    HRESULT hr;

    hr = partition_->GetPartitionInfo(&servicePartitionInfo);
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "[StatefulServiceBase::GetPartitionLowKey] GetPartitionInfo failed with hr = '{0}'", hr);
        return hr;
    }

    switch (servicePartitionInfo->Kind)
    {
        case FABRIC_SERVICE_PARTITION_KIND::FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE:
        {
            Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::GetPartitionLowKey] Kind = Int64Range");
            lowKey = ((FABRIC_INT64_RANGE_PARTITION_INFORMATION *)(servicePartitionInfo->Value))->LowKey;
            Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::GetPartitionLowKey] LowKey = '{0}'", lowKey);

            return S_OK;
        }

        case FABRIC_SERVICE_PARTITION_KIND::FABRIC_SERVICE_PARTITION_KIND_NAMED:
        {
            Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::GetPartitionLowKey] Kind = NamedPartition");
            wstring name(((FABRIC_NAMED_PARTITION_INFORMATION *)(servicePartitionInfo->Value))->Name);
            Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::GetPartitionLowKey] Name = '{0}'", name);

            if (!Common::TryParseInt64(name, lowKey))
            {
                Trace.WriteError(
                    TraceComponent,
                    "[StatefulServiceBase::GetPartitionLowKey] NamedPartition = '{0}' is not supported as it needs to be an integer value",
                    name);
                return E_INVALIDARG;
            }

            Trace.WriteInfo(TraceComponent, "[StatefulServiceBase::GetPartitionLowKey] LowKey = '{0}'", lowKey);

            return S_OK;
        }

        default:
        {
            Trace.WriteError(TraceComponent, "[StatefulServiceBase::GetPartitionLowKey] Kind = {0} is unexpected", servicePartitionInfo->Kind);
            return E_INVALIDARG;
        }
    }
}
