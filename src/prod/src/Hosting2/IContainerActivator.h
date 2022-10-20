// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class IContainerActivator :
        public Common::RootedObject,
        public Common::AsyncFabricComponent
    {
    public:
        IContainerActivator(ComponentRoot const & root) : RootedObject(root) {}

        virtual ~IContainerActivator() {}

        _declspec(property(get = get_DockerManager)) DockerProcessManager & DockerProcessManagerObj;
        virtual DockerProcessManager & get_DockerManager() = 0;

        _declspec(property(get = get_OverlayNetworkManager)) OverlayNetworkManager & OverlayNetworkManagerObj;
        virtual OverlayNetworkManager & get_OverlayNetworkManager() = 0;

        _declspec(property(get = get_NatIPAddressProvider)) NatIPAddressProvider & NatIPAddressProviderObj;
        virtual NatIPAddressProvider & get_NatIPAddressProvider() = 0;

        __declspec(property(get = get_JobQueue)) Common::DefaultTimedJobQueue<IContainerActivator> & JobQueueObject;
        virtual Common::DefaultTimedJobQueue<IContainerActivator> & get_JobQueue() const = 0;

        virtual Common::AsyncOperationSPtr BeginActivate(
            bool isSecUserLocalSystem,
            std::wstring const & appHostId,
            std::wstring const & nodeId,
            Hosting2::ContainerDescription const & containerDescription,
            Hosting2::ProcessDescription const & processDescription,
            std::wstring const & fabricbinPath,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndActivate(
            Common::AsyncOperationSPtr const & operation,
            __out std::wstring & containerId) = 0;

        virtual Common::AsyncOperationSPtr BeginQuery(
            Hosting2::ContainerDescription const & containerDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndQuery(
            Common::AsyncOperationSPtr const & operation,
            __out Hosting2::ContainerInspectResponse & containerInspect) = 0;

        virtual Common::AsyncOperationSPtr BeginDeactivate(
            Hosting2::ContainerDescription const & containerDescription,
            bool configuredForAutoRemove,
            bool isContainerRoot,
#if defined(PLATFORM_UNIX)
            bool isContainerIsolated,
#endif
            std::wstring const & cgroupName,
            bool isGracefulDeactivation,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndDeactivate(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginGetLogs(
            Hosting2::ContainerDescription const & containerDescription,
            bool isServicePackageActivationModeExclusive,
            std::wstring const & containerLogsArgs,
            int previous,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndGetLogs(
            Common::AsyncOperationSPtr const & operation,
            __out std::wstring & containerLogs) = 0;

        virtual Common::AsyncOperationSPtr BeginDownloadImages(
            std::vector<Hosting2::ContainerImageDescription> const & images,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndDownloadImages(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteImages(
            std::vector<std::wstring> const & images,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndDeleteImages(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual AsyncOperationSPtr BeginGetStats(
            Hosting2::ContainerDescription const & containerDescription,
            Hosting2::ProcessDescription const & processDescription,
            std::wstring const & parentId,
            std::wstring const & appServiceId,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent) = 0;

        virtual ErrorCode EndGetStats(
            AsyncOperationSPtr const & operation,
            __out uint64 & memoryUsage,
            __out uint64 & totalCpuTime,
            __out Common::DateTime & timeRead) = 0;

        virtual Common::AsyncOperationSPtr BeginInvokeContainerApi(
            ContainerDescription const & containerDescription,
            std::wstring const & httpVerb,
            std::wstring const & uriPath,
            std::wstring const & contentType,
            std::wstring const & requestBody,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndInvokeContainerApi(
            Common::AsyncOperationSPtr const & operation,
            __out std::wstring & result) = 0;

        virtual void TerminateContainer(
            Hosting2::ContainerDescription const & containerDescription,
            bool isContainerRoot,
            std::wstring const & cgroupName) = 0;

        virtual Common::ErrorCode AllocateIPAddresses(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & codePackageNames,
            __out std::vector<std::wstring> & assignedIps) = 0;

        virtual void ReleaseIPAddresses(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId) = 0;

        virtual void CleanupAssignedIpsToNode(std::wstring const & nodeId) = 0;

        virtual void OnContainerActivatorServiceTerminated() = 0;

        virtual Common::AsyncOperationSPtr BeginAssignOverlayNetworkResources(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & servicePackageId,
            std::map<wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndAssignOverlayNetworkResources(
            Common::AsyncOperationSPtr const & operation,
            __out std::map<std::wstring, std::map<std::wstring, std::wstring>> & assignedNetworkResources) = 0;

        virtual Common::AsyncOperationSPtr BeginReleaseOverlayNetworkResources(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & networkNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndReleaseOverlayNetworkResources(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginCleanupAssignedOverlayNetworkResourcesToNode(
            std::wstring const & nodeId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndCleanupAssignedOverlayNetworkResourcesToNode(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginAttachContainerToNetwork(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & containerId,
            std::map<std::wstring, std::wstring> const & overlayNetworkResources,
            Common::StringCollection const & dnsServerList,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndAttachContainerToNetwork(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginDetachContainerFromNetwork(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & containerId,
            std::vector<std::wstring> const & networkNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndDetachContainerFromNetwork(
            Common::AsyncOperationSPtr const & operation) = 0;

#if defined(PLATFORM_UNIX)
        virtual Common::AsyncOperationSPtr BeginSaveContainerNetworkParamsForLinuxIsolation(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & containerId,
            std::wstring const & openNetworkAssignedIp,
            std::map<std::wstring, std::wstring> const & overlayNetworkResources,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndSaveContainerNetworkParamsForLinuxIsolation(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginClearContainerNetworkParamsForLinuxIsolation(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & containerId,
            std::vector<std::wstring> const & networkNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndClearContainerNetworkParamsForLinuxIsolation(
            Common::AsyncOperationSPtr const & operation) = 0;
#endif

        virtual Common::AsyncOperationSPtr BeginUpdateRoutes(
            OverlayNetworkRoutingInformationSPtr const & routingInfo,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndUpdateRoutes(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginContainerUpdateRoutes(
            std::wstring const & containerId,
            ContainerDescription const & containerDescription,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndContainerUpdateRoutes(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual void GetDeployedNetworkCodePackages(
            std::vector<std::wstring> const & servicePackageIds,
            std::wstring const & codePackageName,
            std::wstring const & networkName,
            __out std::map<std::wstring, std::vector<std::wstring>> & networkReservedCodePackages) = 0;

        virtual void GetDeployedNetworkNames(
            ServiceModel::NetworkType::Enum networkType, 
            std::vector<std::wstring> & networkNames) = 0;

        virtual void RetrieveGatewayIpAddresses(
            ContainerDescription const & containerDescription,
            __out std::vector<std::wstring> & gatewayIpAddresses) = 0;

        virtual Common::ErrorCode RegisterNatIpAddressProvider() = 0;

        virtual Common::ErrorCode UnregisterNatIpAddressProvider() = 0;

        // We need to have a discussion about removing AbortNatIpAddressProvider for CU1 as it is publically exposed.
        virtual void AbortNatIpAddressProvider() = 0;
    };
}