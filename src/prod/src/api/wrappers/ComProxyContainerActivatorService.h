// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComProxyContainerActivatorService :
        public Common::ComponentRoot,
        public IContainerActivatorService,
        public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ComProxyContainerActivatorService);

    public:
        ComProxyContainerActivatorService(
            Common::ComPointer<IFabricContainerActivatorService2> const & comImpl);

        virtual ~ComProxyContainerActivatorService();

        virtual Common::ErrorCode StartEventMonitoring(
            bool isContainerServiceManaged,
            uint64 sinceTime);

        virtual Common::AsyncOperationSPtr BeginActivateContainer(
            Hosting2::ContainerActivationArgs const & activationArgs, 
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndActivateContainer(
            Common::AsyncOperationSPtr const & operation,
            __out std::wstring & containerId);

        virtual Common::AsyncOperationSPtr BeginDeactivateContainer(
            Hosting2::ContainerDeactivationArgs const & deactivationArgs,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndDeactivateContainer(
            Common::AsyncOperationSPtr const & operation);

        virtual Common::AsyncOperationSPtr BeginDownloadImages(
            std::vector<Hosting2::ContainerImageDescription> const & images,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndDownloadImages(
            Common::AsyncOperationSPtr const & operation);

        virtual Common::AsyncOperationSPtr BeginDeleteImages(
            std::vector<std::wstring> const & images,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndDeleteImages(
            Common::AsyncOperationSPtr const & operation);

        virtual Common::AsyncOperationSPtr BeginInvokeContainerApi(
            Hosting2::ContainerApiExecutionArgs const & apiExecArgs,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndInvokeContainerApi(
            Common::AsyncOperationSPtr const & operation,
            __out Hosting2::ContainerApiExecutionResponse & apiExecResp);
    
        virtual Common::AsyncOperationSPtr BeginContainerUpdateRoutes(
            Hosting2::ContainerUpdateRoutesRequest const & updateRoutesRequest,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
    
        virtual Common::ErrorCode EndContainerUpdateRoutes(
            Common::AsyncOperationSPtr const & operation);

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;

        class ActivateContainerAsyncOperation;
        class DeactivateContainerAsyncOperation;
        class DownloadContainerImagesAsyncOperation;
        class DeleteContainerImagesAsyncOperation;
        class InvokeContainerApiAsyncOperation;
        class ContainerUpdateRoutesAsyncOperation;

        Common::ComPointer<IFabricContainerActivatorService2> const comImpl_;
    };
}