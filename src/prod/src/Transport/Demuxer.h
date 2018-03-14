// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class Demuxer : public DemuxerT<Actor::Enum, ReceiverContext>
    {
        DENY_COPY(Demuxer);
    public:
        Demuxer(Common::ComponentRoot const & root, IDatagramTransportSPtr const & datagramTransport);

    protected:
        virtual ReceiverContextUPtr CreateReceiverContext(Message & message, ISendTarget::SPtr const & replyTargetSPtr);

        virtual Actor::Enum GetActor(Message & message);
    };

    typedef std::unique_ptr<Transport::Demuxer> DemuxerUPtr;
    typedef std::shared_ptr<Transport::Demuxer> DemuxerSPtr;
    typedef Demuxer::MessageHandler DemuxerMessageHandler;
}
