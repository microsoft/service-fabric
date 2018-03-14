// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FailoverManagerServiceFactory : 
            public Api::IStatefulServiceFactory,
            public Common::TextTraceComponent<Common::TraceTaskCodes::FM>,
            public Common::ComponentRoot
        {
            DENY_COPY(FailoverManagerServiceFactory);

        private:

            static Common::GlobalWString FMStoreDirectory;
            static Common::GlobalWString FMStoreFilename;
            static Common::GlobalWString FMSharedLogFilename;

        public:
            FailoverManagerServiceFactory(
                __in Reliability::IReliabilitySubsystem & reliabilitySubsystem,
                Common::EventHandler const & readyEvent, 
                Common::EventHandler const & failureEvent,
                Common::ComponentRoot const & root);

            ~FailoverManagerServiceFactory();

            Common::ErrorCode UpdateReplicatorSecuritySettings(
                Transport::SecuritySettings const & securitySettings);

            FailoverManagerComponent::IFailoverManagerSPtr Test_GetFailoverManager() const;
            Common::ComponentRootWPtr Test_GetFailoverManagerService() const;
            bool Test_IsFailoverManagerReady() const;

            //Indicates to the Factory that the ReliabilitySubsystem is done and it 
            //unregister the events
            void Cleanup();

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
            Federation::FederationSubsystemSPtr federationSPtr_;
            Federation::NodeInstance instance_;

            mutable Common::ExclusiveLock fmServiceLock_;
            std::weak_ptr<FailoverManagerService> fmServiceWPtr_;
            FailoverManagerSPtr fm_;

            Common::Event fmReadyEvent_;
            Common::Event fmFailedEvent_;

            Common::ComponentRootSPtr fabricRoot_;
            Reliability::IReliabilitySubsystem & reliabilitySubsystem_;
            
            void OnServiceChangeRole(::FABRIC_REPLICA_ROLE newRole, ComPointer<IFabricStatefulServicePartition> const & servicePartition);
            void OnServiceCloseReplica();

            void OnFailoverManagerReady();
            void OnFailoverManagerFailed();
        };
    }
}
