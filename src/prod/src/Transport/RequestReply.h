// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class RequestReply
        : Common::RootedObject
        , Common::TextTraceComponent<Common::TraceTaskCodes::Transport>
    {
        DENY_COPY(RequestReply);

        Common::AsyncOperationSPtr CreateRequestAsyncOperation(
            MessageUPtr && request,
            ISendTarget::SPtr const & to,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

    public:
        RequestReply(Common::ComponentRoot const & root, IDatagramTransportSPtr const & datagramTransport, bool dispatchOnTransportThread = false);

        Common::AsyncOperationSPtr BeginRequest(
            MessageUPtr && request, 
            ISendTarget::SPtr const & to,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent = Common::AsyncOperationSPtr());

        Common::AsyncOperationSPtr BeginRequest(
            MessageUPtr && request, 
            ISendTarget::SPtr const & to,
            TransportPriority::Enum,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent = Common::AsyncOperationSPtr());

        template <typename TContext>
        Common::AsyncOperationSPtr BeginRequest(
            MessageUPtr && request,
            ISendTarget::SPtr const & to,
            TContext && context,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent = Common::AsyncOperationSPtr())
        {
            auto op = CreateRequestAsyncOperation(std::move(request), to, timeout, callback, parent);
            op->PushOperationContext(Common::make_unique<OperationContext<TContext>>(move(context))); //push context before start to prevent racing
            op->Start(op);
            return op;
        }

        Common::ErrorCode EndRequest(
            Common::AsyncOperationSPtr const & operation, 
            MessageUPtr & reply);

        Common::AsyncOperationSPtr BeginNotification(
            MessageUPtr && request, 
            ISendTarget::SPtr const & to,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent = Common::AsyncOperationSPtr());

        Common::ErrorCode EndNotification(
            Common::AsyncOperationSPtr const & operation, 
            MessageUPtr & reply);

        virtual bool OnReplyMessage(Transport::Message & reply, ISendTarget::SPtr const &);

        void Open();
        void Close();

        /*If enabled, return SendFailed Error When SendRequest Timeout and return Timeout Error When Reply Timeout.*/
        void EnableDifferentTimeoutError();

    protected:
        IDatagramTransportSPtr GetDatagramTransport() const { return datagramTransport_; }

    private:
        void OnDisconnected(IDatagramTransport::DisconnectEventArgs const & eventArgs);

        IDatagramTransportSPtr datagramTransport_;
        RequestTable requestTable_;
        IDatagramTransport::DisconnectHHandler disconnectHHandler_ = IDatagramTransport::DisconnectEvent::InvalidHHandler;
        bool dispatchOnTransportThread_;
        bool enableDifferentTimeoutError_;
        class RequestReplyAsyncOperation;
    };

    typedef std::unique_ptr<RequestReply> RequestReplyUPtr;
    typedef std::shared_ptr<RequestReply> RequestReplySPtr;
}
