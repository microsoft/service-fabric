// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Federation/VoteProxy.h"

namespace Federation
{
    class SeedNodeProxy : public VoteProxy
    {
        DENY_COPY(SeedNodeProxy);

    public:
        SeedNodeProxy(NodeConfig const & seedConfig, NodeId proxyId);
        ~SeedNodeProxy();

    protected:
        SeedNodeProxy(NodeConfig const & seedConfig, NodeId proxyId, bool shouldZeroTicketOnAbsentFile);

        virtual std::wstring const& GetRingName() const { return seedConfig_.RingName; }

        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

        virtual Common::ErrorCode OnAcquire(__inout NodeConfig & ownerConfig,
            Common::TimeSpan ttl,
            bool preempt,
            __out VoteOwnerState & state);
        virtual Common::ErrorCode OnSetGlobalTicket(Common::DateTime globalTicket);
        virtual Common::ErrorCode OnSetSuperTicket(Common::DateTime superTicket);

        virtual NodeConfig GetConfig();

        virtual Common::AsyncOperationSPtr BeginArbitrate(
            SiteNode & siteNode,
            ArbitrationRequestBody const & request,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndArbitrate(
            Common::AsyncOperationSPtr const & operation,
            SiteNode & siteNode,
            __out ArbitrationReplyBody & result);

    private:
        Common::ErrorCode LoadTickets(__out VoteOwnerState & state);
        Common::ErrorCode WriteData(int offset, void const* buffer, int size);
        void Cleanup();

        static void ProcessArbitrationReply(SiteNodeSPtr const & siteNode, Common::AsyncOperationSPtr const & routeOperation);

        NodeConfig seedConfig_;
        Common::File store_;
        bool shouldZeroTicketOnAbsentFile_;
    };
}
