// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class PingManager : public std::enable_shared_from_this<PingManager>
    {
        DENY_COPY(PingManager);

    public:
        PingManager(SiteNode & siteNode);
        ~PingManager();
        
        void Start();
        void Stop();
        void MessageHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext);

    private:
        void TimerCallback();
        void SendPingMessage(PartnerNodeSPtr const & target, std::vector<VoteTicket> const & tickets);
        void SendPingMessages();

    private:
		RWLOCK(Federation.PingManager, thisLock_);
        SiteNode & siteNode_;
        Common::TimerSPtr timerSPtr_;
        _Guarded_by_(thisLock_) bool stopped_;
    };
}
