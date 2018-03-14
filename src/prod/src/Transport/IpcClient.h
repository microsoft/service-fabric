// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class IpcClient : public Common::FabricComponent, public Common::TextTraceComponent<Common::TraceTaskCodes::IPC>
    {
        DENY_COPY(IpcClient);

    public:
        IpcClient(
            Common::ComponentRoot const & root,
            std::wstring const & clientId,
            std::wstring const & serverTransportAddress,
            bool useUnreliableTransport,
            std::wstring const & owner);

        ~IpcClient();

        __declspec (property(get=get_ClientId)) std::wstring ClientId;
        std::wstring get_ClientId() const { return clientId_; }

        __declspec (property(get=get_ProcessId)) DWORD ProcessId;
        DWORD get_ProcessId() const { return processId_; }

        void RegisterMessageHandler(Actor::Enum actor, IpcMessageHandler const & messageHandler, bool dispatchOnTransportThread);

        void UnregisterMessageHandler(Actor::Enum actor);

        Common::AsyncOperationSPtr BeginRequest(
            Transport::MessageUPtr && request,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent = Common::AsyncOperationSPtr());

        Common::AsyncOperationSPtr BeginRequest(
            Transport::MessageUPtr && request,
            TransportPriority::Enum,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent = Common::AsyncOperationSPtr());

        Common::ErrorCode EndRequest(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply);

        Common::ErrorCode SendOneWay(
            Transport::MessageUPtr && message,
            Common::TimeSpan expiration = Common::TimeSpan::MaxValue,
            TransportPriority::Enum priority = TransportPriority::Normal);

        __declspec(property(get=get_ServerTransportAddress)) std::wstring const & ServerTransportAddress;
        std::wstring const & get_ServerTransportAddress() const { return serverTransportAddress_; };

        __declspec(property(get=get_SecuritySettings, put=set_SecuritySettings)) Transport::SecuritySettings & SecuritySettings;
        Transport::SecuritySettings const & get_SecuritySettings() const;
        void set_SecuritySettings(Transport::SecuritySettings const & value);

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();
        virtual void Reconnect();

        void SetMaxOutgoingFrameSize(ULONG value);

    private:

        void OnSending(Transport::MessageUPtr & message);
        void OnDisconnect(Common::ErrorCode error);
        void Cleanup();

        std::wstring const clientId_;
        std::wstring const traceId_;
        DWORD processId_;
        std::wstring const serverTransportAddress_;
        std::shared_ptr<IDatagramTransport> transport_;
        IpcDemuxer demuxer_;
        RequestReply requestReply_;
        ISendTarget::SPtr serverSendTarget_;
        Common::TimerSPtr reconnectTimer_;
        Common::atomic_long disconnectCount_;
    };

    typedef std::shared_ptr<IpcClient> IpcClientSPtr;
    typedef std::unique_ptr<IpcClient> IpcClientUPtr;
}
