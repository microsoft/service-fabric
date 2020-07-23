// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace Store;
using namespace Transport;

StringLiteral const TraceFactory("Factory");

GlobalWString FailoverManagerServiceFactory::FMStoreDirectory = make_global<wstring>(L"FM");
GlobalWString FailoverManagerServiceFactory::FMStoreFilename = make_global<wstring>(L"FM.edb");
GlobalWString FailoverManagerServiceFactory::FMSharedLogFilename = make_global<wstring>(L"fmshared.log");

FailoverManagerServiceFactory::FailoverManagerServiceFactory(
    __in Reliability::IReliabilitySubsystem & reliabilitySubsystem,
    EventHandler const & readyEvent, 
    EventHandler const & failureEvent,
    ComponentRoot const & root)
    : ComponentRoot()
    , federationSPtr_(reliabilitySubsystem.Federation.shared_from_this())
    , instance_(reliabilitySubsystem.FederationWrapperBase.Instance)
    , fmServiceLock_()
    , fmServiceWPtr_()
    , fm_()
    , fmReadyEvent_()
    , fmFailedEvent_()
    , fabricRoot_(root.CreateComponentRoot())
    , reliabilitySubsystem_(reliabilitySubsystem)
{
    fmReadyEvent_.Add(readyEvent);
    fmFailedEvent_.Add(failureEvent);

    WriteInfo(
        TraceFactory, 
        "FailoverManagerServiceFactory::ctor: node instance = {0}",
        instance_);
}

FailoverManagerServiceFactory::~FailoverManagerServiceFactory()
{
    WriteInfo(
        TraceFactory, 
        "FailoverManagerServiceFactory::~dtor: node instance = {0}",
        instance_);
}

ErrorCode FailoverManagerServiceFactory::CreateReplica(
    wstring const & serviceType,
    NamingUri const & serviceName,
    vector<byte> const & initializationData, 
    Guid const & partitionId,
    FABRIC_REPLICA_ID replicaId,
    __out IStatefulServiceReplicaPtr & replicaResult)
{
    WriteInfo(
        TraceFactory, 
        "CreateReplica called at node instance = {0}",
        instance_);

    ASSERT_IF(
        serviceType != ServiceModel::ServiceTypeIdentifier::FailoverManagerServiceTypeId->ServiceTypeName, 
        "StoreServiceFactory cannot create service of type '{0}'", serviceType);
    ASSERT_IF(serviceName.ToString() != ServiceModel::SystemServiceApplicationNameHelper::PublicFMServiceName, "StoreServiceFactory cannot create service '{0}'", serviceName);

    FailoverConfig & config = FailoverConfig::GetConfig();
    ReplicationComponent::ReplicatorSettingsUPtr replicatorSettings;

    auto error = ReplicationComponent::ReplicatorSettings::FromConfig(
        config,
        reliabilitySubsystem_.NodeConfig->FailoverManagerReplicatorAddress,
        reliabilitySubsystem_.NodeConfig->FailoverManagerReplicatorAddress,
        reliabilitySubsystem_.NodeConfig->FailoverManagerReplicatorAddress,
        reliabilitySubsystem_.SecuritySettings,
        replicatorSettings);
    if (!error.IsSuccess()) { return error; }

    shared_ptr<FailoverManagerService> fmServiceSPtr;
    {
        AcquireWriteLock lock(fmServiceLock_);

        fmServiceSPtr = make_shared<FailoverManagerService>(
            partitionId,
            replicaId,
            instance_,
            *fabricRoot_);

        error = fmServiceSPtr->InitializeForSystemServices(
            config.EnableTStore,
            reliabilitySubsystem_.WorkingDirectory,
            FMStoreDirectory,
            FMSharedLogFilename,
            config,
            partitionId,
            move(replicatorSettings),
            Store::ReplicatedStoreSettings(
                FailoverConfig::GetConfig().CommitBatchingPeriod,
                FailoverConfig::GetConfig().CommitBatchingSizeLimitInKB << 10,
                FailoverConfig::GetConfig().TransactionLowWatermark,
                FailoverConfig::GetConfig().TransactionHighWatermark,
                FailoverConfig::GetConfig().CommitBatchingPeriodExtension,
                FailoverConfig::GetConfig().ThrottleOperationCount,
                FailoverConfig::GetConfig().ThrottleQueueSizeBytes,
                Store::StoreConfig::GetConfig().EnableSystemServiceFlushOnDrain),
            Store::EseLocalStoreSettings(
                FMStoreFilename,
                Path::Combine(reliabilitySubsystem_.WorkingDirectory, FMStoreDirectory),
                FailoverConfig::GetConfig().CompactionThresholdInMB),
            reliabilitySubsystem_.ClientFactory, 
            serviceName,
            initializationData);

        if (!error.IsSuccess()) { return error; }

        fmServiceWPtr_ = fmServiceSPtr;
    }

    auto selfRoot = this->CreateComponentRoot();

    fmServiceSPtr->OnChangeRoleCallback = 
        [this, selfRoot] (::FABRIC_REPLICA_ROLE newRole, Common::ComPointer<IFabricStatefulServicePartition> const & servicePartition)
        {
            this->OnServiceChangeRole(newRole, servicePartition);
        };

    fmServiceSPtr->OnCloseReplicaCallback = 
        [this, selfRoot]()
        {
            this->OnServiceCloseReplica();
        };

    replicaResult = IStatefulServiceReplicaPtr(fmServiceSPtr.get(), fmServiceSPtr->CreateComponentRoot());

    return ErrorCode();
}

Common::ErrorCode FailoverManagerServiceFactory::UpdateReplicatorSecuritySettings(
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
        AcquireExclusiveLock lock(fmServiceLock_);

        auto replica = fmServiceWPtr_.lock();
        if (replica)
        {
            error = replica->UpdateReplicatorSettings(move(replicatorSettings));
        }
    }

    if (error.IsSuccess())
    {
        WriteInfo(
            TraceFactory, 
            "{0}: UpdateReplicatorSettings succeeded, new value = {1}",
            instance_,
            securitySettings);
    }
    else
    {
        WriteError(
            TraceFactory, 
            "{0}: UpdateReplicatorSettings failed, error = {1}, new value = {2}",
            instance_,
            error,
            securitySettings);
    }

    return error;
}

void FailoverManagerServiceFactory::OnServiceChangeRole(FABRIC_REPLICA_ROLE newRole, ComPointer<IFabricStatefulServicePartition> const & servicePartition)
{
    AcquireExclusiveLock lock(fmServiceLock_);
    auto replica = fmServiceWPtr_.lock();
    ASSERT_IF(!replica, "OnServiceChangeRole called even though FMService is null");
    if(newRole == ::FABRIC_REPLICA_ROLE_PRIMARY)
    {
        ASSERT_IF(fm_ != nullptr, "Replica role changed to Primary although FM already exists");

        auto fmStore = make_unique<FailoverManagerStore>(RootedObjectPointer<Store::IReplicatedStore>(
            replica->ReplicatedStore,
            replica->CreateComponentRoot()));

        auto localFederationSPtr = federationSPtr_;

        Api::IServiceManagementClientPtr serviceManagementClient;
        auto error = reliabilitySubsystem_.ClientFactory->CreateServiceManagementClient(serviceManagementClient);

        if (!error.IsSuccess())
        {
            // Do not block opening of FM primary if we can't get the client.
            // Operations that use client should check if it is valid.
            WriteWarning(
                TraceFactory,
                "FM {0} cannot open ServiceManagementClient. Error={1}",
                fm_->Id,
                error);
        }

        fm_ = FailoverManager::CreateFM(
            move(localFederationSPtr),
            reliabilitySubsystem_.HealthClient,
            serviceManagementClient,
            reliabilitySubsystem_.ClientFactory,
            reliabilitySubsystem_.NodeConfig,
            move(fmStore),
            servicePartition,
            replica->PartitionId,
            replica->ReplicaId);
        
        FABRIC_EPOCH epoch;
        ASSERT_IF(!replica->ReplicatedStore->GetCurrentEpoch(epoch).IsSuccess(), "GetCurrentEpoch not successful.");

        fm_->PreOpenFMServiceEpoch = epoch;

        WriteInfo(
            TraceFactory, 
            "FM {0} changed to primary with epoch: {1}",
            fm_->Id, 
            fm_->PreOpenFMServiceEpoch);

        auto selfRoot = this->CreateComponentRoot();
        
        fm_->Open(
            [this, selfRoot] (EventArgs const &) { OnFailoverManagerReady(); },
            [this, selfRoot] (EventArgs const &) { OnFailoverManagerFailed(); });
    }
    else
    {
        if (fm_)
        {
            fm_->Close(newRole != FABRIC_REPLICA_ROLE::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
            fm_ = nullptr;
        }
    }
}

void FailoverManagerServiceFactory::OnServiceCloseReplica()
{
    AcquireExclusiveLock lock(fmServiceLock_);
    auto replica = fmServiceWPtr_.lock();
    ASSERT_IF(!replica, "OnServiceCloseReplica called even though FMService is null");
    if (fm_)
    {
        fm_->Close(true /* isStoreCloseNeeded */);
        fm_ = nullptr;
    }

    fmServiceWPtr_.reset();
}

void FailoverManagerServiceFactory::OnFailoverManagerReady()
{
    fmReadyEvent_.Fire(EventArgs(), true);
}

void FailoverManagerServiceFactory::OnFailoverManagerFailed()
{
    fmFailedEvent_.Fire(EventArgs());
}

void FailoverManagerServiceFactory::Cleanup()
{
    fmFailedEvent_.Close();
    fmReadyEvent_.Close();
    fabricRoot_.reset();
}

IFailoverManagerSPtr FailoverManagerServiceFactory::Test_GetFailoverManager() const
{
    AcquireExclusiveLock lock(fmServiceLock_);
    return fm_;
}

ComponentRootWPtr FailoverManagerServiceFactory::Test_GetFailoverManagerService() const
{
    AcquireExclusiveLock lock(fmServiceLock_);
    return fmServiceWPtr_;
}

bool FailoverManagerServiceFactory::Test_IsFailoverManagerReady() const
{
    AcquireExclusiveLock lock(fmServiceLock_);
    return (fm_ != nullptr && fm_->IsReady);
}
