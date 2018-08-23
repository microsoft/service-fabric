// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ReliabilitySubsystem : 
        public Common::TextTraceComponent<Common::TraceTaskCodes::Reliability>, 
        public IReliabilitySubsystem
    {
        DENY_COPY(ReliabilitySubsystem);

    public:
        ReliabilitySubsystem(ReliabilitySubsystemConstructorParameters &);

        ~ReliabilitySubsystem();

        void InitializeHealthReportingComponent(Client::HealthReportingComponentSPtr const & healthClient);
        void InitializeClientFactory(Api::IClientFactoryPtr const &);

        // todo, remove unnecessary properties and methods after tests are updated to use FabricNode
        // These properties are on IReliabilitySubsystem
        Federation::FederationSubsystem & get_FederationSubsystem() const { return federation_; }
        Reliability::FederationWrapper & get_FederationWrapper() { return federationWrapper_; }
        Reliability::IFederationWrapper & get_FederationWrapperBase()  { return static_cast<IFederationWrapper&>(federationWrapper_); }
        ServiceAdminClient & get_ServiceAdminClient() { return serviceAdminClient_; }
        ServiceResolver & get_ServiceResolver() { return serviceResolver_; }

        __declspec (property(get=get_NodeId)) Federation::NodeId Id;
        Federation::NodeId get_NodeId() const { return federation_.Id; }

        __declspec (property(get=get_Instance)) Federation::NodeInstance const& Instance;
        Federation::NodeInstance const& get_Instance() const { return federation_.Instance; }

        __declspec (property(get=get_ReconfigurationAgent))  ReconfigurationAgentComponent::IReconfigurationAgent & ReconfigurationAgent;
        ReconfigurationAgentComponent::IReconfigurationAgent & get_ReconfigurationAgent() const { return *ra_; }

        __declspec (property(get=get_NodeUpgradeDomainId)) std::wstring const& NodeUpgradeDomainId;
        std::wstring const& get_NodeUpgradeDomainId() const { return nodeUpgradeDomainId_; }

        __declspec (property(get=get_NodeFaultDomainIds)) std::vector<Common::Uri> const& NodeFaultDomainIds;
        std::vector<Common::Uri> const& get_NodeFaultDomainIds() const { return nodeFaultDomainIds_; }

        __declspec (property(get=get_NodePropertyMap)) std::map<std::wstring, std::wstring> const& NodePropertyMap;
        std::map<std::wstring, std::wstring> const& get_NodePropertyMap() const { return nodePropertyMap_; }

        __declspec (property(get=get_NodeCapacityRatioMap)) std::map<std::wstring, uint> const& NodeCapacityRatioMap;
        std::map<std::wstring, uint> get_NodeCapacityRatioMap() const { return nodeCapacityRatioMap_; }

        __declspec (property(get=get_NodeCapacityMap)) std::map<std::wstring, uint> const& NodeCapacityMap;
        std::map<std::wstring, uint> get_NodeCapacityMap() const { return nodeCapacityMap_; }

        __declspec (property(get=get_securitySettings)) Transport::SecuritySettings const& SecuritySettings;
        Transport::SecuritySettings const& get_securitySettings() const;
        Common::ErrorCode SetSecurity(Transport::SecuritySettings const& value);

        Client::HealthReportingComponentSPtr const & get_HealthClient() const { return healthClient_; }
        Api::IClientFactoryPtr const & get_ClientFactory() const { return clientFactory_; }

        Common::HHandler RegisterFailoverManagerReadyEvent(Common::EventHandler const & handler);
        bool UnRegisterFailoverManagerReadyEvent(Common::HHandler hHandler);

        Common::HHandler RegisterFailoverManagerFailedEvent(Common::EventHandler const & handler);
        bool UnRegisterFailoverManagerFailedEvent(Common::HHandler hHandler);

        FailoverManagerComponent::IFailoverManagerSPtr Test_GetFailoverManager() const;
        FailoverManagerComponent::IFailoverManagerSPtr Test_GetFailoverManagerMaster() const;

        bool Test_IsFailoverManagerReady() const;
        Store::ILocalStore & Test_GetLocalStore() override;
        Common::ComponentRootWPtr Test_GetFailoverManagerService() const override;

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        void SendNodeUpToFM();

        void SendNodeUpToFMM();

        void UploadLFUM(
            Reliability::GenerationNumber const & generation,
            std::vector<FailoverUnitInfo> && failoverUnitInfo,
            bool anyReplicaFound);

        void UploadLFUMForFMReplicaSet(
            Reliability::GenerationNumber const & generation,
            std::vector<FailoverUnitInfo> && failoverUnitInfo,
            bool anyReplicaFound);

        NodeDescription GetNodeDescription() const;

        void ProcessNodeUpAck(Transport::MessageUPtr && nodeUpReply, bool isFromFMM);
        void ProcessLFUMUploadReply(Reliability::GenerationHeader const & generationHeader);

    protected:
        //FabricComponent members
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        Federation::FederationSubsystem & federation_;
        std::wstring traceId_;
        Transport::IpcServer & ipcServer_;
        Hosting2::IHostingSubsystem & hostingSubsystem_;
        Hosting2::IRuntimeSharingHelper & runtimeSharingHelper_;
        Common::ComPointer<Api::ComStatefulServiceFactory> serviceFactoryCPtr_;

        Reliability::FederationWrapper federationWrapper_;

        ServiceAdminClient serviceAdminClient_;
        ServiceResolver serviceResolver_;

        std::wstring const runtimeServiceListenAddress_;
        ReconfigurationAgentComponent::IReconfigurationAgentUPtr ra_;

        Common::HHandler routingTokenChangedHHandler_;

        Common::Event fmReadyEvent_;
        Common::Event fmFailedEvent_;

        ServiceUpdateEvent::HHandler fmUpdateHHandler_;

        std::wstring nodeUpgradeDomainId_;
        std::vector<Common::Uri> nodeFaultDomainIds_;
        std::map<std::wstring, std::wstring> nodePropertyMap_;
        std::map<std::wstring, uint> nodeCapacityRatioMap_;
        std::map<std::wstring, uint> nodeCapacityMap_;
        std::wstring replicatorAddress_;
        Transport::SecuritySettings securitySettings_;

        std::shared_ptr<FailoverManagerComponent::FailoverManagerServiceFactory> serviceFactorySPtr_;

        Client::HealthReportingComponentSPtr healthClient_;
        Api::IClientFactoryPtr clientFactory_;

        // Stores data from the LFUM when the RA is open
        // Used to send NodeUp
        std::unique_ptr<const NodeUpOperationFactory> nodeUpOperationFactory_;

        // The lock guard the state below it
        mutable Common::RwLock lockObject_;
        bool isOpened_;
        FailoverManagerComponent::IFailoverManagerSPtr fm_;
        // Saved copy of the GFUM to be transferred to the new FMM.
        SerializedGFUMUPtr savedGFUM_;

        std::weak_ptr<ChangeNotificationOperation> lastSentChangeNotificationOperationFM_;
        std::weak_ptr<ChangeNotificationOperation> lastSentChangeNotificationOperationFMM_;

        void RegisterFederationSubsystemEvents();
        void RegisterRAMessageHandlers();
        void RegisterHostingSubsystemEvents();

        void UnRegisterRAMessageHandlers();
        void UnRegisterFederationSubsystemEvents();

        void UploadLFUMHelper(
            Reliability::GenerationNumber const & generation,
            std::vector<FailoverUnitInfo> && failoverUnitInfo,
            bool isToFMM,
            bool anyReplicaFound);

        void OnFailoverManagerReady();
        void OnFailoverManagerFailed();

        void RouteCallback(Common::AsyncOperationSPtr const& routeOperation,
            std::wstring const & action,
            Federation::NodeInstance target) const;
        void RaOneWayMessageHandler(Transport::Message & message, Federation::OneWayReceiverContext& oneWayReceiverContext);
        void RaRequestMessageHandler(Transport::MessageUPtr & messagePtr, Federation::RequestReceiverContextUPtr & requestReceiverContextPtr);

        void RaRequestMessageHandler_Threadpool(Transport::Message & message, Federation::RequestReceiverContext& requestReceiverContext);
        

        void LookupTableUpdateRequestMessageHandler(Transport::Message & message);

        void ServiceTypeRegisteredHandler(Hosting2::ServiceTypeRegistrationEventArgs const& eventArgs);
        void RuntimeClosedHandler(Hosting2::RuntimeClosedEventArgs const& eventArgs);
        void ApplicationHostClosedHandler(Hosting2::ApplicationHostClosedEventArgs const& eventArgs);
        void ServiceTypeDisabledHandler(Hosting2::ServiceTypeStatusEventArgs const & eventArgs);
        void ServiceTypeEnabledHandler(Hosting2::ServiceTypeStatusEventArgs const & eventArgs);

        // Handle event to send available images on node to the FM
        void SendAvailableContainerImagesHandler(Hosting2::SendAvailableContainerImagesEventArgs const& eventArgs);

        bool IsFMM() const;

        void Cleanup();
        void CleanupServiceFactory();
        void OnRoutingTokenChanged();
        void CreateFailoverManager();

        //we use this to change ServiceFabric to servicefabric rg metric prefix
        //other metrics or rg metrics with correct prefix are just copied
        void PopulateMetricsAdjustRG(std::map<std::wstring, uint> & capacitiesAdjust, std::map<std::wstring, uint> const & capacitiesBase) const;

        // check and read node capacities from LRM
        void RefreshNodeCapacity();
        void ProcessRGMetrics(std::wstring const& resourceName, uint64 resourceCapacity);
    };
}
