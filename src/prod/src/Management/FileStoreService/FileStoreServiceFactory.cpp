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
using namespace Management;
using namespace FileStoreService;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("FileStoreServiceFactory");

FileStoreServiceFactory::FileStoreServiceFactory(
    ComPointer<IFabricRuntime> const & fabricRuntimeCPtr,
    FabricNodeConfigSPtr const & fabricNodeConfig,
    std::wstring const & workingDir,
    wstring const & ipAddressOrFQDN,
    shared_ptr<ManualResetEvent> const & exitServiceHostEvent)
    : NodeTraceComponent(Federation::NodeInstance())
    , fabricRuntimeCPtr_(fabricRuntimeCPtr)
    , fabricNodeConfig_(fabricNodeConfig)
    , workingDir_(workingDir)
    , ipAddressOrFQDN_(ipAddressOrFQDN)
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
        static_cast<void*>(this));

    replicatorAddress_ = fabricNodeConfig_->ImageStoreServiceReplicatorAddress;
}

FileStoreServiceFactory::~FileStoreServiceFactory()
{
    WriteNoise(
        TraceComponent, 
        "{0} ~dtor: this = {1}",
        this->TraceId,
        static_cast<void*>(this));
}

ErrorCode FileStoreServiceFactory::Initialize()
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
    error = ClientFactory::CreateLocalClientFactory(
        localFabricNodeConfig,
        clientFactory_);
    if(!error.IsSuccess())
    {
        return error;
    }

    error = this->InitializeRoutingAgentProxy();
    if(!error.IsSuccess())
    {
        return error;
    }

    auto statefulServiceFactoryCPtr = make_com<Api::ComStatefulServiceFactory>(IStatefulServiceFactoryPtr(
        this,
        this->CreateComponentRoot()));
            
    auto hr = fabricRuntimeCPtr_->RegisterStatefulServiceFactory(
        ServiceModel::ServiceTypeIdentifier::FileStoreServiceTypeId->ServiceTypeName.c_str(), 
        statefulServiceFactoryCPtr.GetRawPointer());

    return ErrorCode::FromHResult(hr);
}

ErrorCode FileStoreServiceFactory::InitializeRoutingAgentProxy()
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
        WriteWarning(TraceComponent, TraceId, "FileStoreServiceFactory unable to get ComFabricRuntime");
        Assert::TestAssert("FileStoreServiceFactory unable to get ComFabricRuntime");
        return ErrorCodeValue::OperationFailed;
    }

    FabricNodeContextSPtr fabricNodeContext;
    auto error = castedRuntimePtr->Runtime->Host.GetFabricNodeContext(fabricNodeContext);
    if (!error.IsSuccess())
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

ErrorCode FileStoreServiceFactory::LoadClusterSecuritySettings(SecuritySettings & clusterSecuritySettings)
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

void FileStoreServiceFactory::RegisterForConfigUpdate()
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

    securityConfig.ClusterCertificateIssuerStoresEntry.AddHandler( // Issuer stores
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

void FileStoreServiceFactory::ClusterSecuritySettingUpdateHandler(weak_ptr<ComponentRoot> const & rootWPtr)
{
    auto rootSPtr = rootWPtr.lock();
    if (rootSPtr)
    {
        FileStoreServiceFactory* thisPtr = static_cast<FileStoreServiceFactory*>(rootSPtr.get());
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

ErrorCode FileStoreServiceFactory::CreateReplica(
    std::wstring const & serviceType,
    Common::NamingUri const & serviceName,
    std::vector<byte> const & initializationData, 
    Common::Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    __out Api::IStatefulServiceReplicaPtr & serviceReplica)
{
    ASSERT_IF(
        serviceType != ServiceModel::ServiceTypeIdentifier::FileStoreServiceTypeId->ServiceTypeName, 
        "FileStoreServiceFactory cannot create service of type '{0}'", serviceType);    

    FileStoreServiceConfig & config = FileStoreServiceConfig::GetConfig();    
    Reliability::ReplicationComponent::ReplicatorSettingsUPtr replicatorSettings;

    auto error = Reliability::ReplicationComponent::ReplicatorSettings::FromConfig(
        config,
        replicatorAddress_,
        replicatorAddress_,
        replicatorAddress_,
        clusterSecuritySettings_,
        replicatorSettings);
    if (!error.IsSuccess())
    {
        return error;
    }

    shared_ptr<FileStoreServiceReplica> fileStoreServiceReplicaSPtr;    
    {
        AcquireWriteLock lock(lock_);

        fileStoreServiceReplicaSPtr = make_shared<FileStoreServiceReplica>(
            partitionId,
            replicaId,
            serviceName.ToString(),
            *routingAgentProxyUPtr_,
            clientFactory_,
            FileStoreServiceReplica::NodeInfo(nodeName_, nodeInstance_, workingDir_, ipAddressOrFQDN_),
            FileStoreServiceFactoryHolder(*this, this->CreateComponentRoot()));
        
        error = fileStoreServiceReplicaSPtr->InitializeForSystemServices(
            config.EnableTStore,
            workingDir_,
            Constants::DatabaseDirectory,
            Constants::SharedLogFilename,
            config,
            partitionId,
            move(replicatorSettings),
            Store::ReplicatedStoreSettings(
                FileStoreServiceConfig::GetConfig().CommitBatchingPeriod,
                FileStoreServiceConfig::GetConfig().CommitBatchingSizeLimitInKB << 10,
                FileStoreServiceConfig::GetConfig().TransactionLowWatermark,
                FileStoreServiceConfig::GetConfig().TransactionHighWatermark,
                FileStoreServiceConfig::GetConfig().CommitBatchingPeriodExtension,
                FileStoreServiceConfig::GetConfig().ThrottleOperationCount,
                FileStoreServiceConfig::GetConfig().ThrottleQueueSizeBytes,
                StoreConfig::GetConfig().EnableSystemServiceFlushOnDrain, 
                SecondaryNotificationMode::BlockSecondaryAck),
            Store::EseLocalStoreSettings(
                Constants::DatabaseFilename, 
                Path::Combine(workingDir_, Constants::DatabaseDirectory),
                FileStoreServiceConfig::GetConfig().CompactionThresholdInMB,
                FileStoreServiceConfig::GetConfig().MaxCursors,
                FileStoreServiceConfig::GetConfig().MaxCacheSizeInMB,
                FileStoreServiceConfig::GetConfig().MinCacheSizeInMB),
            clientFactory_,
            serviceName,
            initializationData);

        if (!error.IsSuccess()) { return error; }

        auto iter = replicaMap_.find(partitionId);
        if(iter != replicaMap_.end())
        {
            replicaMap_.erase(iter);
        }

        replicaMap_.insert(make_pair(partitionId, fileStoreServiceReplicaSPtr));
    }

    auto selfRoot = this->CreateComponentRoot();
    fileStoreServiceReplicaSPtr->OnCloseReplicaCallback = 
        [this, selfRoot](Guid const & partitionId)
    {
        this->OnServiceCloseReplica(partitionId);
    };

    serviceReplica = IStatefulServiceReplicaPtr(fileStoreServiceReplicaSPtr.get(), fileStoreServiceReplicaSPtr->CreateComponentRoot());

    return ErrorCodeValue::Success;
}

Common::ErrorCode FileStoreServiceFactory::UpdateReplicatorSecuritySettings(
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
            if(replica)
            {
                error = replica->UpdateReplicatorSettings(move(replicatorSettings));
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

void FileStoreServiceFactory::OnServiceCloseReplica(Guid const & partitionId)
{
    AcquireWriteLock lock(lock_);
    auto findIter = replicaMap_.find(partitionId);
    if(findIter != replicaMap_.end())
    {
        replicaMap_.erase(findIter);
    }
}
