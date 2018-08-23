// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace std;
using namespace Store;
using namespace SystemServices;
using namespace Transport;

namespace Naming
{
    StringLiteral const TraceComponent("Factory");

    StoreServiceFactory::StoreServiceFactory(
        __in Reliability::FederationWrapper & federation, 
        __in Reliability::ServiceAdminClient & adminClient,
        __in Reliability::ServiceResolver & serviceResolver,
        Api::IClientFactoryPtr const & clientFactory,
        std::wstring const & replicatorAddress,
        SecuritySettings const& securitySettings,
        std::wstring const & workingDir,
        ComponentRoot const & fabricRoot)
        : ComponentRoot() 
        , fabricRoot_(fabricRoot.CreateComponentRoot())
        , storeServicesLock_()
        , storeServicesWPtrMap_()
        , federation_(federation)
        , adminClient_(adminClient)
        , serviceResolver_(serviceResolver)
        , clientFactory_(clientFactory)
        , workingDir_(workingDir)
        , replicatorAddress_(replicatorAddress)
        , clusterSecuritySettings_(securitySettings)
        , activeServices_()
        , activeServicesLock_()
    {
        WriteNoise(
            TraceComponent, 
            "{0} StoreServiceFactory::ctor: node instance = {1}",
            static_cast<void*>(this),
            federation.Instance);
    }

    StoreServiceFactory::~StoreServiceFactory()
    {
        WriteNoise(
            TraceComponent, 
            "{0} StoreServiceFactory::~dtor",
            static_cast<void*>(this));
    }

    ErrorCode StoreServiceFactory::CreateReplica(
        wstring const & serviceType,
        NamingUri const & serviceName,
        vector<byte> const & initializationData, 
        Guid const & partitionId,
        FABRIC_REPLICA_ID replicaId,
        __out Api::IStatefulServiceReplicaPtr & serviceReplica)
    {
        ASSERT_IF(serviceType != ServiceModel::ServiceTypeIdentifier::NamingStoreServiceTypeId->ServiceTypeName, "StoreServiceFactory cannot create service of type '{0}'", serviceType);
        ASSERT_IF(serviceName.ToString() != ServiceModel::SystemServiceApplicationNameHelper::PublicNamingServiceName, "StoreServiceFactory cannot create service '{0}'", serviceName);

        NamingConfig & config = NamingConfig::GetConfig();
        ReplicationComponent::ReplicatorSettingsUPtr replicatorSettings;

        auto error = ReplicationComponent::ReplicatorSettings::FromConfig(
            config,
            replicatorAddress_,
            replicatorAddress_,
            replicatorAddress_,
            clusterSecuritySettings_,
            replicatorSettings);
        if (!error.IsSuccess()) { return error; }

        shared_ptr<StoreService> storeServiceSPtr;
        {
            AcquireWriteLock lock(storeServicesLock_);

            storeServiceSPtr = make_shared<StoreService>(
                partitionId,
                replicaId,
                federation_.Federation.Instance,
                federation_,
                adminClient_,
                serviceResolver_,
                *(this->FabricRoot));

            error = storeServiceSPtr->InitializeForSystemServices(
                config.EnableTStore,
                workingDir_,
                Constants::NamingStoreDirectory,
                Constants::NamingStoreSharedLogFilename,
                config,
                //
                // Naming Service partitions will share a common shared log
                //
                Reliability::ConsistencyUnitId::NamingIdRange->CreateReservedGuid(0),
                move(replicatorSettings),
                Store::ReplicatedStoreSettings(
                    Store::StoreConfig::GetConfig().EnableSystemServiceFlushOnDrain),
                Store::EseLocalStoreSettings(
                    Constants::NamingStoreFilename,
                    Path::Combine(workingDir_, Constants::NamingStoreDirectory),
                    NamingConfig::GetConfig().CompactionThresholdInMB),
                clientFactory_,
                serviceName,
                initializationData);

            if (!error.IsSuccess()) { return error; }

            storeServicesWPtrMap_.erase(partitionId);
            storeServicesWPtrMap_[partitionId] = storeServiceSPtr;
        }

        auto selfRoot = this->CreateComponentRoot();

        storeServiceSPtr->OnChangeRoleCallback =
            [this, selfRoot](
            SystemServiceLocation const & serviceLocation,
            Guid const & partitionId,
            ::FABRIC_REPLICA_ID replicaId,
            ::FABRIC_REPLICA_ROLE replicaRole)
        {
            this->OnServiceChangeRole(serviceLocation, partitionId, replicaId, replicaRole);
        };

        storeServiceSPtr->OnCloseReplicaCallback =
            [this, selfRoot](SystemServiceLocation const & serviceLocation, std::shared_ptr<StoreService> const & storeService)
        {
            this->OnServiceCloseReplica(serviceLocation, storeService);
        };

        serviceReplica = Api::IStatefulServiceReplicaPtr(storeServiceSPtr.get(), storeServiceSPtr->CreateComponentRoot());

        return ErrorCodeValue::Success;
    }

    Common::ErrorCode StoreServiceFactory::UpdateReplicatorSecuritySettings(
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
            AcquireExclusiveLock lock(storeServicesLock_);
            clusterSecuritySettings_ = securitySettings;

            for (auto & item : storeServicesWPtrMap_)
            {
                auto replica = item.second.lock();
                auto clonedReplicatorSettings = replicatorSettings->Clone();

                if (replica)
                {
                    error = replica->UpdateReplicatorSettings(move(clonedReplicatorSettings));
                    if (!error.IsSuccess())
                    {
                        WriteError(
                            TraceComponent,
                            "{0} StoreServiceFactory::UpdateReplicatorSecuritySettings failed: error={1}, new value = {2}",
                            TextTraceThis,
                            error,
                            securitySettings);
                    }
                    else
                    {
                        WriteInfo(
                            TraceComponent,
                            "{0} StoreServiceFactory::UpdateReplicatorSecuritySettings succeeded, new value = {1}",
                            TextTraceThis,
                            securitySettings);
                    }
                }
            }
        }

        return error;
    }

    void StoreServiceFactory::OnServiceChangeRole(
        SystemServiceLocation const & serviceLocation, 
        Guid const & partitionId,
        ::FABRIC_REPLICA_ID replicaId,
        ::FABRIC_REPLICA_ROLE replicaRole)
    {
        AcquireWriteLock lock(activeServicesLock_);
        activeServices_[serviceLocation.Location] = Test_ActiveServiceData(partitionId.ToString(), replicaId, replicaRole);
    }

    void StoreServiceFactory::OnServiceCloseReplica(
        SystemServiceLocation const & serviceLocation,
        std::shared_ptr<StoreService> const & storeService)
    {
        {
            AcquireWriteLock lock(storeServicesLock_);
            storeServicesWPtrMap_.erase(storeService->PartitionId);
        }

        AcquireWriteLock lock(activeServicesLock_);
        activeServices_.erase(serviceLocation.Location);
    }
}
