// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class IFabricActivatorClient : public Common::AsyncFabricComponent
    {
    public:
        virtual ~IFabricActivatorClient() {}

        virtual Common::AsyncOperationSPtr BeginActivateProcess(
            std::wstring const & applicationId,
            std::wstring const & appServiceId,
            ProcessDescriptionUPtr const & processDescription,
            std::wstring const & runasUserId,
            bool isContainerHost,
            ContainerDescriptionUPtr && containerDescription,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndActivateProcess(Common::AsyncOperationSPtr const &, __out DWORD & processId) = 0;

        virtual Common::AsyncOperationSPtr BeginDeactivateProcess(
            std::wstring const & appServiceId,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndDeactivateProcess(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginActivateHostedService(
            Hosting2::HostedServiceParameters const & params,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndActivateHostedService(Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginDeactivateHostedService(
            std::wstring const & serviceName,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
        virtual Common::ErrorCode EndDeactivateHostedService(Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginConfigureSecurityPrincipals(
            std::wstring const & applicationId,
            ULONG applicationPackageCounter,
            ServiceModel::PrincipalsDescription const & principalsDescription,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndConfigureSecurityPrincipals(
            Common::AsyncOperationSPtr const &,
            __out PrincipalsProviderContextUPtr &) = 0;

        virtual Common::AsyncOperationSPtr BeginConfigureSecurityPrincipalsForNode(
            std::wstring const & applicationId,
            ULONG applicationPackageCounter,
            ServiceModel::PrincipalsDescription const & principalsDescription,
            int allowedUserCreationFailureCount,
            bool updateExisting,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndConfigureSecurityPrincipalsForNode(
            Common::AsyncOperationSPtr const &,
            __out PrincipalsProviderContextUPtr &) = 0;

        //Application Package operations
        virtual Common::AsyncOperationSPtr BeginCleanupSecurityPrincipals(
            std::wstring const & applicationId,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndCleanupSecurityPrincipals(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginCleanupApplicationSecurityGroup(
            std::vector<std::wstring> const & applicationIds,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndCleanupApplicationSecurityGroup(
            Common::AsyncOperationSPtr const &,
            __out std::vector<std::wstring> & deletedAppPrincipals) = 0;

#if defined(PLATFORM_UNIX)
        virtual Common::AsyncOperationSPtr BeginDeleteApplicationFolder(
            std::vector<std::wstring> const & appFolders,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndDeleteApplicationFolder(
            Common::AsyncOperationSPtr const &,
            __out std::vector<std::wstring> & deletedAppFolders) = 0;
#endif

        virtual Common::AsyncOperationSPtr BeginConfigureSMBShare(
            std::vector<std::wstring> const & sidPrincipals,
            DWORD accessMask,
            std::wstring const & localFullPath,
            std::wstring const & shareName,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndConfigureSMBShare(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginConfigureResourceACLs(
            std::vector<std::wstring> const & sids,
            DWORD accessMask,
            std::vector<CertificateAccessDescription> const & certificateAccessDescriptions,
            std::vector<wstring> const & applicationFolders,
            ULONG applicationCounter,
            std::wstring const & nodeId,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndConfigureResourceACLs(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginConfigureCrashDumps(
            bool enable,
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & exeNames,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndConfigureCrashDumps(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginFabricUpgrade(
            bool const useFabricInstallerService,
            std::wstring const & programToRun,
            std::wstring const & arguments,
            std::wstring const & downloadedFabricPackageLocation,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndFabricUpgrade(
            Common::AsyncOperationSPtr const &) = 0;

        //ServicePackageOperations
        virtual Common::AsyncOperationSPtr BeginConfigureEndpointSecurity(
            std::wstring const & principalSid,
            UINT port,
            bool isHttps,
            bool cleanupAcls,
            std::wstring const & prefix,
            std::wstring const & servicePackageId,
            bool isSharedPort,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndConfigureEndpointSecurity(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginCreateSymbolicLink(
            std::vector<ArrayPair<std::wstring, std::wstring>> const & symbolicLinks,
            DWORD flags,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndCreateSymbolicLink(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginDownloadContainerImages(
            std::vector<ContainerImageDescription> const & containerImages,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndDownloadContainerImages(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginConfigureSharedFolderPermissions(
            std::vector<std::wstring> const & sharedFolders,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndConfigureSharedFolderPermissions(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginTerminateProcess(
            std::wstring const & appServiceId,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndTerminateProcess(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginConfigureEndpointCertificateAndFirewallPolicy(
            std::wstring const & servicePackageId,
            std::vector<Hosting2::EndpointCertificateBinding> const & endpointCertBindings,
            bool cleanupEndpointCert,
            bool cleanupFirewallPolicy,
            std::vector<LONG> const & firewallPorts,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndConfigureEndpointCertificateAndFirewallPolicy(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginAssignIpAddresses(
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & codePackages,
            bool cleanup,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndAssignIpAddresses(
            Common::AsyncOperationSPtr const &,
            __out std::vector<std::wstring> & assignedIps) = 0;

        virtual Common::AsyncOperationSPtr BeginManageOverlayNetworkResources(
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & servicePackageId,
            std::map<wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
            ManageOverlayNetworkAction::Enum action,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndManageOverlayNetworkResources(
            Common::AsyncOperationSPtr const &,
            __out std::map<wstring, std::map<std::wstring, std::wstring>> & assignedNetworkResources) = 0;

        virtual Common::AsyncOperationSPtr BeginUpdateRoutes(
            Management::NetworkInventoryManager::PublishNetworkTablesRequestMessage const & networkTables,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndUpdateRoutes(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginConfigureContainerCertificateExport(
            std::map<std::wstring, std::vector<ServiceModel::ContainerCertificateDescription>> const & certificateRef,
            std::wstring workDirectoryPath,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndConfigureContainerCertificateExport(
            Common::AsyncOperationSPtr const &,
            __out std::map<std::wstring, std::wstring> & certificatePaths,
            __out std::map<std::wstring, std::wstring> & certificatePasswordPaths) = 0;

        virtual Common::AsyncOperationSPtr BeginCleanupContainerCertificateExport(
            std::map<std::wstring, std::wstring> const & certificatePaths,
            std::map<std::wstring, std::wstring> const & certificatePasswordPaths,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndCleanupContainerCertificateExport(
            Common::AsyncOperationSPtr const &) = 0;

       virtual Common::AsyncOperationSPtr BeginSetupContainerGroup(
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
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndSetupContainerGroup(
            Common::AsyncOperationSPtr const &,
            __out std::wstring & data) = 0;

        virtual Common::ErrorCode ConfigureEndpointSecurity(
            std::wstring const & principalSid,
            UINT port,
            bool isHttps,
            bool cleanupAcls,
            std::wstring const & servicePackageId,
            bool isSharedPort) = 0;

        virtual Common::ErrorCode ConfigureEndpointBindingAndFirewallPolicy(
            std::wstring const & servicePackageId,
            std::vector<Hosting2::EndpointCertificateBinding> const &,
            bool isCleanup,
            bool cleanupFirewallPolicy,
            std::vector<LONG> const & firewallPorts) = 0;

        virtual Common::ErrorCode CleanupAssignedIPs(
            std::wstring const & servicePackageId) = 0;

        virtual Common::ErrorCode CleanupAssignedOverlayNetworkResources(
            std::map<std::wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & servicePackageId) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteContainerImages(
            std::vector<std::wstring> const & imageNames,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndDeleteContainerImages(
            Common::AsyncOperationSPtr const &) =0 ;

        virtual Common::AsyncOperationSPtr BeginGetContainerInfo(
            std::wstring const & appServiceId,
            bool isServicePackageActivationModeExclusive,
            std::wstring const & containerInfoType,
            std::wstring const & containerInfoArgs,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetContainerInfo(
            Common::AsyncOperationSPtr const &,
            __out wstring & containerInfo) = 0;

        virtual Common::AsyncOperationSPtr BeginGetImages(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetImages(
            Common::AsyncOperationSPtr const &,
            __out std::vector<wstring> & images) = 0;

        virtual void AbortApplicationEnvironment(std::wstring const & applicationId) = 0;

        virtual void AbortProcess(std::wstring const & appServiceId) = 0;

        virtual Common::HHandler AddProcessTerminationHandler(Common::EventHandler const & eventhandler) = 0;
        virtual void RemoveProcessTerminationHandler(Common::HHandler const & handler) = 0;

        virtual Common::HHandler AddContainerHealthCheckStatusChangeHandler(Common::EventHandler const & eventhandler) = 0;
        virtual void RemoveContainerHealthCheckStatusChangeHandler(Common::HHandler const & handler) = 0;


        virtual Common::HHandler AddRootContainerTerminationHandler(Common::EventHandler const & eventhandler) = 0;
        virtual void RemoveRootContainerTerminationHandler(Common::HHandler const & handler) = 0;

        virtual bool IsIpcClientInitialized() = 0;

        virtual Common::AsyncOperationSPtr BeginConfigureNodeForDnsService(
            bool,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndConfigureNodeForDnsService(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetNetworkDeployedPackages(
            std::vector<std::wstring> const &,
            std::wstring const &,
            std::wstring const &,
            std::wstring const &,
            std::map<std::wstring, std::wstring> const &,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetNetworkDeployedPackages(
            Common::AsyncOperationSPtr const &,
            __out std::map<std::wstring, std::map<std::wstring, std::vector<std::wstring>>> &,
            __out std::map<std::wstring, std::map<std::wstring, std::wstring>> &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetDeployedNetworks(
            ServiceModel::NetworkType::Enum,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode EndGetDeployedNetworks(
            Common::AsyncOperationSPtr const &,
            __out std::vector<std::wstring> &) = 0;
    };
}