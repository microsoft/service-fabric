// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Federation
{
    using namespace Common;
    using namespace std;

    static const StringLiteral PingTimerTag("Ping");

    PingManager::PingManager(SiteNode & siteNode)
        : siteNode_(siteNode),
          stopped_(false)
    {
    }

    PingManager::~PingManager()
    {
    }
    
    void PingManager::Start()
    {
        {
            AcquireWriteLock writeLock(thisLock_);
            if (stopped_)
            {
                return;
            }

            auto rootSPtr = siteNode_.GetSiteNodeSPtr();
            timerSPtr_ = Timer::Create(
                PingTimerTag,
                [this, rootSPtr] (TimerSPtr const &) { this->TimerCallback(); });

            TimeSpan interval = FederationConfig::GetConfig().PingInterval;
            timerSPtr_->Change(interval, interval);
        }

        SendPingMessages();
    }

    void PingManager::Stop()
    {
        bool needToStop;
        {
            AcquireWriteLock writeLock(thisLock_);
            needToStop = (timerSPtr_ != nullptr);
            stopped_ = true;
        }

        if (needToStop)
        {
            timerSPtr_->Cancel();
        }
    }

    void PingManager::SendPingMessage(PartnerNodeSPtr const & target, vector<VoteTicket> const & tickets)
    {
        TimeSpan ttlDelta = siteNode_.GetVoteManager().GetDelta(target->Instance);
        auto message = FederationMessage::GetPing().CreateMessage(GlobalLease((ttlDelta != TimeSpan::MaxValue ? tickets : vector<VoteTicket>()), ttlDelta), false);
        message->AddBody(siteNode_.GetGlobalStore().AddOutputState(&(target->Instance)), true);
        siteNode_.Table.AddGlobalTimeExchangeHeader(*message, target);
        siteNode_.Send(std::move(message), target);
    }

    void PingManager::SendPingMessages()
    {
        std::vector<PartnerNodeSPtr> targets;
        siteNode_.Table.GetPingTargets(targets);

        vector<VoteTicket> tickets;
        siteNode_.GetVoteManager().GenerateGlobalLease(tickets);

        while(!targets.empty())
        {
            PartnerNodeSPtr target = targets.back();
            targets.pop_back();

            SendPingMessage(target, tickets);
        }
    }

    void PingManager::TimerCallback()
    {
        if (siteNode_.IsShutdown)
        {
            // This is not really needed but in case there is a bug and Stop is not
            // called, this can make sure resource is freed.
            timerSPtr_->Cancel();
            return;
        }

        SendPingMessages();
    }

    // todo: check if SiteNode guarantees lifetime safety when calling this method
    void PingManager::MessageHandler(__in Transport::Message & message, OneWayReceiverContext & oneWayReceiverContext)
    {
        oneWayReceiverContext.Accept();

        if (!siteNode_.IsAvailable)
        {
            return;
        }

        auto body = message.GetBodyStream();
        GlobalLease globalLease;
        
        NTSTATUS status = body->ReadSerializable(&globalLease);
        if (!NT_SUCCESS(status))
        {
            return;
        }

        bool newEntry = siteNode_.GetVoteManager().UpdateGlobalTickets(globalLease, oneWayReceiverContext.FromInstance);

        siteNode_.Table.ProcessGlobalTimeExchangeHeader(message, oneWayReceiverContext.FromWithSameInstance);

        SerializableWithActivationList globalStates;
        status = body->ReadSerializable(&globalStates);
        if (NT_SUCCESS(status))
        {
            siteNode_.GetGlobalStore().ProcessInputState(globalStates, oneWayReceiverContext.From->Instance);
        }

        if (newEntry)
        {
            vector<VoteTicket> tickets;
            siteNode_.GetVoteManager().GenerateGlobalLease(tickets);

            SendPingMessage(oneWayReceiverContext.From, tickets);
        }
    }
}
