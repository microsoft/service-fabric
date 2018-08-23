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

namespace Management
{
    namespace ClusterManager
    {
        StringLiteral const TraceComponent("Factory");

        ClusterManagerFactory::ClusterManagerFactory(
            __in Reliability::FederationWrapper & federation, 
            __in Reliability::ServiceResolver & serviceResolver,
            std::wstring const & clientConnectionAddress,
            Api::IClientFactoryPtr const & clientFactory,
            std::wstring const & replicatorAddress,
            std::wstring const & workingDir,
            std::wstring const & imageBuilderPath,
            std::wstring const & nodeName,
            Transport::SecuritySettings const& clusterSecuritySettings,
            ComponentRoot const & fabricRoot)
            : NodeTraceComponent(federation.Instance) 
            , fabricRoot_(fabricRoot.CreateComponentRoot())
            , federation_(federation)
            , serviceResolver_(serviceResolver)
            , clientConnectionAddress_(clientConnectionAddress)
            , clientFactory_(clientFactory)
            , workingDir_(workingDir)
            , nodeName_(nodeName)
            , imageBuilder_()
            , cmServiceLock_()
            , replicatorAddress_(replicatorAddress)
            , clusterSecuritySettings_(clusterSecuritySettings)
            , cmServiceWPtr_()
        {
            WriteNoise(
                TraceComponent, 
                "{0} ctor: this = {1}",
                this->TraceId,
                static_cast<void*>(this));                   

            if (ManagementConfig::GetConfig().TestComposeDeploymentTestMode)
            {
                imageBuilder_ = TestDockerComposeImageBuilderProxy::Create(
                    Environment::GetExecutablePath(),
                    imageBuilderPath,
                    nodeName_,
                    federation.Instance);
            }
            else
            {
                imageBuilder_ = ImageBuilderProxy::Create(
                    Environment::GetExecutablePath(),
                    imageBuilderPath,
                    nodeName_,
                    federation.Instance);
            }
        }

        ClusterManagerFactory::~ClusterManagerFactory()
        {
            WriteNoise(
                TraceComponent, 
                "{0} ~dtor: this = {1}",
                this->TraceId,
                static_cast<void*>(this));
        }

        ErrorCode ClusterManagerFactory::CreateReplica(
            std::wstring const & serviceType,
            Common::NamingUri const & serviceName,
            std::vector<byte> const & initializationData, 
            Common::Guid const & partitionId,
            FABRIC_REPLICA_ID replicaId,
            __out IStatefulServiceReplicaPtr & replicaResult)
        {
            ASSERT_IF(
                serviceType != ServiceModel::ServiceTypeIdentifier::ClusterManagerServiceTypeId->ServiceTypeName, 
                "ClusterManagerFactory cannot create service of type '{0}'", 
                serviceType);

            ASSERT_IF(
                serviceName.ToString() != ServiceModel::SystemServiceApplicationNameHelper::PublicClusterManagerServiceName, 
                "ClusterManagerFactory cannot create service '{0}'", 
                serviceName);

            ManagementConfig & config = ManagementConfig::GetConfig();
            ReplicationComponent::ReplicatorSettingsUPtr replicatorSettings;

            auto error = ReplicationComponent::ReplicatorSettings::FromConfig(
                config,
                replicatorAddress_,
                replicatorAddress_,
                replicatorAddress_,
                clusterSecuritySettings_,
                replicatorSettings);
            if (!error.IsSuccess()) { return error; }
            
            shared_ptr<ClusterManagerReplica> cmReplicaSPtr;
            {
                AcquireWriteLock lock(cmServiceLock_);

                cmReplicaSPtr = make_shared<ClusterManagerReplica>(
                    partitionId,
                    replicaId,
                    federation_, 
                    serviceResolver_,
                    *imageBuilder_,
                    clientFactory_,
                    workingDir_,
                    nodeName_,
                    *(this->FabricRoot));

                error = cmReplicaSPtr->InitializeForSystemServices(
                    config.EnableTStore,
                    workingDir_,
                    Constants::DatabaseDirectory,
                    Constants::SharedLogFilename,
                    config,
                    partitionId,
                    move(replicatorSettings),
                    Store::ReplicatedStoreSettings(
                        ManagementConfig::GetConfig().HealthStoreCommitBatchingPeriod,        
                        ManagementConfig::GetConfig().HealthStoreCommitBatchingSizeLimitInKB << 10,
                        ManagementConfig::GetConfig().HealthStoreTransactionLowWatermark,
                        ManagementConfig::GetConfig().HealthStoreTransactionHighWatermark,
                        ManagementConfig::GetConfig().HealthStoreCommitBatchingPeriodExtension,
                        ManagementConfig::GetConfig().HealthStoreThrottleOperationCount,
                        ManagementConfig::GetConfig().HealthStoreThrottleQueueSizeBytes,
                        StoreConfig::GetConfig().EnableSystemServiceFlushOnDrain),
                    Store::EseLocalStoreSettings(
                        Constants::DatabaseFilename,
                        Path::Combine(workingDir_, Constants::DatabaseDirectory),
                        ManagementConfig::GetConfig().CompactionThresholdInMB),
                    clientFactory_,
                    serviceName,
                    initializationData);

                if (!error.IsSuccess()) { return error; }

                cmServiceWPtr_ = cmReplicaSPtr;
            }

            // Keep this factory alive for the replica callbacks even after 
            // FabricNode close (FabricNode->ManagementSubsystem will release 
            // the ComPointer)
            //
            auto selfRoot = this->CreateComponentRoot();

            cmReplicaSPtr->OnChangeRoleCallback = 
            [this, selfRoot] (
                SystemServiceLocation const & serviceLocation, 
                Test_ReplicaRef const & service)
            {
                this->OnServiceChangeRole(serviceLocation, service);
            };

            cmReplicaSPtr->OnCloseReplicaCallback = 
            [this, selfRoot](SystemServiceLocation const & serviceLocation)
            {
                this->OnServiceCloseReplica(serviceLocation);
            };
            
            replicaResult = IStatefulServiceReplicaPtr(cmReplicaSPtr.get(), cmReplicaSPtr->CreateComponentRoot());

            return ErrorCode();
        }

        Common::ErrorCode ClusterManagerFactory::UpdateReplicatorSecuritySettings(
            Transport::SecuritySettings const & securitySettings)
        {
            ReplicationComponent::ReplicatorSettingsUPtr replicatorSettings;

            auto error = ReplicationComponent::ReplicatorSettings::FromSecuritySettings(
                securitySettings,
                replicatorSettings);

            if (!error.IsSuccess())
            {
                return error;
            }

            {
                AcquireExclusiveLock lock(cmServiceLock_);
                clusterSecuritySettings_ = securitySettings;

                auto replica = cmServiceWPtr_.lock();
                if (replica)
                {
                    error = replica->UpdateReplicatorSettings(replicatorSettings);
                }
            }

            if (error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent, 
                    "{0}({1}) UpdateReplicatorSecuritySettings succeeded, new value = {2}",
                    this->TraceId,
                    TextTraceThis,
                    securitySettings);
            }
            else
            {
                WriteError(
                    TraceComponent, 
                    "{0}({1}) UpdateReplicatorSecuritySettings failed: error = {2}, new value = {3}",
                    this->TraceId,
                    TextTraceThis,
                    error,
                    securitySettings);
            }

            return error;
        }

        void ClusterManagerFactory::UpdateClientSecuritySettings(SecuritySettings const & securitySettings)
        {
            imageBuilder_->UpdateSecuritySettings(securitySettings);
        }

        void ClusterManagerFactory::OnServiceChangeRole(
            SystemServiceLocation const & serviceLocation, 
            Test_ReplicaRef const & service)
        {
            AcquireWriteLock lock(activeServicesLock_);
            activeServices_[serviceLocation.Location] = service;
        }

        void ClusterManagerFactory::OnServiceCloseReplica(SystemServiceLocation const & serviceLocation)
        {
            {
                AcquireWriteLock lock(cmServiceLock_);
                cmServiceWPtr_.reset();
            }

            AcquireWriteLock lock(activeServicesLock_);
            activeServices_.erase(serviceLocation.Location);
        }
    }
}
