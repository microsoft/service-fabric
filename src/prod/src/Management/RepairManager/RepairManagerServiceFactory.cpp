// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Naming;
using namespace Reliability;
using namespace Transport;
using namespace Store;
using namespace SystemServices;
using namespace Api;
using namespace Hosting2;
using namespace Federation;
using namespace Client;
using namespace TestHooks;
using namespace Management;
using namespace Management::RepairManager;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("RepairManagerServiceFactory");

RepairManagerServiceFactory::RepairManagerServiceFactory(
    ComPointer<IFabricRuntime> const & fabricRuntimeCPtr,
    FabricNodeConfigSPtr const & fabricNodeConfig,
    std::wstring const & workingDir,
    shared_ptr<ManualResetEvent> const & exitServiceHostEvent)
    : NodeTraceComponent(Federation::NodeInstance())
    , fabricRuntimeCPtr_(fabricRuntimeCPtr)
    , fabricNodeConfig_(fabricNodeConfig)
    , workingDir_(workingDir)
    , exitServiceHostEvent_(exitServiceHostEvent)
    , clientFactory_()
    , lock_()
    , replicatorAddress_()
    , clusterSecuritySettings_()
    , routingAgentProxyUPtr_()
    , nodeInstance_()
{
    WriteNoise(
        TraceComponent, 
        "{0} ctor: this = {1}",
        this->TraceId,
        TraceThis);

    replicatorAddress_ = fabricNodeConfig_->RepairManagerReplicatorAddress;
}

RepairManagerServiceFactory::~RepairManagerServiceFactory()
{
    WriteNoise(
        TraceComponent, 
        "{0} ~dtor: this = {1}",
        this->TraceId,
        TraceThis);
}

ErrorCode RepairManagerServiceFactory::Initialize()
{
    SecuritySettings clusterSecuritySettings;
    auto error = this->LoadClusterSecuritySettings(clusterSecuritySettings);
    if(!error.IsSuccess())
    {
        return error;
    }

    this->clusterSecuritySettings_ = move(clusterSecuritySettings);

    this->RegisterForConfigUpdate();

    auto localFabricNodeConfig = fabricNodeConfig_;

    // RDBug#1481055: Naming service endpoint is not available to external service hosts,
    // so e.g. FabricRM.exe cannot use fabric client.  This is a workaround, while a better
    // fix might be to have fabrictest push all config changes to per-node config files,
    // similar to a real scale-min deployment.
    //
    wstring testClientEndpoint;
    if (Environment::GetEnvironmentVariableW(L"_FabricTest_FabricClientEndpoint", testClientEndpoint, Common::NOTHROW()))
    {
        WriteInfo(TraceComponent,
            "{0} RepairManagerServiceFactory using test client address {1}",
            this->TraceId,
            testClientEndpoint);

        vector<wstring> connectionStrings;
        connectionStrings.push_back(move(testClientEndpoint));

        error = ClientFactory::CreateClientFactory(
            move(connectionStrings),
            clientFactory_);
    }
    else
    {
        error = ClientFactory::CreateLocalClientFactory(
            localFabricNodeConfig,
            clientFactory_);
    }

    if(!error.IsSuccess())
    {
        return error;
    }

    error = this->InitializeRoutingAgentProxy();
    if(!error.IsSuccess())
    {
        return error;
    }

    // Test configurations determine what KVS store provider (ESE vs TStore)
    // to use for replicas, so initialize the configs before registering the
    // factory.
    //
    this->InitializeFabricTestConfigurations();

    auto statefulServiceFactoryCPtr = make_com<Api::ComStatefulServiceFactory>(IStatefulServiceFactoryPtr(
        this,
        this->CreateComponentRoot()));

    auto hr = fabricRuntimeCPtr_->RegisterStatefulServiceFactory(
        ServiceModel::ServiceTypeIdentifier::RepairManagerServiceTypeId->ServiceTypeName.c_str(), 
        statefulServiceFactoryCPtr.GetRawPointer());

    return ErrorCode::FromHResult(hr);
}

ErrorCode RepairManagerServiceFactory::InitializeRoutingAgentProxy()
{
    //TODO: create an internal CLSID and do a QueryInterface 
    ComFabricRuntime * castedRuntimePtr;
    try
    {
#if !defined(PLATFORM_UNIX)
        castedRuntimePtr = dynamic_cast<ComFabricRuntime*>(fabricRuntimeCPtr_.GetRawPointer());
#else
        castedRuntimePtr = (ComFabricRuntime*)(fabricRuntimeCPtr_.GetRawPointer());
#endif
    }
    catch (...)
    {
        WriteWarning(TraceComponent, TraceId, "RepairManagerServiceFactory unable to get ComFabricRuntime");
        Assert::TestAssert("RepairManagerServiceFactory unable to get ComFabricRuntime");
        return ErrorCodeValue::OperationFailed;
    }

    FabricNodeContextSPtr fabricNodeContext;
    auto error = castedRuntimePtr->Runtime->Host.GetFabricNodeContext(fabricNodeContext);
    if(!error.IsSuccess())
    {
        return error;
    }

    NodeId nodeId;
    if (!NodeId::TryParse(fabricNodeContext->NodeId, nodeId))
    {
        Assert::TestAssert("Could not parse NodeId {0}", fabricNodeContext->NodeId);
        return ErrorCodeValue::OperationFailed;
    }

    uint64 nodeInstanceId = fabricNodeContext->NodeInstanceId;
    nodeInstance_ = Federation::NodeInstance(nodeId, nodeInstanceId);
    nodeName_ = fabricNodeContext->NodeName;

    this->SetTraceInfo(nodeInstance_);

    IpcClient & ipcClient = castedRuntimePtr->Runtime->Host.Client;

    auto temp = ServiceRoutingAgentProxy::Create(nodeInstance_, ipcClient, *this);
    routingAgentProxyUPtr_.swap(temp);

    error = routingAgentProxyUPtr_->Open();
    if(!error.IsSuccess())
    {
        return error;
    }

    return ErrorCodeValue::Success;
}

// See prod\test\TestHooks\EnvironmentVariables.h
//
#define GET_BOOLEAN_ENV( EnvName, ConfigName ) GET_FABRIC_TEST_BOOLEAN_ENV( EnvName, RepairManagerConfig, ConfigName )
#define GET_BOOL_TO_INT_ENV( EnvName, ConfigName ) GET_FABRIC_TEST_BOOL_TO_INT_ENV( EnvName, RepairManagerConfig, ConfigName )
#define GET_INT_ENV( EnvName, ConfigName ) GET_FABRIC_TEST_INT_ENV( EnvName, RepairManagerConfig, ConfigName )

// RM runs in an activated process. Environment variables are the current
// mechanism for passing configuration from FabricTest.exe to activated
// processes when the activated process does not have direct access
// to FabricTestCommonConfig.h. See prod\test\FabricNodeWrapper.cpp.
//
void RepairManagerServiceFactory::InitializeFabricTestConfigurations()
{
    GET_BOOLEAN_ENV( EnableTStore, EnableTStore )
    GET_BOOLEAN_ENV( EnableHashSharedLogId, EnableHashSharedLogIdFromPath )
    GET_BOOL_TO_INT_ENV( EnableSparseSharedLogs, CreateFlags )
    GET_INT_ENV( SharedLogSize, LogSize )
    GET_INT_ENV( SharedLogMaximumRecordSize, MaximumRecordSize )
}

ErrorCode RepairManagerServiceFactory::LoadClusterSecuritySettings(SecuritySettings & clusterSecuritySettings)
{
    SecurityConfig & securityConfig = SecurityConfig::GetConfig();
    auto constructError = SecuritySettings::FromConfiguration(
        securityConfig.ClusterCredentialType,
        fabricNodeConfig_->ClusterX509StoreName,
        wformatString(X509Default::StoreLocation()),
        fabricNodeConfig_->ClusterX509FindType,
        fabricNodeConfig_->ClusterX509FindValue,
        fabricNodeConfig_->ClusterX509FindValueSecondary,
        securityConfig.ClusterProtectionLevel,
        securityConfig.ClusterCertThumbprints,
        securityConfig.ClusterX509Names,
        securityConfig.ClusterCertificateIssuerStores,
        securityConfig.ClusterAllowedCommonNames,
        securityConfig.ClusterCertIssuers,
        securityConfig.ClusterSpn,
        securityConfig.ClusterIdentities,
        clusterSecuritySettings);

    if(!constructError.IsSuccess())
    {
        WriteError(
            TraceComponent, 
            TraceId,
            "clusterSecuritySettings.FromConfiguration error={0}",
            constructError);
        return constructError;
    }

    return ErrorCodeValue::Success;
}

void RepairManagerServiceFactory::RegisterForConfigUpdate()
{
    WriteInfo(TraceComponent, TraceId, "Register for configuration updates");
    weak_ptr<ComponentRoot> rootWPtr = this->shared_from_this();

    // Global security settings
    // X509 related
    SecurityConfig & securityConfig = SecurityConfig::GetConfig();

    securityConfig.ClusterCertThumbprintsEntry.AddHandler(
        [rootWPtr] (EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });

    securityConfig.ClusterX509NamesEntry.AddHandler(
        [rootWPtr](EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });

    securityConfig.ClusterAllowedCommonNamesEntry.AddHandler(
        [rootWPtr](EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });

    securityConfig.ClusterCertIssuersEntry.AddHandler( // Issuer thumbprint lists
        [rootWPtr] (EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });

    securityConfig.ClusterCertificateIssuerStoresEntry.AddHandler( // Issuer thumbprint lists
        [rootWPtr](EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });

    // Kerberos related
    securityConfig.ClusterIdentitiesEntry.AddHandler(
        [rootWPtr] (EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });

    // Per-node security settings
    // X509 related
    fabricNodeConfig_->ClusterX509StoreNameEntry.AddHandler(
        [this, rootWPtr] (EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });
    fabricNodeConfig_->ClusterX509FindTypeEntry.AddHandler(
        [this, rootWPtr] (EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });
    fabricNodeConfig_->ClusterX509FindValueEntry.AddHandler(
        [this, rootWPtr] (EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });
    fabricNodeConfig_->ClusterX509FindValueSecondaryEntry.AddHandler(
        [this, rootWPtr] (EventArgs const&) { ClusterSecuritySettingUpdateHandler(rootWPtr); });
}

void RepairManagerServiceFactory::ClusterSecuritySettingUpdateHandler(weak_ptr<ComponentRoot> const & rootWPtr)
{
    auto rootSPtr = rootWPtr.lock();
    if (rootSPtr)
    {
        RepairManagerServiceFactory* thisPtr = static_cast<RepairManagerServiceFactory*>(rootSPtr.get());
        SecuritySettings updatedSecuritySettings;
        auto error = thisPtr->LoadClusterSecuritySettings(updatedSecuritySettings);
        if(!error.IsSuccess())
        {
            thisPtr->exitServiceHostEvent_->Set();
        }

        error = thisPtr->UpdateReplicatorSecuritySettings(updatedSecuritySettings);
        if(!error.IsSuccess())
        {
            thisPtr->exitServiceHostEvent_->Set();
        }
    }
}

ErrorCode RepairManagerServiceFactory::CreateReplica(
    wstring const & serviceType,
    NamingUri const & serviceName,
    vector<byte> const & initializationData, 
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    __out IStatefulServiceReplicaPtr & serviceReplica)
{
    ASSERT_IF(
        serviceType != ServiceModel::ServiceTypeIdentifier::RepairManagerServiceTypeId->ServiceTypeName,
        "RepairManagerServiceFactory cannot create service of type '{0}'", serviceType);

    RepairManagerConfig & config = RepairManagerConfig::GetConfig();
    Reliability::ReplicationComponent::ReplicatorSettingsUPtr replicatorSettings;

    auto error = Reliability::ReplicationComponent::ReplicatorSettings::FromConfig(
        config,
        replicatorAddress_,
        replicatorAddress_,
        replicatorAddress_,
        clusterSecuritySettings_,
        replicatorSettings);
    if (!error.IsSuccess()) { return error; }

    RepairManagerServiceReplicaSPtr repairManagerServiceReplicaSPtr;
    {
        AcquireWriteLock lock(lock_);

        repairManagerServiceReplicaSPtr = make_shared<RepairManagerServiceReplica>(
            partitionId,
            replicaId,
            serviceName.ToString(),
            *routingAgentProxyUPtr_,
            clientFactory_,
            nodeInstance_,
            RepairManagerServiceFactoryHolder(*this, this->CreateComponentRoot()));
        
        error = repairManagerServiceReplicaSPtr->InitializeForSystemServices(
            config.EnableTStore,
            workingDir_,
            Constants::DatabaseDirectory,
            Constants::SharedLogFilename,
            config,
            partitionId,
            move(replicatorSettings),
            Store::ReplicatedStoreSettings(),
            Store::EseLocalStoreSettings(
                Constants::DatabaseFilename,
                Path::Combine(workingDir_, Constants::DatabaseDirectory),
                RepairManagerConfig::GetConfig().CompactionThresholdInMB),
            clientFactory_,
            serviceName,
            initializationData);

        if (!error.IsSuccess()) { return error; }

        auto iter = replicaMap_.find(partitionId);
        if(iter != replicaMap_.end())
        {
            replicaMap_.erase(iter);
        }

        replicaMap_.insert(make_pair(partitionId, repairManagerServiceReplicaSPtr));
    }

    auto selfRoot = this->CreateComponentRoot();
    repairManagerServiceReplicaSPtr->OnCloseReplicaCallback =
        [this, selfRoot](Guid const & partitionId)
    {
        this->OnServiceCloseReplica(partitionId);
    };

    serviceReplica = IStatefulServiceReplicaPtr(repairManagerServiceReplicaSPtr.get(), repairManagerServiceReplicaSPtr->CreateComponentRoot());
    
    return ErrorCodeValue::Success;
}

Common::ErrorCode RepairManagerServiceFactory::UpdateReplicatorSecuritySettings(
    Transport::SecuritySettings const & securitySettings)
{
    ErrorCode error;
    ReplicationComponent::ReplicatorSettingsUPtr replicatorSettings;

    error = ReplicationComponent::ReplicatorSettings::FromSecuritySettings(
        securitySettings,
        replicatorSettings);

    if (!error.IsSuccess())
    {
        return error;
    }

    {
        AcquireExclusiveLock lock(lock_);
        clusterSecuritySettings_ = securitySettings;

        for (auto & item : replicaMap_)
        {
            auto replica = item.second.lock();
            auto clonedReplicatorSettings = replicatorSettings->Clone();

            if (replica)
            {
                error = replica->UpdateReplicatorSettings(move(clonedReplicatorSettings));
                if (error.IsSuccess())
                {
                    WriteInfo(
                        TraceComponent,
                        "{0}({1}) UpdateReplicatorSecuritySettings succeeded, new value = {2}",
                        this->TraceId,
                        item.first,
                        securitySettings);
                }
                else
                {
                    WriteError(
                        TraceComponent,
                        "{0}({1}) UpdateReplicatorSecuritySettings failed: error = {2}, new value = {3}",
                        this->TraceId,
                        item.first,
                        error,
                        securitySettings);
                    return error;
                }
            }
        }
    }

    return ErrorCodeValue::Success;
}

void RepairManagerServiceFactory::OnServiceCloseReplica(Guid const & partitionId)
{
    AcquireWriteLock lock(lock_);
    auto findIter = replicaMap_.find(partitionId);
    if(findIter != replicaMap_.end())
    {
        replicaMap_.erase(findIter);
    }
}
