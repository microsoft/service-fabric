// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    struct Test_ActiveServiceData
    {
        DENY_COPY_ASSIGNMENT(Test_ActiveServiceData);
        DEFAULT_COPY_CONSTRUCTOR(Test_ActiveServiceData);

    public:
        Test_ActiveServiceData()
        {
        }

        Test_ActiveServiceData(std::wstring const & partitionId, ::FABRIC_REPLICA_ID replicaId, ::FABRIC_REPLICA_ROLE replicaRole)
            : partitionId_(partitionId), replicaId_(replicaId), replicaRole_(replicaRole)
        {
        }

        Test_ActiveServiceData(Test_ActiveServiceData && rhs)
            : partitionId_(std::move(rhs.partitionId_)), replicaId_(rhs.replicaId_), replicaRole_(rhs.replicaRole_)
        {
        }

        Test_ActiveServiceData & operator =(Test_ActiveServiceData && rhs)
        {
            if (this != &rhs)
            {
                partitionId_ = std::move(rhs.partitionId_);
                replicaId_ = rhs.replicaId_;
                replicaRole_ = rhs.replicaRole_;
            }
            return *this;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write("{0},{1},{2}", partitionId_, replicaId_, static_cast<int>(replicaRole_));
        }

        std::wstring partitionId_;
        ::FABRIC_REPLICA_ID replicaId_;
        ::FABRIC_REPLICA_ROLE replicaRole_;
    };

    typedef std::map<std::wstring, Test_ActiveServiceData> ActiveServiceMap;

    class NamingService : 
        public Common::AsyncFabricComponent,
        private Common::RootedObject
    {
    public:

        ~NamingService();

        static NamingServiceUPtr Create(
            Reliability::FederationWrapper &,
            Hosting2::IRuntimeSharingHelper & runtime,
            Reliability::ServiceAdminClient &,
            Reliability::ServiceResolver &,
            std::wstring const & entreeServiceListenAddress,
            std::wstring const & replicatorAddress,
            Transport::SecuritySettings const& clusterSecuritySettings,
            std::wstring const & workingDir,
            Common::FabricNodeConfigSPtr const& nodeConfig,
            Hosting2::IFabricActivatorClientSPtr const & activatorClient,
            Common::ComponentRoot const &);

        __declspec(property(get=get_Gateway)) IGateway & Gateway;
        IGateway & get_Gateway() const;

        __declspec(property(get=get_NamingServiceCuids)) std::vector<Reliability::ConsistencyUnitId> const & NamingServiceCuids;
        std::vector<Reliability::ConsistencyUnitId> const & get_NamingServiceCuids() const;

        __declspec(property(get=Test_get_ActiveServices)) ActiveServiceMap const & Test_ActiveServices;
        ActiveServiceMap const & Test_get_ActiveServices() const;

        __declspec (property(get=Test_get_EntreeServiceListenAddress)) std::wstring const& Test_EntreeServiceListenAddress;
        std::wstring const& Test_get_EntreeServiceListenAddress() const;

        static Common::ErrorCode CreateSystemServiceDescriptionFromTemplate(
            Reliability::ServiceDescription const & svcTemplate,
            __out Reliability::ServiceDescription & svcResult);

        static Common::ErrorCode CreateSystemServiceDescriptionFromTemplate(
            Common::ActivityId const &,
            Reliability::ServiceDescription const & svcTemplate,
            __out Reliability::ServiceDescription & svcResult);

        Common::AsyncOperationSPtr BeginCreateService(
            Reliability::ServiceDescription const &,
            std::vector<Reliability::ConsistencyUnitDescription>,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndCreateService(Common::AsyncOperationSPtr const &);

        Common::ErrorCode SetClusterSecurity(Transport::SecuritySettings const & securitySettings);
        Common::ErrorCode SetNamingGatewaySecurity(Transport::SecuritySettings const & securitySettings);

        Naming::INamingMessageProcessorSPtr GetGateway();
        void InitializeClientFactory(Api::IClientFactoryPtr const &);

    protected:
        Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &) override;

        void OnAbort();

    private:
        NamingService(            
            Reliability::FederationWrapper &,
            Hosting2::IRuntimeSharingHelper & runtimeSharingHelper,
            Reliability::ServiceAdminClient &,
            Reliability::ServiceResolver &,
            std::wstring const & entreeServiceListenAddress,
            std::wstring const & replicatorAddress,
            Transport::SecuritySettings const& clusterSecuritySettings,
            std::wstring const & workingDir,
            Common::FabricNodeConfigSPtr const& nodeConfig,
            Hosting2::IFabricActivatorClientSPtr const & activatorClient,
            Common::ComponentRoot const &);

        class NamingServiceImpl;
        std::unique_ptr<NamingServiceImpl> impl_;
    };
}
