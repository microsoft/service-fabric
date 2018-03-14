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
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginQuery(
            Hosting2::ContainerDescription const & containerDescription,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndQuery(
            Common::AsyncOperationSPtr const & operation,
            __out Hosting2::ContainerInspectResponse & containerInspect) = 0;

        virtual Common::AsyncOperationSPtr BeginDeactivate(
            std::wstring const & containerName,
            bool configuredForAutoRemove,
            bool isContainerRoot,
            std::wstring const & cgroupName,
            bool isGracefulDeactivation,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndDeactivate(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginGetLogs(
            std::wstring const & containerName,
            std::wstring const & containerLogsArgs,
            bool isContainerRunInteractive,
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

        virtual Common::AsyncOperationSPtr BeginInvokeContainerApi(
            std::wstring const & containerName,
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
            std::wstring const & containerName,
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
    };
}
