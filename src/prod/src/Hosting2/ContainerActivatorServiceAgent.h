// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerActivatorServiceAgent 
        : public Api::IContainerActivatorServiceAgent
        , public Common::ComponentRoot
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ContainerActivatorServiceAgent)

    public:
        ContainerActivatorServiceAgent();

        virtual ~ContainerActivatorServiceAgent();

        static Common::ErrorCode Create( 
            __out ContainerActivatorServiceAgentSPtr & result);

        virtual void Release();

        virtual void ProcessContainerEvents(ContainerEventNotification && notification);

        virtual Common::ErrorCode RegisterContainerActivatorService(
            Api::IContainerActivatorServicePtr const & activatorService);

    private:
        Common::ErrorCode Initialize();

        void RegisterIpcRequestHandler();
        void UnregisterIpcRequestHandler();

        void ProcessIpcMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessActivateContainerRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessDeactivateContainerRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessDownloadContainerImagesMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessDeleteContainerImagesMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessContainerUpdateRoutesMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessInvokeContainerApiMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void SendContainerEventNotification(
            ContainerEventNotification && notification);

        void FinishSendContainerEventNotification(
            Common::AsyncOperationSPtr const & operation);

    private:
        class RegisterAsyncOperation;
        class ActivateContainerAsyncOperation;
        class DeactivateContainerAsyncOperation;
        class DownloadContainerImagesAsyncOperation;
        class DeleteContainerImagesAsyncOperation;
        class InvokeContainerApiAsyncOperation;
        class NotifyContainerEventAsyncOperation;
        class ContainerUpdateRoutesAsyncOperation;

    private:
        DWORD processId_;
        Transport::IpcClientUPtr ipcClient_;
        Transport::IpcServerUPtr ipcServer_;
        Api::IContainerActivatorServicePtr activatorImpl_;
    };
}
