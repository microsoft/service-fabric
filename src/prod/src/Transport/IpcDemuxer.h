// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class IpcDemuxer : public DemuxerT<Actor::Enum, IpcReceiverContext>
    {
        DENY_COPY(IpcDemuxer);

    public:
        IpcDemuxer(Common::ComponentRoot const & root, IDatagramTransportSPtr const & datagramTransport);

    protected:
        virtual IpcReceiverContextUPtr CreateReceiverContext(Message & message, ISendTarget::SPtr const & replyTargetSPtr);

        virtual Actor::Enum GetActor(Message & message);
    };

    typedef IpcDemuxer::MessageHandler IpcMessageHandler;
}
