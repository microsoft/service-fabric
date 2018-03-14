// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerActivatorLinux :
        public IContainerActivator,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ContainerActivatorLinux)

    public:
        ContainerActivatorLinux(
            Common::ComponentRoot const & root,
            ProcessActivationManager & processActivationManager,
            ContainerTerminationCallback const & callback,
            ContainerEngineTerminationCallback const & dockerCallback,
            ContainerHealthCheckStatusCallback const & healthCheckStatusCallback);

        virtual ~ContainerActivatorLinux();

        _declspec(property(get = get_DockerManager)) DockerProcessManager & DockerProcessManagerObj;
        inline DockerProcessManager & get_DockerManager() override { return *dockerProcessManager_; }
        
        __declspec(property(get = get_JobQueue)) Common::DefaultTimedJobQueue<IContainerActivator> & JobQueueObject;
        Common::DefaultTimedJobQueue<IContainerActivator> & get_JobQueue() const override { return *requestProcessingJobQueue_; }

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
    static std::wstring GetContainerHostAddress();

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;

        class ActivateAsyncOperation;
        friend class ActivateAsyncOperation;

        class DeactivateAsyncOperation;
        friend class DeactivateAsyncOperation;

        class GetContainerInfoAsyncOperation;
        friend class GetContainerInfoAsyncOperation;

        class DeactivateContainerProviderAsyncOperation;
        class InitializeContainerEngineAsyncOperation;
        class DownloadContainerImagesAsyncOperation;
        class ConfigureVolumesAsyncOperation;
        class TerminateContainersAsyncOperation;
        class GetDockerStatusAsyncOperation;
        class GetContainerLogsAsyncOperation;
        class ContainerApiOperation;

    private:
        std::wstring fabricBinFolder_;
        std::wstring containerHostAddress_;
        ProcessActivationManager & processActivationManager_;
        DockerProcessManagerUPtr dockerProcessManager_;
        IProcessActivationContextSPtr activationContext_;
        ContainerTerminationCallback callback_;
        ContainerEngineTerminationCallback dockerTerminationCallback_;
        ContainerHealthCheckStatusCallback healthCheckStatusCallback_;

        // Provider that configures ip adresses
        IPAddressProviderUPtr ipAddressProvider_;

        std::unique_ptr<Common::DefaultTimedJobQueue<IContainerActivator>> requestProcessingJobQueue_;
        DockerClientSPtr dockerClient_;
        std::unique_ptr<web::http::client::http_client> httpClient_;
        ContainerImageDownloaderUPtr containerImageDownloader_;
        ProcessDescription defaultProcessDescription_;
    };
}
