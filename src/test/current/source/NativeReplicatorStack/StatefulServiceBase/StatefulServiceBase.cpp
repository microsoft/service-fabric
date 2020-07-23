// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace TXRStatefulServiceBase;

StringLiteral const TraceComponent("StatefulServiceBase");
TimeSpan const HttpServerOpenCloseTimeout = TimeSpan::FromMinutes(2);
ULONG const numberOfParallelHttpRequests = 100;

ComPointer<IFabricReplicatorSettingsResult> LoadReplicatorSettings();
ComPointer<IFabricTransactionalReplicatorSettingsResult> LoadTransactionalReplicatorSettings();

StatefulServiceBase::StatefulServiceBase(
    __in ULONG httpListeningPort,
    __in FABRIC_PARTITION_ID partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in ComponentRootSPtr const & root)
    : instanceId_(SequenceNumber::GetNext())
    , root_(root)
    , replicaId_(replicaId)
    , partitionId_(partitionId)
#if defined(PLATFORM_UNIX)
    , httpListenAddress_(wformatString("http://0.0.0.0:{0}/{1}/{2}/{3}", httpListeningPort, Guid(partitionId), replicaId, instanceId_))
#else
    , httpListenAddress_(wformatString("http://+:{0}/{1}/{2}/{3}", httpListeningPort, Guid(partitionId), replicaId, instanceId_))
#endif
    , changeRoleEndpoint_(wformatString("http://{0}:{1}/{2}/{3}/{4}", Helpers::GetFabricNodeIpAddressOrFqdn(), Helpers::GetHttpPortFromConfigurationPackage(), Guid(partitionId), replicaId, instanceId_))
    , role_(FABRIC_REPLICA_ROLE_UNKNOWN)
    , httpServerSPtr_()
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
#if defined(PLATFORM_UNIX)
    , httpListenAddress_(wformatString("http://0.0.0.0:{0}/{1}/{2}/{3}", Helpers::GetHttpPortFromConfigurationPackage(), Guid(partitionId), replicaId, instanceId_))
#else
    , httpListenAddress_(wformatString("http://+:{0}/{1}/{2}/{3}", Helpers::GetHttpPortFromConfigurationPackage(), Guid(partitionId), replicaId, instanceId_))
#endif
    , changeRoleEndpoint_(wformatString("http://{0}:{1}/{2}/{3}/{4}", Helpers::GetFabricNodeIpAddressOrFqdn(), Helpers::GetHttpPortFromConfigurationPackage(), Guid(partitionId), replicaId, instanceId_))
    , role_(FABRIC_REPLICA_ROLE_UNKNOWN)
    , httpServerSPtr_()
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

HttpServer::IHttpServerSPtr CreateAndOpenHttpServer(
    __in ComponentRootSPtr const & root, 
    __in wstring const & listenAddress,
    __in ULONG numberOfParallelRequests)
{
    AutoResetEvent openEvent;

    shared_ptr<HttpServer::HttpServerImpl> httpServer = make_shared<HttpServer::HttpServerImpl>(
        *root,
        listenAddress,
        numberOfParallelRequests,
        Transport::SecurityProvider::None);

    httpServer->BeginOpen(
        HttpServerOpenCloseTimeout,
    [&openEvent, &httpServer](AsyncOperationSPtr const &operation)
        {
            ErrorCode e = httpServer->EndOpen(operation);
            if (e.IsSuccess())
            {
                Trace.WriteInfo(
                    TraceComponent,
                    "HttpServer Open successful");
            }
            else
            {
                Trace.WriteError(
                    TraceComponent,
                    "HttpServer Open failed - {0}", 
                    e);
            }

            openEvent.Set();
        });

    openEvent.WaitOne();

    return httpServer;
}

void CloseHttpServer(__in HttpServer::IHttpServerSPtr & httpServer)
{
    if (!httpServer)
    {
        return;
    }

    HttpServer::HttpServerImpl & httpServerImpl = (HttpServer::HttpServerImpl&)(*httpServer);
    AutoResetEvent closeEvent;

    httpServerImpl.BeginClose(
        HttpServerOpenCloseTimeout,
        [&closeEvent, &httpServerImpl, &httpServer](AsyncOperationSPtr const &operation)
        {
            ErrorCode e = httpServerImpl.EndClose(operation);

            if (e.IsSuccess())
            {
                Trace.WriteInfo(
                    TraceComponent,
                    "HttpServer close successful");

                httpServer.reset();
            }
            else
            {
                Trace.WriteError(
                    TraceComponent,
                    "HttpServer Close failed - {0}", 
                    e);
            }

            closeEvent.Set();
        },
        nullptr);

    closeEvent.WaitOne();
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

    hr = statefulServicePartition->QueryInterface(IID_IFabricInternalStatefulServicePartition, internalPartition.VoidInitializationAddress());
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    HANDLE txReplicator = nullptr;
    HANDLE ktlSystem = nullptr;

    hr = internalPartition->GetKtlSystem(&ktlSystem);
    ktlSystem_ = (KtlSystem*)ktlSystem;

    ComPointer<IFabricStateProvider2Factory> spFactory = GetStateProviderFactory();

    ComPointer<IFabricDataLossHandler> dataLossHandler = GetDataLossHandler();

    auto v1ReplicatorSettings = LoadReplicatorSettings();
    auto nativeTxnReplicatorSettings = LoadTransactionalReplicatorSettings();

#if defined(PLATFORM_UNIX)
    KTLLOGGER_SHARED_LOG_SETTINGS sharedLogSettings = {};
    std::shared_ptr<TxnReplicator::KtlLoggerSharedLogSettings> ptr;
    auto defaultSettings = ptr->GetKeyValueStoreReplicaDefaultSettings(Helpers::GetWorkingDirectory(), L"", L"statefulservicebase.log", Common::Guid::Empty());
    auto castedLogSettings = dynamic_cast<TxnReplicator::KtlLoggerSharedLogSettings*>(defaultSettings.get());

    Common::ScopedHeap heap;
    castedLogSettings->ToPublicApi(heap, sharedLogSettings);

    hr = internalPartition->CreateTransactionalReplicator(
        spFactory.GetRawPointer(),
        dataLossHandler.GetRawPointer(),
        v1ReplicatorSettings->get_ReplicatorSettings(),
        nativeTxnReplicatorSettings->get_TransactionalReplicatorSettings(),
        &sharedLogSettings,
        primaryReplicator_.InitializationAddress(),
        &txReplicator);
#else
    hr = internalPartition->CreateTransactionalReplicator(
        spFactory.GetRawPointer(),
        dataLossHandler.GetRawPointer(),
        v1ReplicatorSettings->get_ReplicatorSettings(),
        nativeTxnReplicatorSettings->get_TransactionalReplicatorSettings(),
        nullptr,
        primaryReplicator_.InitializationAddress(),
        &txReplicator);
#endif

    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    txReplicatorSPtr_.Attach((TxnReplicator::ITransactionalReplicator *)txReplicator);

    Trace.WriteInfo(
        TraceComponent,
        "Opening HttpServer");

    httpServerSPtr_ = CreateAndOpenHttpServer(
        root_,
        httpListenAddress_,
        numberOfParallelHttpRequests);

    HttpRequestProcessCallback postRequestCallback = 
        [this](Common::ByteBufferUPtr && body,Common::ByteBufferUPtr & responseBody)
    {
        return OnHttpPostRequest(move(body), responseBody);
    };

    TestClientRequestHandlerSPtr handlerSPtr = TestClientRequestHandler::Create(
        postRequestCallback);

    ErrorCode errorCode = httpServerSPtr_->RegisterHandler(L"/", handlerSPtr);

    if (!errorCode.IsSuccess())
    {
        Trace.WriteError(
            TraceComponent,
            "Failed to register request handler with error {0}",
            errorCode);
        
        hr = errorCode.ToHResult();
    }

    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    hr = operation->Initialize(root_, callback);

    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(move(operation), context);
}

HRESULT StatefulServiceBase::EndOpen(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [out][retval] */ IFabricReplicator **replicationEngine)
{
    if (context == NULL || replicationEngine == NULL) { return E_POINTER; }

    HRESULT hr = ComCompletedAsyncOperationContext::End(context);

    if (FAILED(hr)) 
    { 
        return hr; 
    }

    *replicationEngine = primaryReplicator_.DetachNoRelease();
    return hr;
}

HRESULT StatefulServiceBase::BeginChangeRole( 
    /* [in] */ FABRIC_REPLICA_ROLE newRole,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    if (callback == NULL || context == NULL) { return E_POINTER; }

    OnChangeRole(newRole, callback, context);

    role_ = newRole;

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = E_FAIL;
    hr = operation->Initialize(root_, callback);

    return ComAsyncOperationContext::StartAndDetach<ComCompletedAsyncOperationContext>(move(operation), context);
}

void StatefulServiceBase::OnChangeRole(
    /* [in] */ FABRIC_REPLICA_ROLE newRole,
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    UNREFERENCED_PARAMETER(newRole);
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(context);
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

    CloseHttpServer(httpServerSPtr_);

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = operation->Initialize(root_, callback);
    if (FAILED(hr)) return hr;

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
    if (httpServerSPtr_)
    {
        HttpServer::HttpServerImpl & httpServerImpl = (HttpServer::HttpServerImpl&)(*httpServerSPtr_);
        httpServerImpl.Abort();
    }
}

ComPointer<IFabricReplicatorSettingsResult> LoadReplicatorSettings()
{
    ComPointer<IFabricReplicatorSettingsResult> replicatorSettings;
    ComPointer<IFabricCodePackageActivationContext> activationContext;

    ASSERT_IFNOT(
        ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress()) == S_OK,
        "Failed to get activation context in LoadReplicatorSettings");

    ComPointer<IFabricStringResult> configPackageName = make_com<ComStringResult, IFabricStringResult>(L"Config");
    ComPointer<IFabricStringResult> sectionName = make_com<ComStringResult, IFabricStringResult>(L"ReplicatorConfig");

    auto hr = ::FabricLoadReplicatorSettings(
        activationContext.GetRawPointer(),
        configPackageName->get_String(),
        sectionName->get_String(),
        replicatorSettings.InitializationAddress());

    ASSERT_IFNOT(hr == S_OK, "Failed to load replicator v1ReplicatorSettings due to error {0}", hr);

    return replicatorSettings;
}

ComPointer<IFabricTransactionalReplicatorSettingsResult> LoadTransactionalReplicatorSettings()
{
    ComPointer<IFabricTransactionalReplicatorSettingsResult> txnReplicatorSettings;
    ComPointer<IFabricCodePackageActivationContext> activationContext;

    ASSERT_IFNOT(
        ::FabricGetActivationContext(IID_IFabricCodePackageActivationContext, activationContext.VoidInitializationAddress()) == S_OK,
        "Failed to get activation context in LoadTransactionalReplicatorSettings");

    ComPointer<IFabricStringResult> configPackageName = make_com<ComStringResult, IFabricStringResult>(L"Config");
    ComPointer<IFabricStringResult> sectionName = make_com<ComStringResult, IFabricStringResult>(L"ReplicatorConfig");

    auto hr = ::FabricLoadTransactionalReplicatorSettings(
        activationContext.GetRawPointer(),
        configPackageName->get_String(),
        sectionName->get_String(),
        txnReplicatorSettings.InitializationAddress());

    ASSERT_IFNOT(hr == S_OK, "Failed to load replicator v1ReplicatorSettings due to error {0}", hr);

    return txnReplicatorSettings;
}
