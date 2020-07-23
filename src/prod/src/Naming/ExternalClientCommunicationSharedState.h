// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ExternalClientCommunicationSharedState
    {
    public:
        ExternalClientCommunicationSharedState(
            Common::ComponentRoot const & root,
            std::wstring const & traceContext,
            Transport::IDatagramTransportSPtr && transport,
            Transport::RequestReplySPtr && requestReply,
            Transport::DemuxerUPtr && demuxer)
            : root_(root),
              traceContext_(traceContext),
              transport_(std::move(transport)),
              requestReply_(std::move(requestReply)),
              demuxer_(std::move(demuxer))
        {
        }

        __declspec(property(get=get_Root)) Common::ComponentRoot const & Root;
        __declspec(property(get=get_TraceContext)) std::wstring const & TraceContext;
        __declspec(property(get=get_Transport)) Transport::IDatagramTransportSPtr const & Transport;
        __declspec(property(get=get_RequestReply)) Transport::RequestReplySPtr const & RequestReply;
        __declspec(property(get=get_Demuxer)) Transport::DemuxerUPtr const & Demuxer;

        Common::ComponentRoot const & get_Root() const { return root_; }
        std::wstring const & get_TraceContext() const { return traceContext_; }
        Transport::IDatagramTransportSPtr const & get_Transport() const { return transport_; }
        Transport::RequestReplySPtr const & get_RequestReply() const { return requestReply_; }
        Transport::DemuxerUPtr const & get_Demuxer() const { return demuxer_; }

    private:
        Common::ComponentRoot const & root_;
        std::wstring const & traceContext_;
        Transport::IDatagramTransportSPtr transport_;
        Transport::RequestReplySPtr requestReply_;
        Transport::DemuxerUPtr demuxer_;
    };
}
