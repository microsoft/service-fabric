// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Federation/VoteOwnerState.h"

namespace Federation
{
    class ArbitrationRequestBody;
    class ArbitrationReplyBody;

    class VoteProxy : public Common::FabricComponent, public Common::TextTraceComponent<Common::TraceTaskCodes::VoteProxy>
    {
        DENY_COPY(VoteProxy);

    public:
        virtual ~VoteProxy();

        __declspec (property(get=getVoteId)) NodeId VoteId;
        NodeId getVoteId() const
        {
            return voteId_;
        }

        __declspec (property(get=getProxyId)) NodeId ProxyId;
        NodeId getProxyId() const
        {
            return proxyId_;
        }

        __declspec (property(get=getIsSharable)) bool IsSharable;
        bool getIsSharable() const
        {
            return isSharable_;
        }

        __declspec (property(get=getIsAcquired)) bool IsAcquired;
        bool getIsAcquired() const
        {
            return isAcquired_;
        }

        __declspec (property(get=getOwner)) PartnerNodeSPtr & Owner;
        PartnerNodeSPtr const & getOwner() const
        {
            return owner_;
        }

        Common::StopwatchTime GetGlobalTicket()
        {
            return globalTicket_;
        }
        Common::ErrorCode SetGlobalTicket(Common::StopwatchTime globalTicket);

        Common::StopwatchTime GetSuperTicket()
        {
            return superTicket_;
        }
        Common::ErrorCode SetSuperTicket(Common::StopwatchTime superTicket);

        void PostSetGlobalTicket(Common::StopwatchTime globalTicket);

        Common::ErrorCode Acquire(SiteNode & siteNode, bool preempt);

        void ResetOwner()
        {
            owner_.reset();
        }
        
        virtual Common::AsyncOperationSPtr BeginArbitrate(
            SiteNode & siteNode,
            ArbitrationRequestBody const & request,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndArbitrate(
            Common::AsyncOperationSPtr const & operation,
            SiteNode & siteNode,
            __out ArbitrationReplyBody & result) = 0;

    protected:
        VoteProxy(NodeId voteId, NodeId proxyId, bool isSharable);

        virtual std::wstring const& GetRingName() const = 0;

        virtual Common::ErrorCode OnAcquire(__inout NodeConfig & ownerConfig,
            Common::TimeSpan ttl,
            bool preempt,
            __out VoteOwnerState & state) = 0;

        virtual Common::ErrorCode OnSetGlobalTicket(Common::DateTime globalTicket) = 0;
        virtual Common::ErrorCode OnSetSuperTicket(Common::DateTime superTicket) = 0;

        bool isAcquired_;

    private:
        void ProcessStoreError(Common::ErrorCode error);

        NodeId voteId_;
        NodeId proxyId_;
        bool isSharable_;
        PartnerNodeSPtr owner_;
        Common::StopwatchTime globalTicket_;
        Common::StopwatchTime superTicket_;
        MUTABLE_RWLOCK(Federation.VoteProxy, lock_);
        Common::StopwatchTime storedGlobalTicket_;
    };

    typedef std::shared_ptr<VoteProxy> VoteProxySPtr;
}
