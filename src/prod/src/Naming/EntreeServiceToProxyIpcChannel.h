// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    //
    // This class implements the ipc server to communicate with EntreeServiceProxy. This class keeps track of the
    // entreeserviceproxy to which it has established connection and allows raising connection faults for that connection.
    //
    class EntreeServiceToProxyIpcChannel
        : public Common::FabricComponent
        , public EntreeServiceIpcChannelBase
    {
        DENY_COPY(EntreeServiceToProxyIpcChannel)

    public:

        EntreeServiceToProxyIpcChannel(
            Common::ComponentRoot const &root,
            std::wstring const &listenAddress,
            Federation::NodeInstance const &instance,
            std::wstring const &nodeName,
            GatewayEventSource const &eventSource,
            bool useUnreliable = false);

        ~EntreeServiceToProxyIpcChannel();

        typedef BeginFunction BeginHandleMessageFromProxy;
        typedef EndFunction EndHandleMessageFromProxy;

        bool RegisterMessageHandler(
            Transport::Actor::Enum actor,
            BeginHandleMessageFromProxy const& beginFunction,
            EndHandleMessageFromProxy const& endFunction);

        bool UnregisterMessageHandler(
            Transport::Actor::Enum actor);

        Common::AsyncOperationSPtr BeginSendToProxy(
            Transport::MessageUPtr &&message,
            Common::TimeSpan const &,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);
        Common::ErrorCode EndSendToProxy(
            Common::AsyncOperationSPtr const &parent,
            Transport::MessageUPtr &);

        typedef std::function<void(
            Common::ErrorCode fault)> ConnectionFaultHandler;
        void SetConnectionFaultHandler(ConnectionFaultHandler const &handler);
        void RemoveConnectionFaultHandler();

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return traceId_; }

        __declspec(property(get=get_TransportListenAddress)) std::wstring const & TransportListenAddress;
        std::wstring const & get_TransportListenAddress() const { return transportListenAddress_; }

        __declspec(property(get = get_EventSource)) GatewayEventSource const & EventSource;
        GatewayEventSource const & get_EventSource() const { return eventSource_; }

        Transport::IDatagramTransportSPtr GetInnerTransport() { return transport_; }
        Transport::RequestReplyUPtr & GetInnerRequestReply() { return requestReply_; }

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

        void OnIpcFailure(Common::ErrorCode const &error, Transport::ReceiverContext const & receiverContext);

    private:

        class ClientRequestJobItem;

        void Cleanup();

        void ProcessOrQueueIpcRequest(
            Transport::MessageUPtr &&message, 
            Transport::ReceiverContextUPtr &&context);

        void ProcessQueuedIpcRequest(
            Transport::MessageUPtr &&message,
            Transport::ReceiverContextUPtr &&context,
            Common::TimeSpan const &remainingTime);

        void ProcessEntreeServiceTransportMessage(
            Transport::MessageUPtr &&message, 
            Transport::ReceiverContextUPtr const &context);

        void RaiseConnectionFaultHandlerIfNeeded(Transport::IpcHeader &header);
        Transport::ISendTarget::SPtr GetProxySendTarget();

        GatewayEventSource const & eventSource_; 

        // IPC server datastructures
        std::shared_ptr<Transport::IDatagramTransport> transport_;
        Transport::DemuxerUPtr demuxer_;
        Transport::RequestReplyUPtr requestReply_;
        ConnectionFaultHandler faultHandler_;
        std::wstring transportListenAddress_;

        std::wstring traceId_;
        Federation::NodeInstance nodeInstance_;
        std::wstring nodeName_;

        Common::RwLock lock_; // TODO rename

        Transport::ISendTarget::SPtr entreeServiceProxySendTarget_;
        std::wstring entreeServiceProxyClientId_;
        DWORD entreeServiceProxyProcessId_;
        std::unique_ptr<Common::CommonTimedJobQueue<EntreeServiceToProxyIpcChannel>> requestJobQueue_;
    };
}
