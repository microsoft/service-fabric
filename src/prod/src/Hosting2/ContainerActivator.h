// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // Forward declaration
    class ContainerEventTracker;

    class ContainerActivator :
        public IContainerActivator,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ContainerActivator)

    public:
        ContainerActivator(
            Common::ComponentRoot const & root,
            ProcessActivationManager & processActivationManager,
            ContainerTerminationCallback const & terminationCallback,
            ContainerEngineTerminationCallback const & engineTerminationCallback,
            ContainerHealthCheckStatusCallback const & healthCheckStatusCallback);

#if defined(PLATFORM_UNIX)
        virtual ~ContainerActivator();
#endif

        _declspec(property(get = get_DockerManager)) DockerProcessManager & DockerProcessManagerObj;
        DockerProcessManager & get_DockerManager() override { return *dockerProcessManager_; };

        _declspec(property(get = get_OverlayNetworkManager)) OverlayNetworkManager & OverlayNetworkManagerObj;
        inline OverlayNetworkManager & get_OverlayNetworkManager() { return *overlayNetworkManager_; };

        _declspec(property(get = get_IPAddressProvider)) IPAddressProvider & IPAddressProviderObj;
        inline IPAddressProvider & get_IPAddressProvider() { return *ipAddressProvider_; };

        _declspec(property(get = get_NatIPAddressProvider)) NatIPAddressProvider & NatIPAddressProviderObj;
        inline NatIPAddressProvider & get_NatIPAddressProvider() { return *natIpAddressProvider_; };

        __declspec(property(get = get_JobQueue)) Common::DefaultTimedJobQueue<IContainerActivator> & JobQueueObject;
        Common::DefaultTimedJobQueue<IContainerActivator> & get_JobQueue() const override { return *requestJobQueue_; }

        Common::AsyncOperationSPtr BeginActivate(
            bool isSecUserLocalSystem,
            std::wstring const & appHostId,
            std::wstring const & nodeId,
            Hosting2::ContainerDescription const & containerDescription,
            Hosting2::ProcessDescription const & processDescription,
            std::wstring const & fabricbinPath,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndActivate(
            Common::AsyncOperationSPtr const & operation,
            std::wstring & containerId) override;

        Common::AsyncOperationSPtr BeginQuery(
            Hosting2::ContainerDescription const & containerDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndQuery(
            Common::AsyncOperationSPtr const & operation,
            __out Hosting2::ContainerInspectResponse & containerInspect) override;

        Common::AsyncOperationSPtr BeginDeactivate(
            ContainerDescription const & containerDescription,
            bool configuredForAutoRemove,
            bool isContainerRoot,
#if defined(PLATFORM_UNIX)
            bool isContainerIsolated,
#endif
            std::wstring const & cgroupName,
            bool isGracefulDeactivation,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndDeactivate(
            Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginGetLogs(
            ContainerDescription const & containerDescription,
            bool isServicePackageActivationModeExclusive,
            std::wstring const & containerLogsArgs,
            int previous,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndGetLogs(
            Common::AsyncOperationSPtr const & operation,
            __out std::wstring & containerLogs) override;

        Common::AsyncOperationSPtr BeginDownloadImages(
            std::vector<Hosting2::ContainerImageDescription> const & images,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndDownloadImages(
            Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginDeleteImages(
            std::vector<std::wstring> const & images,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndDeleteImages(
            Common::AsyncOperationSPtr const & operation) override;

        AsyncOperationSPtr BeginGetStats(
            Hosting2::ContainerDescription const & containerDescription,
            Hosting2::ProcessDescription const & processDescription,
            std::wstring const & parentId,
            std::wstring const & appServiceId,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent) override;

        ErrorCode EndGetStats(
            AsyncOperationSPtr const & operation,
            __out uint64 & memoryUsage,
            __out uint64 & totalCpuTime,
            __out Common::DateTime & timeRead) override;


        Common::AsyncOperationSPtr BeginInvokeContainerApi(
            Hosting2::ContainerDescription const & containerDescription,
            std::wstring const & httpVerb,
            std::wstring const & uriPath,
            std::wstring const & contentType,
            std::wstring const & requestBody,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndInvokeContainerApi(
            Common::AsyncOperationSPtr const & operation,
            __out std::wstring & result) override;

        void TerminateContainer(
            ContainerDescription const & containerDescription,
            bool isContainerRoot,
            std::wstring const & cgroupName) override;

        Common::ErrorCode AllocateIPAddresses(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & codePackageNames,
            __out std::vector<std::wstring> & assignedIps) override;

        void ReleaseIPAddresses(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId) override;

        void CleanupAssignedIpsToNode(std::wstring const & nodeId) override;

        void OnContainerActivatorServiceTerminated() override;

        _declspec(property(get = get_ActivationManager)) FabricActivationManager const & ActivationManager;
        inline FabricActivationManager const & get_ActivationManager() const;

        Common::AsyncOperationSPtr BeginAssignOverlayNetworkResources(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & servicePackageId,
            std::map<wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndAssignOverlayNetworkResources(
            Common::AsyncOperationSPtr const & operation,
            __out std::map<std::wstring, std::map<std::wstring, std::wstring>> & assignedNetworkResources) override;

        Common::AsyncOperationSPtr BeginReleaseOverlayNetworkResources(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & networkNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndReleaseOverlayNetworkResources(
            Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginCleanupAssignedOverlayNetworkResourcesToNode(
            std::wstring const & nodeId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCleanupAssignedOverlayNetworkResourcesToNode(
            Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginAttachContainerToNetwork(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & containerId,
            std::map<std::wstring, std::wstring> const & overlayNetworkResources,
            Common::StringCollection const & dnsServerList,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndAttachContainerToNetwork(
            Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginDetachContainerFromNetwork(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & containerId,
            std::vector<std::wstring> const & networkNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDetachContainerFromNetwork(
            Common::AsyncOperationSPtr const & operation) override;

#if defined(PLATFORM_UNIX)
        Common::AsyncOperationSPtr BeginSaveContainerNetworkParamsForLinuxIsolation(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & containerId,
            std::wstring const & openNetworkAssignedIp,
            std::map<std::wstring, std::wstring> const & overlayNetworkResources,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndSaveContainerNetworkParamsForLinuxIsolation(
            Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginClearContainerNetworkParamsForLinuxIsolation(
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & containerId,
            std::vector<std::wstring> const & networkNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndClearContainerNetworkParamsForLinuxIsolation(
            Common::AsyncOperationSPtr const & operation) override;
#endif

        Common::AsyncOperationSPtr BeginUpdateRoutes(
            OverlayNetworkRoutingInformationSPtr const & routingInfo,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpdateRoutes(
            Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginContainerUpdateRoutes(
            std::wstring const & containerId,
            ContainerDescription const & containerDescription,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndContainerUpdateRoutes(
            Common::AsyncOperationSPtr const & operation) override;

        void GetDeployedNetworkCodePackages(
            std::vector<std::wstring> const & servicePackageIds,
            std::wstring const & codePackageName,
            std::wstring const & networkName,
            __out std::map<std::wstring, std::vector<std::wstring>> & networkReservedCodePackages) override;

        void GetDeployedNetworkNames(
            ServiceModel::NetworkType::Enum networkType,
            std::vector<std::wstring> & networkNames) override;

        void RetrieveGatewayIpAddresses(
            ContainerDescription const & containerDescription,
            __out std::vector<std::wstring> & gatewayIpAddresses) override;

        Common::ErrorCode RegisterNatIpAddressProvider() override;

        Common::ErrorCode UnregisterNatIpAddressProvider() override;

        void AbortNatIpAddressProvider() override;

    protected:
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode OnEndOpen(
            Common::AsyncOperationSPtr const & operation) override;

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        virtual Common::ErrorCode OnEndClose(
            Common::AsyncOperationSPtr const & operation) override;

        virtual void OnAbort();

    private:
        Common::AsyncOperationSPtr BeginSendMessage(
            Transport::MessageUPtr && message,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndSendMessage(
            Common::AsyncOperationSPtr const & operation,
            __out Transport::MessageUPtr & reply);

        void RegisterIpcRequestHandler();

        void UregisterIpcRequestHandler();

        Common::ErrorCode RegisterAndMonitorContainerActivatorService(
            DWORD processId,
            std::wstring const & listenAddress);

        Common::ErrorCode UnregisterContainerActivatorService();

        void ProcessIpcMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessRegisterContainerActivatorServiceRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessContainerEventNotification(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void SendRegisterContainerActivatorServiceReply(
            Common::ErrorCode const & error,
            __in Transport::IpcReceiverContextUPtr & context);

        bool TryGetIpcClient(__out Transport::IpcClientSPtr & ipcClient);

    private:
        class OpenAsyncOperation;
        friend class OpenAsyncOperation;

        class CloseAsyncOperation;
        friend class CloseAsyncOperation;

        class ActivateAsyncOperation;
        friend class ActivateAsyncOperation;

        class GetContainerInfoAsyncOperation;
        friend class GetContainerInfoAsyncOperation;

        class DeactivateAsyncOperation;
        friend class DeactivateAsyncOperation;

        class GetContainerStatsAsyncOperation;
        friend class GetContainerStatsAsyncOperation;

        class GetContainerLogsAsyncOperation;
        friend class GetContainerLogsAsyncOperation;

        class DownloadContainerImagesAsyncOperation;
        class DeleteContainerImagesAsyncOperation;
        class ContainerApiOperation;
        class ContainerUpdateRoutesAsyncOperation;

        class InvokeContainerApiOperation;
        class SendIpcMessageAsyncOperation;

        friend class ContainerEventTracker;

#if defined(PLATFORM_UNIX)
        class ActivateClearContainersAsyncOperation;
        friend class ActivateClearContainersAsyncOperation;
        class DeactivateClearContainersAsyncOperation;
        friend class DeactivateClearContainersAsyncOperation;
        class GetContainerLogsClearContainersAsyncOperation;
        friend class GetContainerLogsClearContainersAsyncOperation;
#endif

    private:
        ProcessActivationManager & processActivationManager_;
        DockerProcessManagerUPtr dockerProcessManager_;

        Common::RwLock containerServiceRegistrationLock_;
        std::shared_ptr<Transport::IpcClient> ipcClient_;
        DWORD containerServiceProcessId_;

        std::unique_ptr<ContainerEventTracker> eventTracker_;
        ContainerTerminationCallback terminationCallback_;
        ContainerEngineTerminationCallback engineTerminationCallback_;
        ContainerHealthCheckStatusCallback healthCheckStatusCallback_;

        // Provider that configures ip addresses
        IPAddressProviderUPtr ipAddressProvider_;

        // Provider that configures nat ip addresses
        NatIPAddressProviderUPtr natIpAddressProvider_;

        // Overlay network manager
        OverlayNetworkManagerUPtr overlayNetworkManager_;

        // Network plugin process manager
        INetworkPluginProcessManagerSPtr networkPluginProcessManager_;

        // This is the callback registered with the network plugin process manager.
        // It is used by the plugin process manager when the plugin process is restarted.
        NetworkPluginProcessRestartedCallback networkPluginProcessRestartedCallback_;

        // Class supporting common overlay network operations
        ContainerNetworkOperationUPtr containerNetworkOperations_;

        std::unique_ptr<Common::DefaultTimedJobQueue<IContainerActivator>> requestJobQueue_;

        const DWORD NatIpRetryIntervalInMilliseconds = 30*1000;
        const int NatIpRetryCount = 5;

#if defined(PLATFORM_UNIX)
        void *containerStatusCallback_;
        void *podSandboxStatusCallback_;
#endif
    };
}