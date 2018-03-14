// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class DuplexRequestReply : public RequestReply
    {
        DENY_COPY(DuplexRequestReply);

    public:
        typedef std::function<void(Message &, ReceiverContextUPtr &)> NotificationHandler;

        DuplexRequestReply(Common::ComponentRoot const & root, IDatagramTransportSPtr const &);

        void SetNotificationHandler(NotificationHandler const &, bool dispatchOnTransportThread);
        void RemoveNotificationHandler();

        virtual bool OnReplyMessage(Transport::Message & reply, ISendTarget::SPtr const &);

    private:
        NotificationHandler notificationHandler_;
        Common::RwLock notificationHandlerLock_;
        bool dispatchOnTransportThread_;
    };

    typedef std::shared_ptr<DuplexRequestReply> DuplexRequestReplySPtr;
}
