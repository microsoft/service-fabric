// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Hosting2
{
    class FabricActivatorClient
        : public Common::RootedObject
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
        , public IFabricActivatorClient
    {
    public:
        FabricActivatorClient(
            Common::ComponentRoot const & root,
            _In_ HostingSubsystem & hosting,
            std::wstring const & nodeId,
            std::wstring const & fabricBinFolder,
            uint64 nodeInstanceId);

        virtual ~FabricActivatorClient();

        __declspec(property(get=get_Client)) Transport::IpcClient & Client;
        inline Transport::IpcClient & get_Client() const { return *ipcClient_.get(); };

        Common::AsyncOperationSPtr BeginRegister(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndRegister(Common::AsyncOperationSPtr operation);

        Common::AsyncOperationSPtr BeginActivateProcess(
            std::wstring const & applicationId,
            std::wstring const & appServiceId,
            ProcessDescriptionUPtr const & processDescription,
            std::wstring const & runasUserId,
            bool isContainerHost,
            ContainerDescriptionUPtr && containerDescription,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndActivateProcess(Common::AsyncOperationSPtr const &, __out DWORD & processId) override;

        Common::AsyncOperationSPtr BeginDeactivateProcess(
            std::wstring const & appServiceId,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndDeactivateProcess(Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginDeactivateContainer(
            std::wstring const & containerName,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndDeactivateContainer(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginActivateHostedService(
            HostedServiceParameters const & params,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndActivateHostedService(Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginDeactivateHostedService(
            std::wstring const & serviceName,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode EndDeactivateHostedService(Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginConfigureSecurityPrincipals(
            std::wstring const & applicationId,
            ULONG applicationPackageCounter,
            ServiceModel::PrincipalsDescription const & principalsDescription,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndConfigureSecurityPrincipals(
            Common::AsyncOperationSPtr const &,
            __out PrincipalsProviderContextUPtr &) override;

        // This is currently only supported for system application
        // The only failure that is allowed is CertificateNotFound
        Common::AsyncOperationSPtr BeginConfigureSecurityPrincipalsForNode(
            std::wstring const & applicationId,
            ULONG applicationPackageCounter,
            ServiceModel::PrincipalsDescription const & principalsDescription,
            int allowedUserCreationFailureCount,
            bool updateExisting,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndConfigureSecurityPrincipalsForNode(
            Common::AsyncOperationSPtr const &,
            __out PrincipalsProviderContextUPtr &) override;

        Common::AsyncOperationSPtr BeginCleanupSecurityPrincipals(
            std::wstring const & applicationId,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndCleanupSecurityPrincipals(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginCleanupApplicationSecurityGroup(
            std::vector<std::wstring> const & applicationIds,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndCleanupApplicationSecurityGroup(
            Common::AsyncOperationSPtr const &,
            __out std::vector<std::wstring> & deletedAppPrincipals) override;

#if defined(PLATFORM_UNIX)
        Common::AsyncOperationSPtr BeginDeleteApplicationFolder(
                std::vector<std::wstring> const & appFolders,
                Common::TimeSpan,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndDeleteApplicationFolder(
                Common::AsyncOperationSPtr const &,
                __out std::vector<std::wstring> & deletedAppFolders) override;
#endif

        Common::AsyncOperationSPtr BeginConfigureSMBShare(
            std::vector<std::wstring> const & sidPrincipals,
            DWORD accessMask,
            std::wstring const & localFullPath,
            std::wstring const & shareName,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndConfigureSMBShare(Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginConfigureResourceACLs(
            std::vector<std::wstring> const & sids,
            DWORD accessMask,
            std::vector<CertificateAccessDescription> const & certificateAccessDescriptions,
            std::vector<wstring> const & applicationFolders,
            ULONG applicationCounter,
            std::wstring const & nodeId,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndConfigureResourceACLs(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginConfigureCrashDumps(
            bool enable,
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & exeNames,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndConfigureCrashDumps(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginFabricUpgrade(
            bool const useFabricInstallerService,
            std::wstring const & programToRun,
            std::wstring const & arguments,
            std::wstring const & downloadedFabricPackageLocation,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndFabricUpgrade(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginConfigureEndpointSecurity(
            std::wstring const & principalSid,
            UINT port,
            bool isHttps,
            bool cleanupAcls,
            std::wstring const & prefix,
            std::wstring const & servicePackageId,
            bool isSharedPort,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndConfigureEndpointSecurity(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginCreateSymbolicLink(
            std::vector<ArrayPair<std::wstring, std::wstring>> const & symbolicLinks,
            DWORD flags,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndCreateSymbolicLink(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginDownloadContainerImages(
            std::vector<ContainerImageDescription> const & containerImages,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndDownloadContainerImages(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginConfigureSharedFolderPermissions(
            std::vector<std::wstring> const & sharedFolders,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndConfigureSharedFolderPermissions(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginTerminateProcess(
            std::wstring const & appServiceId,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndTerminateProcess(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginConfigureEndpointCertificateAndFirewallPolicy(
            std::wstring const & servicePackageId,
            std::vector<EndpointCertificateBinding> const & endpointCertBindings,
            bool cleanupEndpointCert,
            bool cleanupFirewallPolicy,
            std::vector<LONG> const & firewallPorts,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndConfigureEndpointCertificateAndFirewallPolicy(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginSetupContainerGroup(
            std::wstring const & containerGroupId,
            ServiceModel::NetworkType::Enum networkType,
            std::wstring const & openNetworkAssignedIp,
            std::map<std::wstring, std::wstring> const & overlayNetworkResources,
            std::vector<std::wstring> const & dnsServers,
            std::wstring const & appfolder,
            std::wstring const & appId,
            std::wstring const & appName,
            std::wstring const & partitionId,
            std::wstring const & servicePackageActivationId,
            ServiceModel::ServicePackageResourceGovernanceDescription const & spRg,
#if defined(PLATFORM_UNIX)
            ContainerPodDescription const & podDescription,
#endif
            bool isCleanup,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndSetupContainerGroup(
            Common::AsyncOperationSPtr const &,
            __out std::wstring & containerName_) override;

        Common::AsyncOperationSPtr BeginAssignIpAddresses(
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & codePackages,
            bool cleanup,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndAssignIpAddresses(
            Common::AsyncOperationSPtr const &,
            __out std::vector<std::wstring> & assignedIps) override;

        Common::AsyncOperationSPtr BeginManageOverlayNetworkResources(
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & servicePackageId,
            std::map<wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
            ManageOverlayNetworkAction::Enum action,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndManageOverlayNetworkResources(
            Common::AsyncOperationSPtr const &,
            __out std::map<wstring, std::map<std::wstring, std::wstring>> & assignedNetworkResources) override;

        Common::AsyncOperationSPtr BeginUpdateRoutes(
            Management::NetworkInventoryManager::PublishNetworkTablesRequestMessage const & networkTables,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndUpdateRoutes(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginDeleteContainerImages(
            std::vector<std::wstring> const & imageNames,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndDeleteContainerImages(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginGetContainerInfo(
            std::wstring const & appServiceId,
            bool isServicePackageActivationModeExclusive,
            std::wstring const & containerInfoType,
            std::wstring const & containerInfoArgs,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndGetContainerInfo(
            Common::AsyncOperationSPtr const &,
            __out wstring & containerInfo) override;

        Common::AsyncOperationSPtr BeginGetImages(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndGetImages(
            Common::AsyncOperationSPtr const &,
            __out std::vector<wstring> & images) override;

       Common::AsyncOperationSPtr BeginConfigureContainerCertificateExport(
            std::map<std::wstring, std::vector<ServiceModel::ContainerCertificateDescription>> const & certificateRef,
            std::wstring workDirectoryPath,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndConfigureContainerCertificateExport(
            Common::AsyncOperationSPtr const &,
            __out std::map<std::wstring, std::wstring> & certificatePaths,
            __out std::map<std::wstring, std::wstring> & certificatePasswordPaths) override;

       Common::AsyncOperationSPtr BeginCleanupContainerCertificateExport(
            std::map<std::wstring, std::wstring> const & certificatePaths,
            std::map<std::wstring, std::wstring> const & certificatePasswordPaths,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
      Common::ErrorCode EndCleanupContainerCertificateExport(
            Common::AsyncOperationSPtr const &) override;;

        Common::ErrorCode ConfigureEndpointSecurity(
            std::wstring const & principalSid,
            UINT port,
            bool isHttps,
            bool cleanupAcls,
            std::wstring const & servicePackageId,
            bool isSharedPort) override;

        Common::ErrorCode ConfigureEndpointBindingAndFirewallPolicy(
            std::wstring const & servicePackageId,
            std::vector<EndpointCertificateBinding> const &,
            bool isCleanup,
            bool cleanupFirewallPolicy,
            std::vector<LONG> const & firewallPorts) override;

        Common::ErrorCode CleanupAssignedIPs(
            std::wstring const & servicePackageId) override;

        Common::ErrorCode FabricActivatorClient::CleanupAssignedOverlayNetworkResources(
            std::map<std::wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & servicePackageId) override;

        void AbortApplicationEnvironment(std::wstring const & applicationId) override;

        void AbortProcess(std::wstring const & appServiceId) override;

        Common::HHandler AddProcessTerminationHandler(Common::EventHandler const & eventhandler) override;
        void RemoveProcessTerminationHandler(Common::HHandler const & handler) override;

        Common::HHandler AddContainerHealthCheckStatusChangeHandler(Common::EventHandler const & eventhandler) override;
        void RemoveContainerHealthCheckStatusChangeHandler(Common::HHandler const & handler) override;


        Common::HHandler AddRootContainerTerminationHandler(Common::EventHandler const & eventhandler) override;
        void RemoveRootContainerTerminationHandler(Common::HHandler const & handler) override;


        bool IsIpcClientInitialized() override;

        Common::AsyncOperationSPtr BeginConfigureNodeForDnsService(
            bool,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndConfigureNodeForDnsService(
            Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr BeginGetNetworkDeployedPackages(
            std::vector<std::wstring> const & servicePackageIds,
            std::wstring const & codePackageName,
            std::wstring const & networkName,
            std::wstring const & nodeId,
            std::map<std::wstring, std::wstring> const & codePackageInstanceAppHostMap,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndGetNetworkDeployedPackages(
            Common::AsyncOperationSPtr const &,
            __out std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> &,
            __out std::map<std::wstring, std::map<std::wstring, std::wstring>> &) override;

        Common::AsyncOperationSPtr BeginGetDeployedNetworks(
            ServiceModel::NetworkType::Enum networkType,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) override;
        Common::ErrorCode EndGetDeployedNetworks(
            Common::AsyncOperationSPtr const &,
            __out std::vector<std::wstring> &) override;

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

        virtual void ProcessIpcMessage(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context);

    private:
        void RegisterIpcMessageHandler();
        void UnregisterIpcMessageHandler();
        void IpcMessageHandler(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context);
        void ProcessApplicationServiceTerminatedRequest(__in Transport::Message &, __in Transport::IpcReceiverContextUPtr & context);
        void ProcessContainerHealthCheckStatusChangeRequest(__in Transport::Message &, __in Transport::IpcReceiverContextUPtr & context);
        void ProcessDockerProcessTerminatedRequest(__in Transport::Message &, __in Transport::IpcReceiverContextUPtr & context);
        void ProcessGetOverlayNetworkDefinitionRequest(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context);
        void ProcessDeleteOverlayNetworkDefinitionRequest(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context);
        void ProcessPublishNetworkTablesRequest(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context);

        Transport::MessageUPtr CreateUnregisterClientRequest();

        void UnregisterFabricActivatorClient();

    private:

        HostingSubsystem & hosting_;
        std::unique_ptr<Transport::IpcClient> ipcClient_;
        std::wstring nodeId_;
        DWORD processId_;
        std::wstring fabricBinFolder_;
        std::wstring clientId_;
        Common::Event processTerminationEvent_;
        Common::Event containerHealthStatusChangeEvent_;
        Common::Event containerRootDiedEvent_;
        uint64 nodeInstanceId_;

        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class BaseActivatorAsyncOperation;
        class ActivateProcessAsyncOperation;
        class DeactivateProcessAsyncOperation;
        class ActivateHostedServiceAsyncOperation;
        class DeactivateHostedServiceAsyncOperation;
        class TerminateProcessAsyncOperation;
        class ConfigureSecurityPrincipalsAsyncOperation;
        class CleanupSecurityPrincipalsAsyncOperation;
        class CleanupApplicationLocalGroupsAsyncOperation;
        class AssignIpAddressesAsyncOperation;
        class ManageOverlayNetworkResourcesAsyncOperation;
        class UpdateOverlayNetworkRoutesAsyncOperation;
        class GetNetworkDeployedPackagesAsyncOperation;
        class GetDeployedNetworksAsyncOperation;
#if defined(PLATFORM_UNIX)
        class DeleteApplicationFoldersAsyncOperation;
#endif
        class ConfigureEndpointSecurityAsyncOperation;
        class ConfigureResourceACLsAsyncOperation;
        class ConfigureCrashDumpsAsyncOperation;
        class FabricUpgradeAsyncOperation;
        class CreateSymbolicLinkAsyncOperation;
        class ConfigureSharedFolderACLsAsyncOperation;
        class ConfigureSBMShareAsyncOperation;
        class ConfigureContainerCertificateExportAsyncOperation;
        class CleanupContainerCertificateExportAsyncOperation;
        class DownloadContainerImagesAsyncOperation;
        class DeleteContainerImagesAsyncOperation;
        class GetContainerInfoAsyncOperation;
        class ConfigureNodeForDnsServiceAsyncOperation;
        class ConfigureContainerGroupAsyncOperation;
        class ConfigureEndpointCertificateAndFirewallPolicyAsyncOperation;
        class GetImagesAsyncOperation;

        class AppInfo
        {
        public: 
            AppInfo(
                wstring appFolder, 
                wstring appId,
                wstring appName)
                : appFolder_(appFolder),
                appId_(appId),
                appName_(appName)
            {}

            wstring appFolder_;
            wstring appId_;
            wstring appName_;
        };
    };
}