// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability {
namespace ReplicationComponent {

    class ReplicationDemuxer : public Transport::DemuxerT<ReplicationEndpointId, Transport::ReceiverContext>
    {
        DENY_COPY(ReplicationDemuxer);
    public:
        ReplicationDemuxer(Common::ComponentRoot const & root, Transport::IDatagramTransportSPtr const & datagramTransport);

    protected:
        virtual Transport::ReceiverContextUPtr CreateReceiverContext(Transport::Message & message, Transport::ISendTarget::SPtr const & replyTargetSPtr);

        virtual ReplicationEndpointId GetActor(Transport::Message & message);
    };

    typedef std::unique_ptr<ReplicationDemuxer> ReplicationDemuxerUPtr;
}
}
