// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    //
    // This class is the wrapper for the transport at entreeservice, between entreeservice and fabric 
    // clients. It special cases the notifications for pass through send target to handle fabric 
    // clients in the same process, and just uses the EntreeServiceToProxyIpcChannel for 
    // everything else.
    //
    class EntreeServiceTransport
        : public Common::FabricComponent
        , public Federation::NodeTraceComponent<Common::TraceTaskCodes::NamingGateway>
    {
        DENY_COPY(EntreeServiceTransport)

    public:
        EntreeServiceTransport(
            Common::ComponentRoot const &root,
            std::wstring const &listenAddress,
            Federation::NodeInstance const &instance,
            std::wstring const &nodeName,
            GatewayEventSource const & eventSource,
            bool useUnreliable = false);

        void RegisterMessageHandler(
            Transport::Actor::Enum actor,
            EntreeServiceToProxyIpcChannel::BeginHandleMessageFromProxy const& beginFunction,
            EntreeServiceToProxyIpcChannel::EndHandleMessageFromProxy const& endFunction);
        void UnregisterMessageHandler(Transport::Actor::Enum actor);

        void SetConnectionFaultHandler(EntreeServiceToProxyIpcChannel::ConnectionFaultHandler const & handler);
        void RemoveConnectionFaultHandler();

        Common::AsyncOperationSPtr BeginNotification(
            Transport::MessageUPtr &&request,
            Common::TimeSpan timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode EndNotification(
            Common::AsyncOperationSPtr const &operation,
            Transport::MessageUPtr &reply);

        __declspec(property(get=get_TransportListenAddress)) std::wstring const & TransportListenAddress;
        std::wstring const & get_TransportListenAddress() const { return ipcToProxy_.TransportListenAddress; }

        Transport::IDatagramTransportSPtr GetInnerTransport() { return ipcToProxy_.GetInnerTransport(); }

        Common::AsyncOperationSPtr Test_BeginRequestReply(
            Transport::MessageUPtr &&request,
            Transport::ISendTarget::SPtr const &to,
            Common::TimeSpan timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent);

        Common::ErrorCode Test_EndRequestReply(
            Common::AsyncOperationSPtr const &operation,
            Transport::MessageUPtr &reply);

    protected:

        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        class NotificationRequestAsyncOperation;

        EntreeServiceToProxyIpcChannel ipcToProxy_;
    };
}
