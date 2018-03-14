// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace FabricTest;
using namespace std;
using namespace Common;
using namespace TestCommon;
using namespace Transport;
using namespace Hosting2;

StringLiteral const TraceSource("FabricTestHost.ServiceFactory");

TestHostServiceFactory::TestHostServiceFactory(
    std::wstring const& traceId, 
    std::wstring const& codePackageId, 
    std::wstring const& appId, 
    std::wstring const& packageName, 
    std::wstring const& workingDir,
    SecuritySettings const& clientSecuritySettings)
    : traceId_(traceId),
    codePackageId_(codePackageId),
    appId_(appId),
    packageName_(packageName),
    workingDir_(workingDir),
    activeServices_(),
    lock_(),
    clientFactory_()
{
    TestSession::WriteNoise(TraceSource, traceId_, "Ctor TestHostServiceFactory");

    auto appHostSPtr = ApplicationHostContainer::GetApplicationHostContainer().GetApplicationHost();

    FabricNodeContextSPtr nodeContextSPtr;
    auto error = appHostSPtr->GetFabricNodeContext(nodeContextSPtr);

    TestSession::FailTestIfNot(error.IsSuccess(), "GetFabricNodeContext failed: {0}", error);

    vector<wstring> gatewayAddresses;
    gatewayAddresses.push_back(nodeContextSPtr->ClientConnectionAddress);

    TestSession::WriteInfo(TraceSource, traceId_, "ClientConnectionAddresses: {0}", gatewayAddresses);

    error = Client::ClientFactory::CreateClientFactory(move(gatewayAddresses), clientFactory_);

    TestSession::FailTestIfNot(error.IsSuccess(), "CreateClientFactory failed: {0}", error);

    if (clientSecuritySettings.SecurityProvider() != SecurityProvider::None)
    {
        IClientSettingsPtr settings;
        error = clientFactory_->CreateSettingsClient(settings);

        TestSession::FailTestIfNot(error.IsSuccess(), "CreateSettingsClient failed: {0}", error);

        error = settings->SetSecurity(SecuritySettings(clientSecuritySettings));

        TestSession::FailTestIfNot(error.IsSuccess(), "SetSecurity failed: {0}", error);
    }

    this->InitializeSharedLogSettings();
}

TestHostServiceFactory::~TestHostServiceFactory()
{
    TestSession::WriteNoise(TraceSource, traceId_, "Dtor TestHostServiceFactory");
}

CalculatorServiceSPtr TestHostServiceFactory::CreateStatelessService(FABRIC_PARTITION_ID partitionId, wstring const& serviceName, wstring const& serviceType, wstring const& initDataStr)
{
    TestSession::WriteNoise(TraceSource, traceId_, "CreateStatelessService with name:type {0}:{1}", serviceName, serviceType);
    wstring qualifiedServiceType = ServiceModel::ServiceTypeIdentifier::GetQualifiedServiceTypeName(appId_, packageName_, serviceType);
    NodeId nodeId;
    TestSession::FailTestIfNot(NodeId::TryParse(FABRICHOSTSESSION.NodeId, nodeId), "could not parse node id {0}", FABRICHOSTSESSION.NodeId);
    CalculatorServiceSPtr statelessServiceSPtr = make_shared<CalculatorService>(nodeId, partitionId, serviceName, initDataStr, qualifiedServiceType, appId_);

    //Keeps the TestServiceFactory alive
    auto selfRoot = CreateComponentRoot();

    statelessServiceSPtr->OnOpenCallback = 
        [this, selfRoot] (TestServiceInfo const & serviceInfo, ComponentRootSPtr const& root, bool hasPersistedState)
    {
        ProcessOpenCallback(serviceInfo, root, hasPersistedState);
    };

    statelessServiceSPtr->OnCloseCallback = 
        [this, selfRoot] (wstring const& location)
    {
        ProcessCloseCallback(location);
    };

    return statelessServiceSPtr;
}

TestStoreServiceSPtr TestHostServiceFactory::CreateStatefulService(FABRIC_PARTITION_ID partitionId, wstring const& serviceName, wstring const& serviceType, wstring const& initDataStr)
{
    TestSession::WriteNoise(TraceSource, traceId_, "CreateStatefulService with name:type {0}:{1}", serviceName, serviceType);
    wstring qualifiedServiceType = ServiceModel::ServiceTypeIdentifier::GetQualifiedServiceTypeName(appId_, packageName_, serviceType);

    NodeId nodeId;
    TestSession::FailTestIfNot(NodeId::TryParse(FABRICHOSTSESSION.NodeId, nodeId), "could not parse node id {0}", FABRICHOSTSESSION.NodeId);
    TestStoreServiceSPtr storeServiceSPtr = make_shared<TestStoreService>(partitionId, serviceName, nodeId, false, initDataStr, qualifiedServiceType, appId_);

    //Keeps the TestServiceFactory alive
    auto selfRoot = CreateComponentRoot();

    storeServiceSPtr->OnOpenCallback = 
        [this, selfRoot] (TestServiceInfo const & serviceInfo, ComponentRootSPtr const& root, bool hasPersistedState)
    {
        ProcessOpenCallback(serviceInfo, root, hasPersistedState);
    };

    storeServiceSPtr->OnCloseCallback = 
        [this, selfRoot] (wstring const& location)
    {
        ProcessCloseCallback(location);
    };

    storeServiceSPtr->OnChangeRoleCallback = 
        [this, selfRoot] (wstring const& oldLocation, wstring const& newLocation, FABRIC_REPLICA_ROLE replicaRole)
    {
        TestCommon::TestSession::FailTestIfNot(newLocation.empty(), "newLocation is not empty: {0}", newLocation);
        ProcessChangeRoleCallback(oldLocation, replicaRole);
    };

    return storeServiceSPtr;
}

// See prod\test\TestHooks\EnvironmentVariables.h
//
#define GET_BOOLEAN_ENV( EnvName, ConfigName ) GET_FABRIC_TEST_BOOLEAN_ENV( EnvName, FabricTestCommonConfig, ConfigName )
#define GET_BOOL_TO_INT_ENV( EnvName, ConfigName ) GET_FABRIC_TEST_BOOL_TO_INT_ENV( EnvName, FabricTestCommonConfig, ConfigName )
#define GET_INT_ENV( EnvName, ConfigName ) GET_FABRIC_TEST_INT_ENV( EnvName, FabricTestCommonConfig, ConfigName )

// This replica runs in a hosted process outside FabricTest.exe.
// Global shared log settings for the test must be applied
// explicitly for them to take effect on this replica.
// See FabricTestDispatcher::SetSharedLogSettings()
//
void TestHostServiceFactory::InitializeSharedLogSettings()
{
    wstring unused;
    if (Environment::GetEnvironmentVariableW(TestHooks::EnvironmentVariables::FabricTest_EnableTStore, unused, Common::NOTHROW()))
    {
        GET_BOOLEAN_ENV( EnableTStore, EnableTStore )
        GET_BOOLEAN_ENV( EnableHashSharedLogId, EnableHashSharedLogIdFromPath )
        GET_BOOL_TO_INT_ENV( EnableSparseSharedLogs, CreateFlags )
        GET_INT_ENV( SharedLogSize, LogSize )
        GET_INT_ENV( SharedLogMaximumRecordSize, MaximumRecordSize )
    }
    else
    {
        // Some FabricTest CITs activate FabricTestHost in a sandbox, so environment variables will not be inherited.
        // Ensure the defaults are at least still loaded in this case.
        //
        FabricTestCommonConfig::GetConfig().EnableHashSharedLogIdFromPath = FabricTestCommonConfig::GetConfig().EnableHashSharedLogId;
        FabricTestCommonConfig::GetConfig().CreateFlags = FabricTestCommonConfig::GetConfig().EnableSparseSharedLogs ? 1 : 0;
        FabricTestCommonConfig::GetConfig().LogSize = FabricTestCommonConfig::GetConfig().SharedLogSize;
        FabricTestCommonConfig::GetConfig().MaximumRecordSize = FabricTestCommonConfig::GetConfig().SharedLogMaximumRecordSize;
    }
}

TestPersistedStoreServiceSPtr TestHostServiceFactory::CreatePersistedService(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    wstring const& serviceName,
    wstring const& serviceType,
    wstring const& initDataStr)
{
    TestSession::WriteInfo(TraceSource, traceId_, "{0} Creating TestPersistedStoreService with name:type {1}:{2} init='{3}'", traceId_, serviceName, serviceType, initDataStr);
    wstring qualifiedServiceType = ServiceModel::ServiceTypeIdentifier::GetQualifiedServiceTypeName(appId_, packageName_, serviceType);

    NodeId nodeId;
    TestSession::FailTestIfNot(NodeId::TryParse(FABRICHOSTSESSION.NodeId, nodeId), "could not parse node id {0}", FABRICHOSTSESSION.NodeId);

    ReplicatorSettingServiceInitDataParser rsParser(initDataStr);
    auto replicatorSettings = rsParser.CreateReplicatorSettings(initDataStr, Guid(partitionId).ToString());

    auto storeServiceSPtr = make_shared<TestPersistedStoreService>(
        Guid(partitionId),
        replicaId,
        serviceName,
        nodeId,
        false,
        FABRICHOSTSESSION,
        initDataStr,
        qualifiedServiceType,
        appId_);

    // Settings are passed from FabricTest.exe to the test host via environment variables
    // (see InitializeSharedLogSettings)
    //
    auto & config = FabricTestCommonConfig::GetConfig();

    storeServiceSPtr->InitializeForTestServices(
        config.EnableTStore,
        workingDir_,
        TestPersistedStoreService::TestPersistedStoreDirectory,
        TestPersistedStoreService::SharedLogFilename,
        config,
        Guid(partitionId),
        move(replicatorSettings),
        Store::ReplicatedStoreSettings(
            StoreConfig::GetConfig().EnableUserServiceFlushOnDrain,
            Store::SecondaryNotificationMode::BlockSecondaryAck),
        Store::EseLocalStoreSettings(
            TestPersistedStoreService::EseDatabaseFilename, 
            Path::Combine(workingDir_, TestPersistedStoreService::TestPersistedStoreDirectory)),
        clientFactory_, 
        NamingUri(serviceName),
        true); // allowRepairUpToQuorum

    //Keeps the TestServiceFactory alive
    auto selfRoot = CreateComponentRoot();

    storeServiceSPtr->OnOpenCallback = 
        [this, selfRoot] (TestServiceInfo const & serviceInfo, ComponentRootSPtr const& root, bool hasPersistedState)
    {
        ProcessOpenCallback(serviceInfo, root, hasPersistedState);
    };

    storeServiceSPtr->OnCloseCallback = 
        [this, selfRoot] (wstring const& location)
    {
        ProcessCloseCallback(location);
    };

    storeServiceSPtr->OnChangeRoleCallback = 
        [this, selfRoot] (wstring const& oldLocation, wstring const& newLocation, FABRIC_REPLICA_ROLE replicaRole)
    {
        TestCommon::TestSession::FailTestIfNot(newLocation.empty(), "newLocation is not empty: {0}", newLocation);
        ProcessChangeRoleCallback(oldLocation, replicaRole);
    };

    return storeServiceSPtr;
}

bool TestHostServiceFactory::TryGetServiceInfo(wstring const& location, TestServiceInfo & testServiceInfo) const
{
    auto it = activeServices_.find(location);
    if (it != activeServices_.end())
    {
        testServiceInfo = (*it).second;
        return true;
    }

    return false;
}

void TestHostServiceFactory::ProcessChangeRoleCallback(wstring const& serviceLocation, FABRIC_REPLICA_ROLE replicaRole)
{
    TestSession::WriteInfo(TraceSource, "{0} ProcessChangeRoleCallback({1})", traceId_, serviceLocation);

    AcquireWriteLock grab(lock_);
    TestCommon::TestSession::FailTestIf(!ContainsServiceInfo(serviceLocation), "{0} does not exist", serviceLocation);
    activeServices_[serviceLocation].StatefulServiceRole = replicaRole;
}

bool TestHostServiceFactory::ContainsServiceInfo(wstring const& location) const
{
    TestServiceInfo testServiceInfo;
    return (TryGetServiceInfo(location, testServiceInfo));
}

void TestHostServiceFactory::ProcessOpenCallback(TestServiceInfo const & serviceInfo, ComponentRootSPtr const& root, bool hasPersistedState)
{
    UNREFERENCED_PARAMETER(hasPersistedState);

    TestSession::WriteInfo(TraceSource, "{0} ProcessOpenCallback({1})", traceId_, serviceInfo.ServiceLocation);

    AcquireWriteLock grab(lock_);
    TestCommon::TestSession::FailTestIf(ContainsServiceInfo(serviceInfo.ServiceLocation), "{0} already exists", serviceInfo.ServiceLocation);
    activeServices_[serviceInfo.ServiceLocation] = serviceInfo;

    if (serviceInfo.IsStateful)
    {
        auto casted = dynamic_pointer_cast<ITestStoreService>(const_pointer_cast<ComponentRoot>(root));
        TestCommon::TestSession::FailTestIf(casted.get() == nullptr, "{0} cannot cast to ITestStoreService: {1}", traceId_, serviceInfo);

        storeServices_.push_back(move(casted));
    }
}

void TestHostServiceFactory::ProcessCloseCallback(wstring const& location)
{
    TestSession::WriteInfo(TraceSource, "{0} ProcessCloseCallback({1})", traceId_, location);

    bool noMoreActiveServices = false;
    {
        AcquireWriteLock grab(lock_);
        TestCommon::TestSession::FailTestIf(!ContainsServiceInfo(location), "{0} does not exist", location);
        if(activeServices_[location].IsStateful)
        {
            bool found = false;
            for (auto iterator = storeServices_.begin() ; iterator != storeServices_.end(); ++iterator)
            {
                if((*iterator)->GetServiceLocation().compare(location) == 0)
                {
                    storeServices_.erase(iterator);
                    found = true;
                    break;
                }
            }

            TestSession::FailTestIfNot(found, "No test store service found at location {0}", location);
        }

        activeServices_.erase(location);
        noMoreActiveServices = activeServices_.size() == 0;
    }

    if(noMoreActiveServices)
    {
        std::map<std::wstring, TestServiceInfo> activeServices;
        FABRICHOSTSESSION.Dispatcher.SendPlacementData(activeServices, codePackageId_);
    }
}

void TestHostServiceFactory::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    AcquireReadLock grab(lock_);
    if(activeServices_.size() == 0)
    {
        writer.Write("No active services");
    }

    for ( auto iterator = activeServices_.begin() ; iterator != activeServices_.end(); iterator++ )
    {
        TestServiceInfo const & serviceInfo = (*iterator).second;
        writer.WriteLine("{0}", serviceInfo);
    }
}

bool TestHostServiceFactory::TryFindStoreService(std::wstring const& serviceLocation, ITestStoreServiceSPtr & storeServiceSPtr)
{
    AcquireReadLock grab(lock_);
    for ( auto iterator = storeServices_.begin() ; iterator != storeServices_.end(); iterator++ )
    {
        if((*iterator)->GetServiceLocation().compare(serviceLocation) == 0)
        {
            storeServiceSPtr = (*iterator);
            return true;
        }
    }

    return false;
}
