// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService;

    class StoreServiceFactory
        : public Api::IStatefulServiceFactory
        , public Common::TextTraceComponent<Common::TraceTaskCodes::NamingStoreService>
        , public Common::ComponentRoot
    {
        DENY_COPY(StoreServiceFactory);

    public:
        StoreServiceFactory(
            __in Reliability::FederationWrapper &, 
            __in Reliability::ServiceAdminClient &,
            __in Reliability::ServiceResolver &,
            Api::IClientFactoryPtr const &,
            std::wstring const & replicatorAddress,
            Transport::SecuritySettings const&,
            std::wstring const & workingDir,
            Common::ComponentRoot const & fabricRoot);

        ~StoreServiceFactory();

        __declspec(property(get=Test_get_ActiveServices)) ActiveServiceMap const & Test_ActiveServices;
        ActiveServiceMap const & Test_get_ActiveServices() const { return activeServices_; }

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

    public:

        Common::ErrorCode UpdateReplicatorSecuritySettings(
            Transport::SecuritySettings const & securitySettings);

    private:

        __declspec (property(get=get_FabricRoot)) Common::ComponentRootSPtr const & FabricRoot;
        Common::ComponentRootSPtr const & get_FabricRoot() const { return fabricRoot_; }

        void OnServiceChangeRole(SystemServices::SystemServiceLocation const &, Common::Guid const &, ::FABRIC_REPLICA_ID, ::FABRIC_REPLICA_ROLE);
        void OnServiceCloseReplica(SystemServices::SystemServiceLocation const &, std::shared_ptr<StoreService> const &);

        // Root the FederationSubsystem since the factory itself is 
        // actually part of the service runtime object hierarchy
        //
        Common::ComponentRootSPtr fabricRoot_;

        Common::RwLock storeServicesLock_;
        std::map<Common::Guid, std::weak_ptr<StoreService>> storeServicesWPtrMap_;

        Reliability::FederationWrapper & federation_;
        Reliability::ServiceAdminClient & adminClient_;
        Reliability::ServiceResolver & serviceResolver_;
        Api::IClientFactoryPtr const & clientFactory_;
        std::wstring workingDir_;
        std::wstring const replicatorAddress_;
        Transport::SecuritySettings clusterSecuritySettings_;
        
        ActiveServiceMap activeServices_;
        Common::RwLock activeServicesLock_;
    };
};
