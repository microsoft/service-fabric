// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;

LocalMessageSenderComponent::LocalMessageSenderComponent(
    Common::ComponentRoot const & root,
    Transport::IpcClient & ipcClient)
        :pendingMessages_(),
        lock_(),
        open_(false),
        retryTimerSPtr_(),
        ipcClient_(ipcClient),
        retryInterval_(FailoverConfig::GetConfig().LocalMessageRetryInterval),
        Common::RootedObject(root)
{
}

LocalMessageSenderComponent::~LocalMessageSenderComponent()
{
    ASSERT_IF(retryTimerSPtr_, "LocalMessageSenderComponent::~LocalMessageSenderComponent timer is set");
}

void LocalMessageSenderComponent::Open(ReconfigurationAgentProxyId const & id)
{
    {
        AcquireWriteLock grab(lock_);

        id_ = id;

        RAPEventSource::Events->MessageSenderOpen(id_);

        ASSERT_IF(open_, "LocalMessageSenderComponent::Open called while open");
        open_ = true;
    }
}

void LocalMessageSenderComponent::Close()
{
    {
        AcquireWriteLock grab(lock_);
        RAPEventSource::Events->MessageSenderClose(
            id_, pendingMessages_.size());

        ASSERT_IFNOT(open_, "LocalMessageSenderComponent::Close while not open");

        open_ = false;

        if (retryTimerSPtr_ != nullptr)
        {
            retryTimerSPtr_->Cancel();
            retryTimerSPtr_ = nullptr;
        }

        pendingMessages_.clear();
    }
}

void LocalMessageSenderComponent::SetTimerCallerHoldsLock()
{
    RAPEventSource::Events->MessageSenderSetTimer(
        id_, pendingMessages_.size());

    if (!open_)
    {
        // Component has been shutdown
        return;
    }

    if (!retryTimerSPtr_)
    {
        auto root = Root.CreateComponentRoot();

        // Create the timer
        retryTimerSPtr_ = Common::Timer::Create(
            TimerTagDefault,
            [this, root] (Common::TimerSPtr const &)
            { 
                this->RetryCallback();
            });

        retryTimerSPtr_->Change(FailoverConfig::GetConfig().ProxyOutgoingMessageRetryTimerInterval);
    }
}

void LocalMessageSenderComponent::UpdateTimerCallerHoldsLock()
{
    RAPEventSource::Events->MessageSenderUpdateTimer(
        id_, pendingMessages_.size());

    if (!open_)
    {
        // Component has been shutdown
        return;
    }

    ASSERT_IFNOT(retryTimerSPtr_, "UpdateTimer: timer is not set");

    retryTimerSPtr_->Change(FailoverConfig::GetConfig().ProxyOutgoingMessageRetryTimerInterval);
}

void LocalMessageSenderComponent::RetryCallback()
{
    {
        AcquireWriteLock grab(lock_);

        RAPEventSource::Events->MessageSenderCallback(
            id_, pendingMessages_.size());

        if (!open_)
        {
            return;
        }

        for (auto iter = pendingMessages_.begin(); iter != pendingMessages_.end();)
        {
            ProxyOutgoingMessageUPtr const& message = *iter;
            if (message->ShouldResend())
            {
                SendMessage(message->TransportMessage);
            }
            else
            {
                RAPEventSource::Events->MessageSenderRemove(
                    id_, message->TransportMessage->Action);
                
                iter = pendingMessages_.erase(iter);
                continue;
            }

            iter++;
        }

        UpdateTimerCallerHoldsLock();
    }
}

void LocalMessageSenderComponent::AddMessage(ProxyOutgoingMessageUPtr && message)
{
    {
        AcquireWriteLock grab(lock_);

        if (!open_)
        {
            return;
        }

        RAPEventSource::Events->MessageSenderAdd(
            id_, pendingMessages_.size());

        SendMessage(message->TransportMessage);
        pendingMessages_.push_back(move(message));

        SetTimerCallerHoldsLock();
    }
}

void LocalMessageSenderComponent::SendMessage(Transport::MessageUPtr const& message)
{
    RAPEventSource::Events->MessageSenderSend(
    id_, message->Action);

   Transport::MessageUPtr messageCopy = message->Clone();
   ipcClient_.SendOneWay(std::move(messageCopy));
}
