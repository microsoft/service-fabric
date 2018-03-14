// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class BroadcastForwardContext
    {
        DENY_COPY(BroadcastForwardContext);

    public:
        BroadcastForwardContext();

        BroadcastForwardContext(
            NodeId nodeId,
            Transport::MessageId const & broadcastId,
            NodeIdRange const & requestRange,
            std::vector<NodeIdRange> && subRanges,
            Common::AsyncOperationSPtr const & operation,
            RequestReceiverContextUPtr && requestContext,
            std::vector<std::wstring> const & externalRings);

        BroadcastForwardContext(BroadcastForwardContext && other);

        void CompleteRange(NodeIdRange const & range);

        void CompleteLocalRange();

        void CompleteRing(std::wstring const & ringName);

        bool AddRange(
            NodeIdRange const & range, 
            NodeIdRange const & neighborRange, 
            std::vector<NodeIdRange> & result,
            RequestReceiverContextUPtr & requestContext);

        void SetError(Common::ErrorCode error);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

    private:
        class ForwardRequest
        {
            DENY_COPY(ForwardRequest);

        public:
            ForwardRequest(
                NodeIdRange const & requestRange,
                std::vector<NodeIdRange> && subRanges,
                std::vector<std::wstring> const & externalRings,
                Common::AsyncOperationSPtr const & operation,
                RequestReceiverContextUPtr && requestContext);

            ForwardRequest(ForwardRequest && other);
            
            ForwardRequest & operator =(ForwardRequest && other);
            
            bool CompleteIfNeeded(Common::ErrorCode error);
            void CompleteRange(NodeIdRange const & range);
            void CompleteLocalRange(NodeId nodeId, NodeIdRange & localRange);
            void CompleteRing(std::wstring const & ringName);

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

            NodeIdRange requestRange_;
            std::vector<NodeIdRange> pending_;
            std::vector<std::wstring> pendingRings_;
            Common::AsyncOperationSPtr operation_;
            RequestReceiverContextUPtr requestContext_;
        };

        void CompletIfNeeded();

        NodeId nodeId_;
        Transport::MessageId broadcastId_;
        Common::ErrorCode error_;
        std::vector<NodeIdRange> acked_;
        std::vector<ForwardRequest> requests_;
        std::vector<std::wstring> ackedRings_;
    };
}
