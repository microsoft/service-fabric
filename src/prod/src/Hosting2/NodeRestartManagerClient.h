// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class NodeRestartManagerClient 
        : public Common::RootedObject
        , public Common::AsyncFabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
    public:
        NodeRestartManagerClient(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting,
            std::wstring const & nodeId);
        virtual ~NodeRestartManagerClient();

        __declspec(property(get=get_Client)) Transport::IpcClient & Client;
        inline Transport::IpcClient & get_Client() const { return *ipcClient_.get(); };

        Common::AsyncOperationSPtr BeginRegister(
            Common::TimeSpan,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndRegister(Common::AsyncOperationSPtr operation);

        bool IsIpcClientInitialized();

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
        void ProcessDisableNodeRequest(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context);
        void OnDisableNodeRequestOperationComplete(Common::AsyncOperationSPtr const & operation);
        void ProcessEnableNodeRequest(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context);
        void OnEnableNodeRequestOperationComplete(Common::AsyncOperationSPtr const & operation);
        void CancelRunningEnableOperation();
        void UnregisterFabricActivatorClient();
        Transport::MessageUPtr CreateUnregisterClientRequest();

    private:
        HostingSubsystem & hosting_;

        std::unique_ptr<Transport::IpcClient> ipcClient_;
        std::wstring nodeId_;
        DWORD processId_;
        std::wstring clientId_;
        std::wstring executor_;
        std::wstring executorPrefix_;
        std::wstring upgradeDomainId_;
        Common::AsyncOperationSPtr disableNodeRequestOperation_;
        Common::AsyncOperationSPtr enableNodeRequestOperation_;

        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class DisableNodeRequestAsyncOperation;
        class EnableNodeRequestAsyncOperation;
    };
}
