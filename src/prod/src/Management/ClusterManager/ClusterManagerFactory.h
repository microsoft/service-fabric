// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class IImageBuilder;
        class ClusterManagerReplica;

        using Test_ReplicaRef = std::shared_ptr<ClusterManagerReplica>;
        using Test_ReplicaMap = std::map<std::wstring, Test_ReplicaRef>;

        class ClusterManagerFactory 
            : public Api::IStatefulServiceFactory
            , public Common::ComponentRoot
            , public Federation::NodeTraceComponent<Common::TraceTaskCodes::ClusterManager>
        {
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::ClusterManager>::TraceId;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::ClusterManager>::get_TraceId;

            DENY_COPY(ClusterManagerFactory);

        public:
            ClusterManagerFactory(
                __in Reliability::FederationWrapper &, 
                __in Reliability::ServiceResolver &,
                std::wstring const & clientConnectionAddress,
                Api::IClientFactoryPtr const &, 
                std::wstring const & replicatorAddress,
                std::wstring const & workingDir,
                std::wstring const & imageBuilderPath,
                std::wstring const & nodeName,
                Transport::SecuritySettings const& clusterSecuritySettings,
                Common::ComponentRoot const &);

            virtual ~ClusterManagerFactory();

            __declspec(property(get=Test_get_ActiveServices)) Test_ReplicaMap const & Test_ActiveServices;
            Test_ReplicaMap const & Test_get_ActiveServices() const { return activeServices_; }

            __declspec (property(get=get_FabricRoot)) Common::ComponentRootSPtr const & FabricRoot;
            Common::ComponentRootSPtr const & get_FabricRoot() const { return fabricRoot_; }

            __declspec (property(get=get_ImageBuilder)) IImageBuilder const & ImageBuilder;
            IImageBuilder const & get_ImageBuilder() const { return *imageBuilder_; }

            Common::ErrorCode UpdateReplicatorSecuritySettings(
                Transport::SecuritySettings const &);

            void UpdateClientSecuritySettings(
                Transport::SecuritySettings const &);

        public:

            //
            // IStatefulServiceFactory
            //
            virtual Common::ErrorCode CreateReplica(
                std::wstring const & serviceType,
                Common::NamingUri const & serviceName,
                std::vector<byte> const & initializationData, 
                Common::Guid const & partitionId,
                FABRIC_REPLICA_ID replicaId,
                __out Api::IStatefulServiceReplicaPtr & serviceReplica) override;

        private:
            void OnServiceChangeRole(SystemServices::SystemServiceLocation const &, Test_ReplicaRef const &);
            void OnServiceCloseReplica(SystemServices::SystemServiceLocation const &);

            // FabricNode must be kept alive since we hold references to its ReliabilitySubsystem
            // and FederationSubsystem for resolving+routing. Note that this factory is kept alive by FabricNode
            // through a ComPointer and that FabricNode may release that ComPointer at any time (i.e. on close).
            // Therefore, this factory must also be a ComponentRoot and keep itself alive as needed.
            //
            Common::ComponentRootSPtr fabricRoot_;

            Reliability::FederationWrapper & federation_;
            Reliability::ServiceResolver & serviceResolver_;
            std::wstring clientConnectionAddress_;
            std::wstring workingDir_;
            std::wstring nodeName_;
            std::wstring const replicatorAddress_;
            Transport::SecuritySettings clusterSecuritySettings_;

            Common::RwLock cmServiceLock_;
            std::weak_ptr<ClusterManagerReplica> cmServiceWPtr_;

            // Lifetime is managed by ManagementSubsystem
            //
            Api::IClientFactoryPtr const & clientFactory_;

            std::shared_ptr<IImageBuilder> imageBuilder_;
            Test_ReplicaMap activeServices_;
            Common::RwLock activeServicesLock_;
        };
    }
}
