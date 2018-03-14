// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class FileStoreServiceReplica;
        class FileStoreServiceFactory
            : public Api::IStatefulServiceFactory
            , public Common::ComponentRoot
            , public Federation::NodeTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::FileStoreService>::TraceId;
            using Federation::NodeTraceComponent<Common::TraceTaskCodes::FileStoreService>::get_TraceId;

            DENY_COPY(FileStoreServiceFactory);

        public:
            FileStoreServiceFactory(
                Common::ComPointer<IFabricRuntime> const & fabricRuntime,
                Common::FabricNodeConfigSPtr const & fabricNodeConfig,
                std::wstring const & workingDir,
                std::wstring const & ipAddressOrFQDN,
                std::shared_ptr<Common::ManualResetEvent> const & exitServiceHostEvent);
            ~FileStoreServiceFactory();

            Common::ErrorCode Initialize();

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
            Common::ErrorCode LoadClusterSecuritySettings(__out Transport::SecuritySettings & clusterSecuritySettings);
            void RegisterForConfigUpdate();
            Common::ErrorCode UpdateReplicatorSecuritySettings(Transport::SecuritySettings const & securitySettings);
            static void ClusterSecuritySettingUpdateHandler(std::weak_ptr<Common::ComponentRoot> const & rootWPtr);

            Common::ErrorCode InitializeRoutingAgentProxy();

            void OnServiceCloseReplica(Common::Guid const & partitionId);

        private:
            Common::ComPointer<IFabricRuntime> fabricRuntimeCPtr_;
            Common::FabricNodeConfigSPtr fabricNodeConfig_;
            std::wstring const workingDir_;            
            std::wstring replicatorAddress_;            
            Api::IClientFactoryPtr clientFactory_;
            std::shared_ptr<Common::ManualResetEvent> exitServiceHostEvent_;
            Transport::SecuritySettings clusterSecuritySettings_;
            SystemServices::ServiceRoutingAgentProxyUPtr routingAgentProxyUPtr_;
            Federation::NodeInstance nodeInstance_;
            std::wstring nodeName_;
            std::wstring ipAddressOrFQDN_;

            std::map<Common::Guid, std::weak_ptr<FileStoreServiceReplica>> replicaMap_;
            Common::RwLock lock_;            
        };
    }
}
