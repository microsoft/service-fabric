// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class HostingSubsystem :
        public Common::RootedObject,
        public IHostingSubsystem,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(HostingSubsystem)

    public:
        HostingSubsystem(
            Common::ComponentRoot const & root,
            Federation::FederationSubsystem & federation,
            std::wstring const & nodeWorkingDirectory,
            __in Common::FabricNodeConfigSPtr const & fabricNodeConfig,
            __in Transport::IpcServer & ipcServer,
            bool serviceTypeBlocklistingEnabled,
            std::wstring const & deploymentFolder,
            __in KtlLogger::KtlLoggerNodeSPtr const & ktlLoggerNode);
        virtual ~HostingSubsystem();

        ServiceTypeRegisteredEventHHandler RegisterServiceTypeRegisteredEventHandler(ServiceTypeRegisteredEventHandler const & handler);
        bool UnregisterServiceTypeRegisteredEventHandler(ServiceTypeRegisteredEventHHandler const & hHandler);

        ServiceTypeDisabledEventHHandler RegisterServiceTypeDisabledEventHandler(ServiceTypeDisabledEventHandler const & handler);
        bool UnregisterServiceTypeDisabledEventHandler(ServiceTypeDisabledEventHHandler const & hHandler);

        ServiceTypeEnabledEventHHandler RegisterServiceTypeEnabledEventHandler(ServiceTypeEnabledEventHandler const & handler);
        bool UnregisterServiceTypeEnabledEventHandler(ServiceTypeEnabledEventHHandler const & hHandler);

        RuntimeClosedEventHHandler RegisterRuntimeClosedEventHandler(RuntimeClosedEventHandler const & handler);
        bool UnregisterRuntimeClosedEventHandler(RuntimeClosedEventHHandler const & hHandler);

        ApplicationHostClosedEventHHandler RegisterApplicationHostClosedEventHandler(ApplicationHostClosedEventHandler const & handler);
        bool UnregisterApplicationHostClosedEventHandler(ApplicationHostClosedEventHHandler const & hHandler);

        AvailableContainerImagesEventHHandler RegisterSendAvailableContainerImagesEventHandler(AvailableContainerImagesEventHandler const & handler);
        bool UnregisterSendAvailableContainerImagesEventHandler(AvailableContainerImagesEventHHandler const & hHandler);

        Common::ErrorCode FindServiceTypeRegistration(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            Reliability::ServiceDescription const & serviceDescription,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            __out uint64 & sequenceNumber,
            __out ServiceTypeRegistrationSPtr & registration);

        Common::ErrorCode GetHostId(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            std::wstring const & applicationName,
            __out std::wstring & hostId);

        Common::ErrorCode GetHostId(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            std::wstring const & applicationName,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            __out std::wstring & hostId);

        void InitializeHealthReportingComponent(Client::HealthReportingComponentSPtr const & healthClient) override;
        void InitializePassThroughClientFactory(Api::IClientFactoryPtr const &clientFactoryPtr) override;
        void InitializeLegacyTestabilityRequestHandler(LegacyTestabilityRequestHandlerSPtr const & legacyTestabilityRequestHandler) override;

        /* Application Download and Upgrade */
        Common::AsyncOperationSPtr BeginDownloadApplication(
            ApplicationDownloadSpecification const & appDownloadSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndDownloadApplication(
            Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode AnalyzeApplicationUpgrade(
            ServiceModel::ApplicationUpgradeSpecification const & appUpgradeSpec,
            __out CaseInsensitiveStringSet & affectedRuntimeIds);

        Common::AsyncOperationSPtr BeginUpgradeApplication(
            ServiceModel::ApplicationUpgradeSpecification const & appUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndUpgradeApplication(
            Common::AsyncOperationSPtr const & operation);

        /* Fabric Download and Upgrade */
        Common::AsyncOperationSPtr BeginDownloadFabric(
            Common::FabricVersion const & fabricVersion,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndDownloadFabric(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginValidateFabricUpgrade(
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndValidateFabricUpgrade(
            __out bool & shouldRestartReplica,
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginFabricUpgrade(
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            bool const shouldRestartReplica,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndFabricUpgrade(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginRestartDeployedPackage(
            int64 codePackageInstanceId,
            std::wstring applicationNameArgument,
            std::wstring serviceManifestNameArgument,
            std::wstring codePackageNameArgument,
            Common::ActivityId const activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::AsyncOperationSPtr BeginRestartDeployedPackage(
            int64 codePackageInstanceId,
            std::wstring const & applicationNameArgument,
            std::wstring const & serviceManifestNameArgument,
            std::wstring const & servicePackageActivationId,
            std::wstring const & codePackageNameArgument,
            Common::ActivityId const activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndRestartDeployedPackage(
            Common::AsyncOperationSPtr const &operation,
            __out Transport::MessageUPtr& reply) override;

        Common::AsyncOperationSPtr BeginTerminateServiceHost(
            std::wstring const & hostId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        void EndTerminateServiceHost(
            Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode IncrementUsageCount(
            ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext);

        void DecrementUsageCount(
            ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext);

        /*Forward health reports received via IPC*/
        void SendToHealthManager(Transport::MessageUPtr && messagePtr, Transport::IpcReceiverContextUPtr && ipcTransportContext);

        __declspec(property(get=get_BlocklistingEnabled, put=set_BlocklistingEnabled)) bool ServiceTypeBlocklistingEnabled;
        bool get_BlocklistingEnabled() const;
        void set_BlocklistingEnabled(bool);

        __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        inline std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get=get_NodeIdAsLargeInteger)) Common::LargeInteger const & NodeIdAsLargeInteger;
        inline Common::LargeInteger const & get_NodeIdAsLargeInteger() const { return nodeIdAsLargeInteger_; }

        __declspec(property(get=get_NodeInstanceId)) uint64 NodeInstanceId;
        inline uint64 get_NodeInstanceId() const { return federation_.Instance.getInstanceId(); }

        __declspec(property(get=get_NodeName)) std::wstring const NodeName;
        inline std::wstring const get_NodeName() const { return fabricNodeConfig_->InstanceName; }

        __declspec(property(get=get_NodeType)) std::wstring const NodeType;
        inline std::wstring const get_NodeType() const { return fabricNodeConfig_->NodeType; }

        __declspec(property(get=get_RuntimeServiceAddress)) std::wstring const RuntimeServiceAddress;
        inline std::wstring const get_RuntimeServiceAddress() const { return ipcServer_.TransportListenAddress; }

        __declspec(property(get = get_RuntimeServiceAddressForContainers)) std::wstring const RuntimeServiceAddressForContainers;
        inline std::wstring const get_RuntimeServiceAddressForContainers() const { return ipcServer_.TransportListenAddressTls; }

        __declspec(property(get=get_IpcServer)) Transport::IpcServer & IpcServerObj;
        inline Transport::IpcServer & get_IpcServer() const { return ipcServer_; }

        __declspec(property(get=get_LeaseAgent)) LeaseWrapper::LeaseAgent const & LeaseAgentObj;
        inline LeaseWrapper::LeaseAgent const & get_LeaseAgent() const { return federation_.LeaseAgent; }

        __declspec(property(get=get_FabricRuntimeManager)) FabricRuntimeManagerUPtr const & FabricRuntimeManagerObj;
        inline FabricRuntimeManagerUPtr const & get_FabricRuntimeManager() const { return runtimeManager_; }

        __declspec(property(get=get_ApplicationManager)) ApplicationManagerUPtr const & ApplicationManagerObj;
        inline ApplicationManagerUPtr const & get_ApplicationManager() const { return applicationManager_; }

        __declspec(property(get=get_ApplicationHostManager)) ApplicationHostManagerUPtr const & ApplicationHostManagerObj;
        inline ApplicationHostManagerUPtr const & get_ApplicationHostManager() const { return hostManager_; }

        __declspec(property(get = get_GuestServiceTypeHostManager)) GuestServiceTypeHostManagerUPtr const & GuestServiceTypeHostManagerObj;
        inline GuestServiceTypeHostManagerUPtr const & get_GuestServiceTypeHostManager() const { return typeHostManager_; }

        __declspec(property(get=get_FabricUpgradeManager)) FabricUpgradeManagerUPtr const & FabricUpgradeManagerObj;
        inline FabricUpgradeManagerUPtr const & get_FabricUpgradeManager() const { return fabricUpgradeManager_; }

        __declspec(property(get=get_DownloadManagerObj)) DownloadManagerUPtr const & DownloadManagerObj;
        DownloadManagerUPtr const & get_DownloadManagerObj() { return this->downloadManager_; }

        __declspec(property(get = get_HostingHealthManagerObj)) HostingHealthManagerUPtr const & HealthManagerObj;
        HostingHealthManagerUPtr const & get_HostingHealthManagerObj() const { return this->healthManager_; }

        __declspec(property(get=get_EventDispatcher)) EventDispatcherUPtr const & EventDispatcherObj;
        inline EventDispatcherUPtr const & get_EventDispatcher() const { return eventDispatcher_; }

        __declspec(property(get = get_FabricActivatorClient)) IFabricActivatorClientSPtr const & FabricActivatorClientObj;
        inline IFabricActivatorClientSPtr const & get_FabricActivatorClient() const { return fabricActivatorClient_; }

        __declspec(property(get = get_LocalResourceManager)) LocalResourceManagerUPtr const & LocalResourceManagerObj;
        inline LocalResourceManagerUPtr const & get_LocalResourceManager() const { return this->localResourceManager_; }

        __declspec(property(get = get_LocalSecretServiceManagerObj)) LocalSecretServiceManagerUPtr const & LocalSecretServiceManagerObj;
        inline LocalSecretServiceManagerUPtr const & get_LocalSecretServiceManagerObj() { return this->localSecretServiceManager_; }

        __declspec(property(get=get_DeploymentFolder)) std::wstring const & DeploymentFolder;
        inline std::wstring const & get_DeploymentFolder() const { return deploymentFolder_; }

        __declspec(property(get=get_FabricUpgradeDeploymentFolder)) std::wstring const & FabricUpgradeDeploymentFolder;
        inline std::wstring const & get_FabricUpgradeDeploymentFolder() const { return fabricUpgradeDeploymentFolder_; }

        __declspec(property(get=get_ImageCacheFolder)) std::wstring const & ImageCacheFolder;
        inline std::wstring const & get_ImageCacheFolder() const { return imageCacheFolder_; }

        __declspec(property(get=get_FabricBinFolder)) std::wstring const & FabricBinFolder;
        inline std::wstring const & get_FabricBinFolder() const { return fabricBinFolder_; }

        __declspec(property(get=get_StartApplicationPortRange)) int StartApplicationPortRange;
        inline int get_StartApplicationPortRange() { return startApplicationPortRange_; }

        __declspec(property(get=get_EndApplicationPortRange)) int EndApplicationPortRange;
        inline int get_EndApplicationPortRange() { return endApplicationPortRange_; }

        __declspec(property(get=get_FabricNodeConfig)) Common::FabricNodeConfig & FabricNodeConfigObj;
        inline Common::FabricNodeConfig & get_FabricNodeConfig() const { return *fabricNodeConfig_; }

        __declspec(property(get=get_RunLayout)) Management::ImageModel::RunLayoutSpecification const & RunLayout;
        inline Management::ImageModel::RunLayoutSpecification const & get_RunLayout() const { return runLayout_; }

        __declspec(property(get=get_SharedLayout)) Management::ImageModel::StoreLayoutSpecification const & SharedLayout;
        inline Management::ImageModel::StoreLayoutSpecification const & get_SharedLayout() const { return sharedLayout_; }

        __declspec(property(get=get_NodeWorkFolder)) std::wstring const & NodeWorkFolder;
        inline std::wstring const & get_NodeWorkFolder() const { return nodeWorkFolder_; }

        __declspec(property(get=get_ClientConnectionAddress)) std::wstring const & ClientConnectionAddress;
        inline std::wstring const & get_ClientConnectionAddress() const { return clientConnectionAddress_; }

        __declspec(property(get=get_QueryClient)) Api::IQueryClientPtr const & QueryClient;
        inline Api::IQueryClientPtr const & get_QueryClient() const { return queryClientPtr_; }

        inline NodeRestartManagerClientSPtr const & get_NodeRestartManagerClient() const { return nodeRestartManagerClient_; }
        __declspec(property(get = get_RepairManagementClient)) Api::IRepairManagementClientPtr const & RepairManagementClient;
        inline Api::IRepairManagementClientPtr const & get_RepairManagementClient() const { return repairMgmtClientPtr_; }

        __declspec(property(get = get_SecretStoreClient)) Api::ISecretStoreClientPtr const & SecretStoreClient;
        inline Api::ISecretStoreClientPtr const & get_SecretStoreClient() const { return secretStoreClient_; }

        __declspec(property(get = get_ApplicationSharedLogSettings)) KtlLogger::SharedLogSettingsSPtr ApplicationSharedLogSettings;
        KtlLogger::SharedLogSettingsSPtr get_ApplicationSharedLogSettings() const;

        __declspec(property(get = get_SystemServicesSharedLogSettings)) KtlLogger::SharedLogSettingsSPtr SystemServicesSharedLogSettings;
        KtlLogger::SharedLogSettingsSPtr get_SystemServicesSharedLogSettings() const;

        __declspec(property(get = get_DnsEnvManager)) DnsServiceEnvironmentManagerUPtr const & DnsEnvManager;
        inline DnsServiceEnvironmentManagerUPtr const & get_DnsEnvManager() const { return dnsEnvManager_; }

        __declspec(property(get = get_NetworkInventoryAgent, put = set_NetworkInventoryAgent)) NetworkInventoryAgentSPtr NetworkInventoryAgent;
        NetworkInventoryAgentSPtr const HostingSubsystem::get_NetworkInventoryAgent() const { return networkInventoryAgent_; }
        void HostingSubsystem::set_NetworkInventoryAgent(NetworkInventoryAgentSPtr value) { networkInventoryAgent_ = value; }

        Common::ErrorCode GetDllHostPathAndArguments(std::wstring & dllHostPath, std::wstring & dllHostArguments);
        Common::ErrorCode GetTypeHostPath(std::wstring & typeHostPath, bool useSFReplicatedStore);

        static Common::ErrorCode GetDeploymentFolder(
            __in Common::FabricNodeConfig & fabricNodeConfig,
            std::wstring const & nodeWorkingDirectory,
            __out std::wstring & deploymentFolder);

        static std::wstring GetFabricUpgradeDeploymentFolder(__in Common::FabricNodeConfig & fabricNodeConfig, std::wstring const & nodeWorkingDirectory);
        static std::wstring GetImageCacheFolder(__in Common::FabricNodeConfig & fabricNodeConfig, std::wstring const & nodeWorkingDirectory);

        bool TryGetServicePackagePublicActivationId(
            ServiceModel::ServicePackageIdentifier const & servicePackageId,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            __out std::wstring & activationId);

        bool HostingSubsystem::TryGetExclusiveServicePackageServiceName(
            ServiceModel::ServicePackageIdentifier const & servicePackageId,
            ServiceModel::ServicePackageActivationContext const & activationContext,
			__out std::wstring & serviceName) const;

        bool TryGetServicePackagePublicApplicationName(
            ServiceModel::ServicePackageIdentifier const & servicePackageId,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            __out std::wstring & applicationName) const;

        std::wstring GetOrAddServicePackagePublicActivationId(
            ServiceModel::ServicePackageIdentifier const & servicePackageId,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            std::wstring const & serviceName,
            std::wstring const & applicationName);

        int64 GetNextSequenceNumber() const;

        void Test_SetFabricActivatorClient(IFabricActivatorClientSPtr && testFabricActivatorClient);      

        HostingQueryManagerUPtr & Test_GetHostingQueryManager() { return hostingQueryManager_; }

        // Retrieves the capacity on the node for RG metrics.
        uint64 GetResourceNodeCapacity(std::wstring const& resourceName) const;

        void Test_SetFabricNodeConfig(Common::FabricNodeConfigSPtr && fabricNodeConfig);
        void Test_SetSecretStoreClient(Api::ISecretStoreClientPtr secretStoreClient);

    protected:
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &);

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const & ,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &);

        virtual void OnAbort();

    private:

        void RegisterMessageHandler();
        void RequestMessageHandler(Transport::MessageUPtr & message, Federation::RequestReceiverContextUPtr & requestReceiverContext);

        void ProcessIpcMessage(Transport::MessageUPtr & message, Transport::IpcReceiverContextUPtr & context);

        Common::AsyncOperationSPtr BeginProcessRequest(
            Transport::MessageUPtr &&,
            Federation::RequestReceiverContextUPtr &&,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndProcessRequest(
            Common::AsyncOperationSPtr const &,
            __out Transport::MessageUPtr &,
            __out Federation::RequestReceiverContextUPtr &);

        void OnProcessRequestComplete(Common::ActivityId const &, Common::AsyncOperationSPtr const &);

        class OpenAsyncOperation;
        friend class OpenAsyncOperation;

        class CloseAsyncOperation;
        friend class CloseAsyncOperation;

        class HostingSubsystemQueryHandler;

        class ServicePackageInstanceInfoMap;
        typedef std::shared_ptr<ServicePackageInstanceInfoMap> ServicePackageInstanceInfoMapSPtr;

    private:
        Federation::FederationSubsystem & federation_;
        Common::FabricNodeConfigSPtr fabricNodeConfig_;
        std::wstring const nodeId_;
        Common::LargeInteger nodeIdAsLargeInteger_;
        FabricRuntimeManagerUPtr runtimeManager_;
        ApplicationManagerUPtr applicationManager_;
        IFabricActivatorClientSPtr fabricActivatorClient_;
        NodeRestartManagerClientSPtr nodeRestartManagerClient_;
        ApplicationHostManagerUPtr hostManager_;
        GuestServiceTypeHostManagerUPtr typeHostManager_;
        FabricUpgradeManagerUPtr fabricUpgradeManager_;
        DownloadManagerUPtr downloadManager_;
        HostingQueryManagerUPtr hostingQueryManager_;
        HostingHealthManagerUPtr healthManager_;
        EventDispatcherUPtr eventDispatcher_;
        Transport::IpcServer & ipcServer_;
        DeletionManagerUPtr deletionManager_;
        LocalResourceManagerUPtr localResourceManager_;
        LocalSecretServiceManagerUPtr localSecretServiceManager_;
        std::wstring const imageCacheFolder_;
        std::wstring const deploymentFolder_;
        std::wstring const fabricUpgradeDeploymentFolder_;
        int const startApplicationPortRange_;
        int const endApplicationPortRange_;
        std::wstring const clientConnectionAddress_;
        std::wstring const fabricBinFolder_;
        std::wstring const nodeWorkFolder_;
        Management::ImageModel::RunLayoutSpecification const runLayout_;
        Management::ImageModel::StoreLayoutSpecification const sharedLayout_;
        DnsServiceEnvironmentManagerUPtr dnsEnvManager_;

        Api::IClientFactoryPtr passThroughClientFactoryPtr_;
        Api::IQueryClientPtr queryClientPtr_;
        Api::IRepairManagementClientPtr repairMgmtClientPtr_;
        Api::ISecretStoreClientPtr secretStoreClient_;
        NetworkInventoryAgentSPtr networkInventoryAgent_;

        Common::RwLock hostPathInitializationLock_;
        std::wstring dllHostPath_;
        std::wstring dllHostArguments_;
        std::wstring typeHostPath_;

        IFabricActivatorClientSPtr testFabricActivatorClient_;

        ServicePackageInstanceInfoMapSPtr svcPkgInstanceInfoMap_;

        mutable Common::atomic_uint64 sequenceNumberTracker_;

        KtlLogger::KtlLoggerNodeSPtr ktlLoggerNode_;
    };
}
