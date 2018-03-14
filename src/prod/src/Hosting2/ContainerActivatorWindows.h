// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerActivatorWindows :
        public IContainerActivator,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ContainerActivatorWindows)

    public:
        ContainerActivatorWindows(
            Common::ComponentRoot const & root,
            ProcessActivationManager & processActivationManager,
            ContainerTerminationCallback const & terminationCallback,
            ContainerEngineTerminationCallback const & engineTerminationCallback,
            ContainerHealthCheckStatusCallback const & healthCheckStatusCallback);

        _declspec(property(get = get_DockerManager)) DockerProcessManager & DockerProcessManagerObj;
        DockerProcessManager & get_DockerManager() override { return *dockerProcessManager_; };

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
            Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginQuery(
            Hosting2::ContainerDescription const & containerDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndQuery(
            Common::AsyncOperationSPtr const & operation,
            __out Hosting2::ContainerInspectResponse & containerInspect) override;

        Common::AsyncOperationSPtr BeginDeactivate(
            std::wstring const & containerName,
            bool configuredForAutoRemove,
            bool isContainerRoot,
            std::wstring const & cgroupName,
            bool isGracefulDeactivation,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;

        Common::ErrorCode EndDeactivate(
            Common::AsyncOperationSPtr const & operation) override;

        Common::AsyncOperationSPtr BeginGetLogs(
            std::wstring const & containerName,
            std::wstring const & containerLogsArgs,
            bool isContainerRunInteractive,
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

        Common::AsyncOperationSPtr BeginInvokeContainerApi(
            std::wstring const & containerName,
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
            std::wstring const & containerName, 
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

        _declspec(property(get = get_ActivationManager)) FabricActivationManager const & ActivationManager;
        inline FabricActivationManager const & get_ActivationManager() const;

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
            std::wstring const & listenAddress,
            std::wstring const & callbackAddress);

        Common::ErrorCode UnregisterContainerActivatorService(DWORD processId);

        void ProcessIpcMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessRegisterContainerActivatorServiceRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessContainerEventNotification(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void OnContainerActivatorServiceTerminated(
            DWORD processId,
            std::wstring const & requestor,
            Common::ErrorCode const & waitResult);

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

        class GetContainerLogsAsyncOperation;
        friend class GetContainerLogsAsyncOperation;

        class DownloadContainerImagesAsyncOperation;
        class DeleteContainerImagesAsyncOperation;
        class ContainerApiOperation;

        class InvokeContainerApiOperation;
        class SendIpcMessageAsyncOperation;

        struct ContainerInfo;
        class ContainerEventTracker;
        friend class ContainerEventTracker;

        typedef std::map<
            std::wstring /*ContainerName*/, 
            ContainerEventDescription, 
            Common::IsLessCaseInsensitiveComparer<std::wstring>> HealthEventMap;

        typedef std::map<
            std::wstring, /* Node id*/
            std::vector<ContainerHealthStatusInfo>,
            Common::IsLessCaseInsensitiveComparer<wstring>> HealthStatusInfoMap;

        typedef std::map<
            std::wstring, /* ContainerName*/
            ContainerInfo, 
            Common::IsLessCaseInsensitiveComparer<std::wstring>> ContainerInfoMap;

    private:
        ProcessActivationManager & processActivationManager_;
        DockerProcessManagerUPtr dockerProcessManager_;

        Common::RwLock activatorServiceRegistrationLock_;
        ActivatorRequestorUPtr activatorServiceRegistration_;
        std::shared_ptr<Transport::IpcClient> ipcClient_;

        std::unique_ptr<ContainerEventTracker> eventTracker_;
        ContainerTerminationCallback terminationCallback_;
        ContainerEngineTerminationCallback engineTerminationCallback_;
        ContainerHealthCheckStatusCallback healthCheckStatusCallback_;

        // Provider that configures ip adresses
        IPAddressProviderUPtr ipAddressProvider_;

        std::unique_ptr<Common::DefaultTimedJobQueue<IContainerActivator>> requestJobQueue_;
    };
}
