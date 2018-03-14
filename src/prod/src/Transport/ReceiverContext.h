// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class ReceiverContext
    {
        DENY_COPY_ASSIGNMENT(ReceiverContext);

    public:
        ReceiverContext(MessageId const & messageId, ISendTarget::SPtr const & replyTargetSPtr, IDatagramTransportSPtr const & datagramTransport);
        virtual ~ReceiverContext();

        __declspec(property(get=getMessageId)) Transport::MessageId const & MessageId;
        Transport::MessageId const & getMessageId() const;

        virtual Common::ErrorCode Reply(Transport::MessageUPtr && reply, TransportPriority::Enum = TransportPriority::Normal) const;
        __declspec(property(get=getReplyTarget)) ISendTarget::SPtr const & ReplyTarget;

        ISendTarget::SPtr const & getReplyTarget() const;

        //This method is provided instead of automatically retrieving TimeoutHeader
        //from request message, so that the latency incurred on transport dispatching
        //thread can be avoided when possible.
        void SetTimeoutHeader(TimeoutHeader const & timeoutHeader);

    protected:
        Transport::MessageId messageId_;
        TimeoutHeader timeoutHeader_ { Common::TimeSpan::MaxValue };
        ISendTarget::SPtr replyTargetSPtr_;
        IDatagramTransportSPtr datagramTransport_;
    };

    typedef std::unique_ptr<ReceiverContext> ReceiverContextUPtr;
}
