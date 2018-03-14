// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Federation/VoteTicket.h"
#include "Federation/TicketTransfer.h"
#include "Federation/VoteProxy.h"

namespace Federation
{
    class VoteEntry
    {
        DENY_COPY(VoteEntry);

    public:
        VoteEntry(std::wstring const & type, std::wstring const & connectionString, VoteProxySPtr && proxy, SiteNode & siteNode);

        ~VoteEntry();

        __declspec (property(get=getVoteId)) NodeId VoteId;
        NodeId getVoteId() const
        {
            return proxy_->VoteId;
        }

        __declspec (property(get=getType)) std::wstring const & Type;
        std::wstring const & getType() const
        {
            return type_;
        }

        __declspec (property(get=getConnectionString)) std::wstring const & ConnectionString;
        std::wstring const & getConnectionString() const
        {
            return connectionString_;
        }

        __declspec (property(get=getIsSharable)) bool IsSharable;
        bool getIsSharable() const
        {
            return proxy_->IsSharable;
        }

        __declspec (property(get=getIsAcquired)) bool IsAcquired;
        bool getIsAcquired() const
        {
            return proxy_->IsAcquired;
        }

        __declspec (property(get=getOwner)) PartnerNodeSPtr const & Owner;
        PartnerNodeSPtr const & getOwner() const
        {
            return proxy_->Owner;
        }

        __declspec (property(get=getProxy)) VoteProxySPtr const & Proxy;
        VoteProxySPtr const & getProxy() const
        {
            return proxy_;
        }

		__declspec (property(get = getSuperTicket)) Common::StopwatchTime SuperTicket;
		Common::StopwatchTime getSuperTicket() const
		{
			return superTicket_;
		}

        __declspec (property(get=getGlobalTicket)) Common::StopwatchTime GlobalTicket;
        Common::StopwatchTime getGlobalTicket() const
        {
            return globalTicket_;
        }

        __declspec (property(get=getLastSend)) Common::StopwatchTime LastSend;
        Common::StopwatchTime getLastSend() const
        {
            return lastSend_;
        }

        Common::ErrorCode Initialize(bool acquireSharedVoter);
        void Dispose();
        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        bool IsSuperTicketOwned() const;
        bool IsSuperTicketEffective() const;
        bool IsVoteOwned() const;
        bool CanSendBootstrapRequest() const;
        bool CanSendVoteTransferRequest() const;
		bool CanSendRenewRequest() const;
        bool CanRemove() const;

        void CompleteBootstrap(SiteNode & siteNode, bool notifySeedNodes);
        void UpdateLastSend();
        bool UpdateLastReceived(NodeInstance const & from);

        void UpdateGlobalTicket(VoteTicket const & ticket);
        void IssueGlobalTicket();
        Common::StopwatchTime PreUpdateGlobalTicket();
        void PostUpdateGlobalTicket(Common::StopwatchTime value, bool updated);
        void UpdateGaps();
        VoteTicket IssueVoteTicket();

        void CreateTicketGap(NodeIdRange const & range);
        void ProcessTicketGap(TicketGap const & gap);
        void ConvertSuperTicket();
        const TicketGap* GetGap(NodeIdRange range);

        void AcceptTransfer(TicketTransfer const & transfer);
        void TransferSuperTicket(std::vector<TicketTransfer> & transfers, Common::TimeSpan delta);
        Transport::MessageUPtr ExtendSuperTicket(Common::TimeSpan delta);

        void CheckOwnershipInBootstrap();
        Common::TimeSpan CheckOwnershipAfterBootstrap(RoutingToken const & token);

    private:
        bool SetGlobalTicket(Common::StopwatchTime globalTicket);
        bool SetSuperTicket(Common::StopwatchTime superTicket);

        void Acquire();

        std::wstring type_;
        std::wstring connectionString_;

        VoteProxySPtr proxy_;
        SiteNode & siteNode_;

        Common::StopwatchTime globalTicket_;
        Common::StopwatchTime superTicket_;

        std::vector<TicketGap> gaps_;

        bool isTokenOwned_;
        bool isUpdatingGlobalTicket_;
        Common::StopwatchTime lastRenew_;
        Common::StopwatchTime lastFailedAcquire_;

        Common::StopwatchTime lastSend_;
        Common::StopwatchTime lastReceive_;
    };

    typedef std::unique_ptr<VoteEntry> VoteEntryUPtr;
}
