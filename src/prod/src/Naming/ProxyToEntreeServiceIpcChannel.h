// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    //
    // This class implements the ipc client from EntreeServiceProxy to EntreeService. This maintains
    // two connections. One for EnteeServiceProxy->EntreeService and a separate connection for communications
    // initiated from EntreeService.
    //
    class ProxyToEntreeServiceIpcChannel
        : public EntreeServiceIpcChannelBase
        , public Transport::IpcClient
    {
        DENY_COPY(ProxyToEntreeServiceIpcChannel)

    public:

        explicit ProxyToEntreeServiceIpcChannel(
            Common::ComponentRoot const &root,
            std::wstring const &traceId,
            std::wstring const &ipcServerListenAddress,
            bool useUnreliableTransport = false);

        ~ProxyToEntreeServiceIpcChannel();

        __declspec (property(get=get_NodeId)) Federation::NodeId Id;
        Federation::NodeId get_NodeId() const { return instance_.Id; }

        __declspec (property(get=get_Instance)) Federation::NodeInstance Instance;
        Federation::NodeInstance get_Instance() { return instance_; }

        __declspec (property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() { return nodeName_; }

        __declspec (property(get=get_TraceId, put=put_traceId)) std::wstring & TraceId;
        std::wstring & get_TraceId() { return traceId_; }
        void put_traceId(std::wstring const &id) { traceId_ = id; }

        Common::AsyncOperationSPtr BeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndOpen(Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr BeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndClose(Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr BeginSendToEntreeService(
            Transport::MessageUPtr &&message,
            Common::TimeSpan const &timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndSendToEntreeService(
            Common::AsyncOperationSPtr const &operation,
            __out Transport::MessageUPtr &reply);

        typedef BeginFunction BeginHandleNotification;
        typedef EndFunction EndHandleNotification;

        bool RegisterNotificationsHandler(
            Transport::Actor::Enum actor, 
            BeginHandleNotification const &beginFunction,
            EndHandleNotification const &endFunction);

        bool UnregisterNotificationsHandler(Transport::Actor::Enum actor);

        void UpdateTraceId(std::wstring const &traceId);

    protected:

        bool IsValidIpcRequest(Transport::MessageUPtr &message);

        void OnIpcFailure(Common::ErrorCode const &error, Transport::ReceiverContext const & receiverContext);

        virtual void Reconnect();
    private:

        void ProcessIpcRequest(
            Transport::MessageUPtr && message, 
            Transport::IpcReceiverContextUPtr && context);

        Common::AsyncOperationSPtr BeginRequest(
            Transport::MessageUPtr && request,
            Transport::TransportPriority::Enum,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRequest(
            Common::AsyncOperationSPtr const & operation,
            Transport::MessageUPtr & reply);

        void GetInitializationData(Common::AsyncOperationSPtr const& asyncOperation);
        Transport::MessageUPtr CreateInitializationRequest(Common::TimeSpan const &timeout);

        class OpenAsyncOperation;
        class CloseAsyncOperation;

        //
        // The node name and instance of the fabric node which implements the IPC server.
        //
        Federation::NodeInstance instance_;
        std::wstring nodeName_;
        std::wstring traceId_;
    };
}
