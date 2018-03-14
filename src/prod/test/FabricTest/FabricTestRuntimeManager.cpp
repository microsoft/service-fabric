// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace FabricTest;
using namespace Federation;
using namespace std;
using namespace Transport;
using namespace Common;
using namespace ServiceModel;
using namespace Fabric;
using namespace TestCommon;
using namespace FederationTestCommon;
using namespace Reliability;
using namespace Naming;
using namespace Hosting2;

const StringLiteral TraceSource("FabricTest.RuntimeManager"); 

const int FabricTestRuntimeManager::MaxRetryCount = 5;

FabricTestRuntimeManager::FabricTestRuntimeManager(
    NodeId nodeId, 
    wstring const & runtimeServiceAddress,
    wstring const& namingServiceListenAddress,
    bool retryCreateHost,
    SecuritySettings const & clientSecuritySettings)
    : nodeId_(nodeId),
    runtimeServiceAddress_(runtimeServiceAddress),
    namingServiceListenAddress_(namingServiceListenAddress),
    clientFactory_(),
    statelessFabricRuntime_(),
    statefulFabricRuntime_(),
    statelessHost_(),
    statefulHost_(),
    serviceFactory_(),
    serviceGroupFactory_(),
    statelessServiceTypeMap_(),
    statefulServiceTypeMap_(),
    retryCreateHost_(retryCreateHost)
{
    vector<wstring> gatewayAddresses;
    gatewayAddresses.push_back(namingServiceListenAddress_);
    auto error = Client::ClientFactory::CreateClientFactory(move(gatewayAddresses), clientFactory_);
    TestSession::FailTestIfNot(error.IsSuccess(), "CreateClientFactory failed: {0}", error);

    if (clientSecuritySettings.SecurityProvider() != SecurityProvider::None)
    {
        IClientSettingsPtr settings;
        error = clientFactory_->CreateSettingsClient(settings);

        TestSession::FailTestIfNot(error.IsSuccess(), "CreateSettingsClient failed: {0}", error);

        error = settings->SetSecurity(SecuritySettings(clientSecuritySettings));

        TestSession::FailTestIfNot(error.IsSuccess(), "SetSecurity failed: {0}", error);
    }

    serviceFactory_ = make_shared<TestServiceFactory>(nodeId_, clientFactory_);
    serviceGroupFactory_ = make_shared<SGServiceFactory>(nodeId_); 
}

FabricTestRuntimeManager::~FabricTestRuntimeManager()
{
}

void FabricTestRuntimeManager::CreateAllRuntimes()
{
    vector<wstring> statelessServiceTypes;
    vector<wstring> statefulServiceTypes;
    AddStatelessFabricRuntime(statelessServiceTypes);
    AddStatefulFabricRuntime(statefulServiceTypes);
}

void FabricTestRuntimeManager::AddStatelessHost()
{    
    this->PerformRetryableHostOperation(
        L"StatelessHost.Open",
        [&, this] (TimeSpan timeout, Common::AsyncCallback const& callback)
    {
        auto error = Hosting2::NonActivatedApplicationHost::Create(
            nullptr, // KtlSystemBase
            runtimeServiceAddress_, 
            this->statelessHost_);
        TestSession::FailTestIfNot(error.IsSuccess(), "NonActivatedApplicationHost::Create failed with error {0}", error);
        this->statelessHost_->BeginOpen(timeout, callback);
    },
        [&, this] (Common::AsyncOperationSPtr const& operation) -> ErrorCode
    {
        return this->statelessHost_->EndOpen(operation);
    });
}

void FabricTestRuntimeManager::AddStatefulHost()
{ 
    this->PerformRetryableHostOperation(
        L"StatefulHost.Open",
        [&, this] (TimeSpan timeout, Common::AsyncCallback const& callback)
    {
        auto error = Hosting2::NonActivatedApplicationHost::Create(
            nullptr, // KtlSystemBase
            runtimeServiceAddress_, 
            this->statefulHost_);
        TestSession::FailTestIfNot(error.IsSuccess(), "NonActivatedApplicationHost::Create failed with error {0}", error);
        this->statefulHost_->BeginOpen(timeout, callback);
    },
        [&, this] (Common::AsyncOperationSPtr const& operation) -> ErrorCode
    {
        return this->statefulHost_->EndOpen(operation);
    });
}

void FabricTestRuntimeManager::AddStatelessFabricRuntime(vector<wstring> serviceTypes)
{
    if(!statelessFabricRuntime_)
    {
        AddStatelessHost();
        this->PerformRetryableHostOperation(
            L"StatelessHost.CreateRuntime",
            [&, this] (TimeSpan timeout, Common::AsyncCallback const& callback)
        {
            this->statelessHost_->BeginCreateComFabricRuntime(timeout, callback, AsyncOperationSPtr());
        },
            [&, this] (Common::AsyncOperationSPtr const& operation) -> ErrorCode
        {
            return this->statelessHost_->EndCreateComFabricRuntime(operation, this->statelessFabricRuntime_);
        });

        TestSession::WriteNoise(
            TraceSource, 
            "ComFabricRuntime::FabricCreateRuntime for fabric node {0} completed with success.", 
            nodeId_);

        if(serviceTypes.size() == 0)
        {
            serviceTypes = serviceFactory_->SupportedStatelessServiceTypes;
        }

        RegisterStatelessServiceType(serviceTypes);
    }
}

void FabricTestRuntimeManager::AddStatefulFabricRuntime(vector<wstring> serviceTypes)
{
    if(!statefulFabricRuntime_)
    {
        AddStatefulHost();

        this->PerformRetryableHostOperation(
            L"StatefulHost.CreateRuntime",
            [&, this] (TimeSpan timeout, Common::AsyncCallback const& callback)
        {
            this->statefulHost_->BeginCreateComFabricRuntime(timeout, callback, AsyncOperationSPtr());
        },
            [&, this] (Common::AsyncOperationSPtr const& operation) -> ErrorCode
        {
            return this->statefulHost_->EndCreateComFabricRuntime(operation, this->statefulFabricRuntime_);
        });

        TestSession::WriteNoise(
            TraceSource, 
            "ComFabricRuntime::FabricCreateRuntime for fabric node {0} completed with success.", 
            nodeId_);

        if(serviceTypes.size() == 0)
        {
            serviceTypes = serviceFactory_->SupportedStatefulServiceTypes;
        }

        RegisterStatefulServiceType(serviceTypes);
    }
}

void FabricTestRuntimeManager::PerformRetryableHostOperation(
    std::wstring const & operationName,
    BeginHostOperationCallback const & begin,
    EndHostOperationCallback const & end)
{
    TestSession::WriteNoise(
        TraceSource, 
        "Performing FabricTestRuntimeManager::{0}()",
        operationName);

    ErrorCode error;
    int retryCount = 0;
    bool done = false;
    while(!done) 
    {
        TimeSpan timeout = retryCreateHost_ ? TimeSpan::FromSeconds(10) : FabricTestSessionConfig::GetConfig().HostingOpenCloseTimeout;
        TimeSpan waitTimeSpan = timeout + timeout; // Double the timeout to decide how long to wait
        auto waiter = make_shared<AsyncOperationWaiter>();
        begin(
            timeout,
            [this, waiter, &end] (AsyncOperationSPtr const & operation) 
        { 
            ErrorCode endError = end(operation);
            waiter->SetError(endError);
            waiter->Set();
        });

        bool waitResult = waiter->WaitOne(waitTimeSpan);
        TestSession::FailTestIfNot(waitResult, "WaitOne failed due to timeout in {0}", operationName);
        
        error = waiter->GetError();
        if(!error.IsSuccess() && retryCreateHost_ &&  retryCount <= MaxRetryCount)
        {
            Sleep(5000);
            ++retryCount;
            TestSession::WriteNoise(
                TraceSource, 
                "Performing retry for {0}",
                operationName);
        }
        else
        {
            done = true;
        }
    }

    TestSession::FailTestIfNot(error.IsSuccess(), "{0}->End failed with error {1}", operationName, error);
}

void FabricTestRuntimeManager::RemoveStatelessHost(bool abort)
{
    TestSession::WriteNoise(TraceSource,
        "RemoveStatelessHost for fabric node {0} abort={1}...",
        nodeId_, abort);

    if (!abort)
    {
        ErrorCode error;
        auto waiter = make_shared<AsyncOperationWaiter>();
        TimeSpan waitTimeSpan = FabricTestSessionConfig::GetConfig().HostingOpenCloseTimeout + FabricTestSessionConfig::GetConfig().HostingOpenCloseTimeout; // Double the timeout to decide how long to wait
        statelessHost_->BeginClose(
            FabricTestSessionConfig::GetConfig().HostingOpenCloseTimeout,
            [this, waiter] (AsyncOperationSPtr const & operation) 
        { 
            ErrorCode error = statelessHost_->EndClose(operation);
            waiter->SetError(error);
            waiter->Set();
        });

        bool waitResult = waiter->WaitOne(waitTimeSpan);
        TestSession::FailTestIfNot(waitResult, "WaitOne failed due to timeout in RemoveStatelessHost");
        
        error = waiter->GetError();
        TestSession::FailTestIfNot(error.IsSuccess(), "statelesshost_->Close failed with error {0}", error);
    }
    else
    {
        statelessHost_->Abort();
    }
}

void FabricTestRuntimeManager::RemoveStatefulHost(bool abort)
{
    TestSession::WriteNoise(TraceSource,
        "RemoveStatefulHost for fabric node {0} abort={1}...",
        nodeId_, abort);

    if (!abort)
    {
        ErrorCode error;

        auto waiter = make_shared<AsyncOperationWaiter>();
        statefulHost_->BeginClose(
            FabricTestSessionConfig::GetConfig().HostingOpenCloseTimeout,
            [this, waiter] (AsyncOperationSPtr const & operation) 
        { 
            ErrorCode error = statefulHost_->EndClose(operation);
            waiter->SetError(error);
            waiter->Set();
        });

        TimeSpan waitTimeSpan = FabricTestSessionConfig::GetConfig().HostingOpenCloseTimeout + FabricTestSessionConfig::GetConfig().HostingOpenCloseTimeout; // Double the timeout to decide how long to wait
        bool waitResult = waiter->WaitOne(waitTimeSpan);
        TestSession::FailTestIfNot(waitResult, "WaitOne failed due to timeout in RemoveStatefulHost");

        error = waiter->GetError();
        TestSession::FailTestIfNot(error.IsSuccess(), "statefulhost_->Close failed with error {0}", error);
    }
    else
    {
        statefulHost_->Abort();
    }
}

void FabricTestRuntimeManager::UnregisterStatelessFabricRuntime()
{
    if(statelessFabricRuntime_)
    {
        statefulHost_->UnregisterRuntimeAsync(statelessFabricRuntime_->get_Runtime()->RuntimeId);
        statelessFabricRuntime_.Release();
        statelessServiceTypeMap_.clear();
    }
}

void FabricTestRuntimeManager::UnregisterStatefulFabricRuntime()
{
    if(statefulFabricRuntime_)
    {
        statefulHost_->UnregisterRuntimeAsync(statefulFabricRuntime_->get_Runtime()->RuntimeId);
        statefulFabricRuntime_.Release();
        statefulServiceTypeMap_.clear();
    }
}

void FabricTestRuntimeManager::RemoveStatelessFabricRuntime(bool abort)
{
    if(statelessFabricRuntime_)
    {
        RemoveStatelessHost(abort);
        statelessFabricRuntime_.Release();
        statelessServiceTypeMap_.clear();
    }
}

void FabricTestRuntimeManager::RemoveStatefulFabricRuntime(bool abort)
{
    if(statefulFabricRuntime_)
    {
       RemoveStatefulHost(abort);
       statefulFabricRuntime_.Release();
       statefulServiceTypeMap_.clear();
    }
}

void FabricTestRuntimeManager::RegisterStatelessServiceType(vector<wstring> const & serviceTypes)
{
    if(statelessFabricRuntime_)
    {
        for (wstring const & serviceType : serviceTypes)
        {
            ComPointer<ComTestServiceFactory> comTestServiceFactoryCPtr = make_com<ComTestServiceFactory>(*serviceFactory_);
            HRESULT result = statelessFabricRuntime_->RegisterStatelessServiceFactory(serviceType.c_str(), comTestServiceFactoryCPtr.GetRawPointer());
            TestSession::FailTestIf(FAILED(result), "ComFabricRuntime::RegisterStatelessServiceFactory failed with {0} at nodeId {1}", result, nodeId_);
            statelessServiceTypeMap_.push_back(serviceType);
        }

        ComPointer<IFabricServiceGroupFactoryBuilder> factoryBuilder;
        HRESULT hr = statelessFabricRuntime_->CreateServiceGroupFactoryBuilder(factoryBuilder.InitializationAddress());
        TestSession::FailTestIf(FAILED(hr), "CreateServiceGroupFactoryBuilder failed with {0} at nodeId {1}", hr, nodeId_);

        ComPointer<IFabricStatelessServiceFactory> comServiceGroupFactory(make_com<SGComServiceFactory>(*serviceGroupFactory_), IID_IFabricStatelessServiceFactory);
        hr = factoryBuilder->AddStatelessServiceFactory(SGStatelessService::StatelessServiceType.c_str(), comServiceGroupFactory.GetRawPointer());
        TestSession::FailTestIf(FAILED(hr), "Add service factory {0} failed with {1} at nodeId {2}", SGStatelessService::StatelessServiceType, hr, nodeId_);

        ComPointer<IFabricServiceGroupFactory> serviceGroupFactory;
        hr = factoryBuilder->ToServiceGroupFactory(serviceGroupFactory.InitializationAddress());
        TestSession::FailTestIf(FAILED(hr), "ToServiceGroupFactory failed with {0} at nodeId {1}", hr, nodeId_);

        hr = statelessFabricRuntime_->RegisterServiceGroupFactory(SGServiceFactory::SGStatelessServiceGroupType.c_str(), serviceGroupFactory.GetRawPointer());        
        TestSession::FailTestIf(FAILED(hr), "RegisterServiceGroupFactory failed with {0} at nodeId {1}", hr, nodeId_);
        statelessServiceTypeMap_.push_back(SGServiceFactory::SGStatelessServiceGroupType);
    }
}

void FabricTestRuntimeManager::RegisterStatefulServiceType(vector<wstring> const & serviceTypes)
{
    if(statefulFabricRuntime_)
    {
        for (wstring const & serviceType : serviceTypes)
        {
            ComPointer<ComTestServiceFactory> comTestServiceFactoryCPtr = make_com<ComTestServiceFactory>(*serviceFactory_);
            HRESULT result = statefulFabricRuntime_->RegisterStatefulServiceFactory(serviceType.c_str(), comTestServiceFactoryCPtr.GetRawPointer());
            TestSession::FailTestIf(FAILED(result), "ComFabricRuntime::RegisterStatefulServiceFactory failed with {0} at nodeId {1}", result, nodeId_);
            statefulServiceTypeMap_.push_back(serviceType);
        }

        ComPointer<IFabricServiceGroupFactoryBuilder> factoryBuilder;
        HRESULT hr = statefulFabricRuntime_->CreateServiceGroupFactoryBuilder(factoryBuilder.InitializationAddress());
        TestSession::FailTestIf(FAILED(hr), "CreateServiceGroupFactoryBuilder failed with {0} at nodeId {1}", hr, nodeId_);

        ComPointer<IFabricStatefulServiceFactory> comServiceGroupFactory(make_com<SGComServiceFactory>(*serviceGroupFactory_), IID_IFabricStatefulServiceFactory);
        hr = factoryBuilder->AddStatefulServiceFactory(SGStatefulService::StatefulServiceType.c_str(), comServiceGroupFactory.GetRawPointer());
        TestSession::FailTestIf(FAILED(hr), "Add service factory {0} failed with {1} at nodeId {2}", SGStatefulService::StatefulServiceType, hr, nodeId_);
        hr = factoryBuilder->AddStatefulServiceFactory(SGStatefulService::StatefulServiceECCType.c_str(), comServiceGroupFactory.GetRawPointer());
        TestSession::FailTestIf(FAILED(hr), "Add service factory {0} failed with {1} at nodeId {2}", SGStatefulService::StatefulServiceType, hr, nodeId_);
        hr = factoryBuilder->AddStatefulServiceFactory(SGStatefulService::StatefulServiceECSType.c_str(), comServiceGroupFactory.GetRawPointer());
        TestSession::FailTestIf(FAILED(hr), "Add service factory {0} failed with {1} at nodeId {2}", SGStatefulService::StatefulServiceType, hr, nodeId_);
        hr = factoryBuilder->AddStatefulServiceFactory(SGStatefulService::StatefulServiceNCCType.c_str(), comServiceGroupFactory.GetRawPointer());
        TestSession::FailTestIf(FAILED(hr), "Add service factory {0} failed with {1} at nodeId {2}", SGStatefulService::StatefulServiceType, hr, nodeId_);
        hr = factoryBuilder->AddStatefulServiceFactory(SGStatefulService::StatefulServiceNCSType.c_str(), comServiceGroupFactory.GetRawPointer());
        TestSession::FailTestIf(FAILED(hr), "Add service factory {0} failed with {1} at nodeId {2}", SGStatefulService::StatefulServiceType, hr, nodeId_);

        ComPointer<ComTestServiceFactory> comStoreFactoryCPtr = make_com<ComTestServiceFactory>(*serviceFactory_);
        hr = factoryBuilder->AddStatefulServiceFactory(TestPersistedStoreService::DefaultServiceType.c_str(), comStoreFactoryCPtr.GetRawPointer());
        TestSession::FailTestIf(FAILED(hr), "Add service factory {0} failed with {1} at nodeId {2}", TestPersistedStoreService::DefaultServiceType, hr, nodeId_);

        ComPointer<IFabricServiceGroupFactory> serviceGroupFactory;
        hr = factoryBuilder->ToServiceGroupFactory(serviceGroupFactory.InitializationAddress());
        TestSession::FailTestIf(FAILED(hr), "ToServiceGroupFactory failed with {0} at nodeId {1}", hr, nodeId_);

        hr = statefulFabricRuntime_->RegisterServiceGroupFactory(SGServiceFactory::SGStatefulServiceGroupType.c_str(), serviceGroupFactory.GetRawPointer());        
        TestSession::FailTestIf(FAILED(hr), "RegisterServiceGroupFactory failed with {0} at nodeId {1}", hr, nodeId_);
        statefulServiceTypeMap_.push_back(SGServiceFactory::SGStatefulServiceGroupType);
    }
}


ReliabilityTestApi::ReconfigurationAgentComponentTestApi::ReconfigurationAgentProxyTestHelperUPtr FabricTestRuntimeManager::GetProxyForStatefulHost()
{
    return make_unique<ReliabilityTestApi::ReconfigurationAgentComponentTestApi::ReconfigurationAgentProxyTestHelper>(
        statefulHost_->ReconfigurationAgentProxyObj);
}
