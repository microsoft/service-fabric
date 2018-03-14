// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Upgrade;
using namespace Infrastructure;

namespace 
{
    namespace Source
    {
        StringLiteral const Cancel("Cancel");
        StringLiteral const Upgrade("Upgrade");
        StringLiteral const QueuedCancelCompletion("QueuedCancelCompletion");
    }

    namespace Tag
    {
        StringLiteral const Started("Start");
        StringLiteral const StartedRollback("StartRollback");
        StringLiteral const Dropped("Drop");
        StringLiteral const Accepted("Accept");
        StringLiteral const ResendReply("ResendReply");
        StringLiteral const SuccessfullyCancelled("Cancelled");
        StringLiteral const CancellationQueued("CancellationQueued");
        StringLiteral const Empty("(empty)");
    }
}

UpgradeMessageProcessor::UpgradeMessageProcessor(ReconfigurationAgent & ra)
: ra_(ra),
  isClosed_(false)
{
}

UpgradeStateName::Enum UpgradeMessageProcessor::Test_GetUpgradeState(std::wstring const & identifier)
{
    AcquireReadLock grab(lock_);

    if (StringUtility::AreEqualCaseInsensitive(identifier, *ServiceModel::SystemServiceApplicationNameHelper::SystemServiceApplicationName))
    {
        auto it = upgrades_.find(*Constants::FabricApplicationId);
        if (it == upgrades_.end())
        {
            return UpgradeStateName::Invalid;
        }

        auto const & element = it->second;
        if (element.Current == nullptr)
        {
            return UpgradeStateName::Invalid;
        }

        return element.Current->CurrentState;
    }

    for (auto it : upgrades_)
    {
        if (it.second.Current == nullptr)
        {
            continue;
        }

        auto applicationUpgrade = dynamic_cast<ApplicationUpgrade*>(&it.second.Current->Upgrade);
        if (applicationUpgrade == nullptr)
        {
            continue;
        }

        if (StringUtility::AreEqualCaseInsensitive(applicationUpgrade->UpgradeSpecification.AppName, identifier))
        {
            return it.second.Current->CurrentState;
        }
    }

    return UpgradeStateName::Invalid;
}

bool UpgradeMessageProcessor::IsUpgrading(std::wstring const & applicationId) const
{
    AcquireReadLock grab(lock_);
    if (isClosed_)
    {
        return false;
    }

    auto element = upgrades_.find(applicationId);
    if (element == upgrades_.end())
    {
        return false;
    }

    auto const & stateMachine = element->second.Current;
    if (stateMachine == nullptr)
    {
        return false;
    }

    auto state = stateMachine->CurrentState;
    return state != UpgradeStateName::Completed && state != UpgradeStateName::None;
}

bool UpgradeMessageProcessor::ProcessUpgradeMessage(std::wstring const & applicationId, UpgradeStateMachineSPtr const & incomingUpgrade)
{
    ASSERT_IF(incomingUpgrade == nullptr, "sm can't be null");

    uint64 instanceId = incomingUpgrade->InstanceId;

    {
        AcquireReadLock grab(lock_);
        if (isClosed_)
        {
            // this upgrade should be closed
            incomingUpgrade->Close();
            return false;
        }
    }

    UpgradeStateMachineSPtr existingUpgrade;
    Action::Enum existingUpgradeAction;    
    Action::Enum incomingUpgradeAction;

    DecideUpgradeAction(applicationId, instanceId, incomingUpgrade, incomingUpgradeAction, existingUpgrade, existingUpgradeAction);

    // This order is important - the existing upgrade may need to be closed first
    PerformAction(existingUpgradeAction, existingUpgrade, UpgradeStateMachineSPtr());
    PerformAction(incomingUpgradeAction, incomingUpgrade, existingUpgrade);

    // TODO: Unit tests for this
    return incomingUpgradeAction == Action::Start || incomingUpgradeAction == Action::Queue || incomingUpgradeAction == Action::StartRollback;
}

void UpgradeMessageProcessor::ProcessCancelUpgradeMessage(wstring const & applicationId, ICancelUpgradeContextSPtr const & cancelUpgradeContext)
{
    UpgradeStateMachineSPtr existingUpgrade;
    Action::Enum existingUpgradeAction;
    bool shouldSendReply = DecideCancelAction(applicationId, cancelUpgradeContext, existingUpgradeAction, existingUpgrade);

    if (shouldSendReply)
    {
        cancelUpgradeContext->SendReply();
    }

    PerformAction(existingUpgradeAction, existingUpgrade, UpgradeStateMachineSPtr());
}

void UpgradeMessageProcessor::Close()
{
    RAEventSource::Events->UpgradeMessageProcessorClose(ra_.NodeInstanceIdStr);

    UpgradeMessageProcessorMap copy;

    {
        AcquireWriteLock grab(lock_);

        copy = upgrades_;
        upgrades_.clear();

        isClosed_ = true;
    }

    for(auto it = copy.begin(); it != copy.end(); ++it)
    {
        UpgradeElement & element = it->second;
        
        if (element.Current != nullptr)
        {
            element.Current->Close();
        }

        if (element.Queued != nullptr)
        {
            element.Queued->Close();
        }
    }
}

void UpgradeMessageProcessor::PerformAction(Action::Enum action, UpgradeStateMachineSPtr const & upgrade, UpgradeStateMachineSPtr const & previousUpgrade)
{
    switch (action)
    {
    case Action::None:
    case Action::Queue:
        break;

    case Action::Close:
        ASSERT_IF(upgrade == nullptr, "can't close null upgrade");
        upgrade->Close();
        break;

    case Action::SendReplyAndClose:
        ASSERT_IF(upgrade == nullptr, "can't sendreply null upgrade");
        upgrade->SendReply();
        upgrade->Close();
        break;

    case Action::Start:
        ASSERT_IF(upgrade == nullptr, "can't start null upgrade");
        upgrade->Start(RollbackSnapshotUPtr());
        break;

    case Action::StartRollback:
        {
            ASSERT_IF(upgrade == nullptr, "can't start null upgrade");
            ASSERT_IF(previousUpgrade == nullptr, "can't start rollback without previous upgrade");

            auto snapshot = previousUpgrade->CreateRollbackSnapshot();
            upgrade->Start(move(snapshot));
            break;
        }

    default:
        Assert::CodingError("unknown action {0}", (int)action);
        break;
    };
}

bool UpgradeMessageProcessor::DecideCancelAction(
    std::wstring const & applicationId,
    ICancelUpgradeContextSPtr const & incomingCancelContext,
    Action::Enum & existingUpgradeAction,
    UpgradeStateMachineSPtr & existingUpgrade)
{
    existingUpgradeAction = Action::None;

    AcquireWriteLock grab(lock_);
    auto upgradeIter = upgrades_.find(applicationId);
    if (upgradeIter == upgrades_.end())
    {
        // First Cancellation request for an application 
        // Add it to the map
        // Return true so that action can be taken outside lock
        UpgradeElement element;
        element.CancellationContext = incomingCancelContext;

        upgrades_[applicationId] = element;
        
        TraceUpgradeAction(Source::Cancel, Tag::Accepted, nullptr, nullptr, nullptr, incomingCancelContext);
        return true;
    }

    UpgradeElement & element = upgradeIter->second;
    if (!element.CanProcess(incomingCancelContext->InstanceId))
    {
        TraceUpgradeAction(Source::Cancel, Tag::Dropped, element.Current, nullptr, element.CancellationContext, incomingCancelContext);
        return false;
    }

    // There are two scenarios possible
    // Either an upgrade is in progress
    // Or this element has previously received a cancel message
    existingUpgrade = element.Current;
    auto existingCancelContext = element.CancellationContext;

    ASSERT_IF(existingCancelContext == nullptr && existingUpgrade == nullptr, "Atleast one of existing upgrade or cancel must be non-null");
    ASSERT_IF(existingCancelContext != nullptr && existingUpgrade != nullptr, "Atleast one of existing upgrade or cancel must be non-null");

    if (existingCancelContext != nullptr)
    {
        ASSERT_IF(incomingCancelContext->InstanceId < existingCancelContext->InstanceId, "InstanceId must be equal or greater");

        // The incoming request has equal or greater instance id
        // Store the context (useful for staleness checks etc in the future)
        // return true to send the reply outside lock
        element.CancellationContext = incomingCancelContext;
        
        TraceUpgradeAction(Source::Cancel, Tag::Accepted, existingUpgrade, nullptr, existingCancelContext, incomingCancelContext);
        return true;
    }

    if (existingUpgrade != nullptr)
    {
        ASSERT_IF(incomingCancelContext->InstanceId < existingUpgrade->InstanceId, "Should have been caught by canprocess");

        if (incomingCancelContext->InstanceId == existingUpgrade->InstanceId)
        {
            // drop this cancel message
            // an upgrade for this instance has already been started
            TraceUpgradeAction(Source::Cancel, Tag::Dropped, existingUpgrade, nullptr, existingCancelContext, incomingCancelContext);
            return true;
        }

        // The contract for Cancel requires that at the time at which the Cancel Reply is sent to the FM
        // No further replica can go down because of the upgrade which is in progress
        // Each state determines whether it supports cancellation and allow the state machine to determine whether
        // the cancellation is supported or not and return the appropriate value
        // If the cancellation is supported the state machine will close itself and return
        auto result = existingUpgrade->TryCancelUpgrade(UpgradeCancelMode::Cancel, nullptr);
        switch (result)
        {
        case UpgradeCancelResult::NotAllowed:
            // return false indicating that no response should be sent
            TraceUpgradeAction(Source::Cancel, Tag::Dropped, existingUpgrade, nullptr, existingCancelContext, incomingCancelContext);
            return false;

        case UpgradeCancelResult::Queued:
            Assert::CodingError("No state currently supports queued cancellation");

        case UpgradeCancelResult::Success:
            // The cancel succeeded
            // Close the current upgrade and send the reply
            element.CancellationContext = incomingCancelContext;
            element.Current = nullptr;
            existingUpgradeAction = Action::Close;

            TraceUpgradeAction(Source::Cancel, Tag::SuccessfullyCancelled, existingUpgrade, nullptr, existingCancelContext, incomingCancelContext);
            return true;
        }
    }

    Assert::CodingError("Cannot come here as either existingUpgrade will be non-null or existingCanclContext will be non-null");
}

void UpgradeMessageProcessor::DecideUpgradeAction(
    std::wstring const & applicationId, 
    uint64 instanceId, 
    UpgradeStateMachineSPtr const & incomingUpgrade,
    Action::Enum & incomingUpgradeAction,
    UpgradeStateMachineSPtr & existingUpgrade,
    Action::Enum & existingUpgradeAction)
{
    // init out params
    incomingUpgradeAction = Action::None;
    existingUpgradeAction = Action::None;

    AcquireWriteLock grab(lock_);
    auto upgradeIter = upgrades_.find(applicationId);
    if (upgradeIter == upgrades_.end())
    {
        // New Upgrade
        // Add it to the list of upgrades
        // Mark flag as true so that the statemachine is started outside the lock
        UpgradeElement element;
        element.Current = incomingUpgrade;

        upgrades_[applicationId] = element;
        
        incomingUpgradeAction = Action::Start;

        TraceUpgradeAction(Source::Upgrade, Tag::Started, nullptr, incomingUpgrade, nullptr, nullptr);
        return;
    }

    UpgradeElement & element = upgradeIter->second;
    if (!element.CanProcess(instanceId))
    {
        // Drop this message right now since it is stale
        incomingUpgradeAction = Action::Close;

        TraceUpgradeAction(Source::Upgrade, Tag::Dropped, element.Current, incomingUpgrade, element.CancellationContext, nullptr);
        return;
    }

    // There are two scenarios possible
    // Either an upgrade is in progress
    // Or this element has previously received a cancel message
    existingUpgrade = element.Current;
    auto existingCancelContext = element.CancellationContext;

    ASSERT_IF(existingCancelContext == nullptr && existingUpgrade == nullptr, "Atleast one of existing upgrade or cancel must be non-null");
    ASSERT_IF(existingCancelContext != nullptr && existingUpgrade != nullptr, "Atleast one of existing upgrade or cancel must be non-null");

    if (existingCancelContext  != nullptr)
    {
        ASSERT_IF(instanceId < existingCancelContext->InstanceId, "InstanceId should already have been filtered");

        // cancel for i1
        // then an upgrade for i2 where i2 >= i1
        element.Current = incomingUpgrade;
        element.CancellationContext = nullptr;
        incomingUpgradeAction = Action::Start;

        TraceUpgradeAction(Source::Upgrade, Tag::Started, existingUpgrade, incomingUpgrade, existingCancelContext, nullptr);

        return;
    }

    if (existingUpgrade != nullptr)
    {
        ASSERT_IF(instanceId < existingUpgrade->InstanceId, "This condition should be checked in CanProcess");

        if (instanceId == existingUpgrade->InstanceId)
        {
            // The same instance id on the upgrade message as the existing message
            // If the upgrade is complete ask the SM to send a message
            // Else drop this message
            // Important to send the message on the incoming context and not the existing context
            if (existingUpgrade->IsCompleted)
            {
                incomingUpgradeAction = Action::SendReplyAndClose;

                TraceUpgradeAction(Source::Upgrade, Tag::ResendReply, existingUpgrade, incomingUpgrade, existingCancelContext, nullptr);
            }
            else
            {
                incomingUpgradeAction = Action::Close;

                TraceUpgradeAction(Source::Upgrade, Tag::Dropped, existingUpgrade, incomingUpgrade, existingCancelContext, nullptr);
            }

            return;
        }
        else if (instanceId > existingUpgrade->InstanceId)
        {
            ASSERT_IF(element.Queued != nullptr, "Should have been handled by CanProcess above");    

            // There is no upgrade queued and an upgrade with a higher instance id has arrived
            // Try to cancel the current upgrade
            auto result = existingUpgrade->TryCancelUpgrade(
                UpgradeCancelMode::Rollback,
                [this, applicationId](UpgradeStateMachineSPtr const & inner) { OnCancellationComplete(applicationId, inner); });

            switch (result)
            {
            case UpgradeCancelResult::NotAllowed:
                /*
                    The upgrade is currently in a state where rollback is not allowed
                    The upgrade needs to complete it's current set of actions before processing rollback

                    For example: During fabric upgrade, while replicas are being closed, if the node receives a rollback
                    The RA does not support the rollback because it would leave some replicas still closing
                    And the RA FT state machine does not support open of a closing FT 
                    (the FT must transition to closed/down which is a terminal state)

                    In this situation we rely on FM retry and waiting for the FT to be closed
                */
                incomingUpgradeAction = Action::Close;
                TraceUpgradeAction(Source::Upgrade, Tag::Dropped, existingUpgrade, incomingUpgrade, existingCancelContext, nullptr);
                return ;

            case UpgradeCancelResult::Queued:
                incomingUpgradeAction = Action::Queue;
                element.Queued = incomingUpgrade;

                TraceUpgradeAction(Source::Upgrade, Tag::CancellationQueued, existingUpgrade, incomingUpgrade, existingCancelContext, nullptr);
                return;

            case UpgradeCancelResult::Success:
                // The cancel succeeded
                // If the current upgrade wasn't completed then this is a rollback
                {
                    element.Current = incomingUpgrade;
                    existingUpgradeAction = Action::Close;
                    
                    StringLiteral const* traceTag = nullptr;
                    if (existingUpgrade->IsCompleted)
                    {
                        incomingUpgradeAction = Action::Start;
                        traceTag = &Tag::Started;
                    }
                    else
                    {
                        incomingUpgradeAction = Action::StartRollback;
                        traceTag = &Tag::StartedRollback;
                    }


                    TraceUpgradeAction(Source::Upgrade, *traceTag, existingUpgrade, incomingUpgrade, existingCancelContext, nullptr);
                    return;
                }
            }
        }
    }
}

void UpgradeMessageProcessor::OnCancellationComplete(std::wstring const & applicationId, UpgradeStateMachineSPtr const & sm)
{
    UpgradeStateMachineSPtr current, queued;

    {
        AcquireWriteLock grab(lock_);

        if (isClosed_)
        {
            return;
        }

        auto upgradeIter = upgrades_.find(applicationId);
        ASSERT_IF(upgradeIter == upgrades_.end(), "when an upgrade is cancelleed it must exist {0}. InstanceId {1}", applicationId, sm->InstanceId);

        UpgradeElement & element = upgradeIter->second;

        ASSERT_IF(element.CancellationContext != nullptr, "Cannot have cancellation queued");
        ASSERT_IF(element.Queued == nullptr, "Upgrade must be queued");
        ASSERT_IF(element.Current != sm, "the current upgrade must be cancelled {0} {1} {2}", applicationId, element.Current->InstanceId, sm->InstanceId);

        // store values for processing outside lock
        current = element.Current;
        queued = element.Queued;

        element.Current = element.Queued;
        element.Queued = nullptr;
    }
    
    // perform actions outside the lock
    PerformAction(Action::Close, current, UpgradeStateMachineSPtr());

    if (queued != nullptr)
    {
        PerformAction(Action::StartRollback, queued, current);
    }

    TraceUpgradeAction(
        Source::QueuedCancelCompletion,
        Tag::StartedRollback,
        current,
        queued,
        ICancelUpgradeContextSPtr(),
        ICancelUpgradeContextSPtr());
}

void UpgradeMessageProcessor::TraceUpgradeAction(
    Common::StringLiteral const & source, 
    Common::StringLiteral const & tag, 
    UpgradeStateMachineSPtr const & existingUpgrade, 
    UpgradeStateMachineSPtr const & incomingUpgrade, 
    ICancelUpgradeContextSPtr const & existingcancelContext,
    ICancelUpgradeContextSPtr const & incomingCancelContext)
{
    wstring incomingUpgradeTrace, existingUpgradeTrace;

    if (incomingUpgrade != nullptr)
    {
        incomingUpgradeTrace = wformatString(incomingUpgrade->Upgrade);
    }

    if (existingUpgrade != nullptr)
    {
        existingUpgradeTrace = wformatString(existingUpgrade->Upgrade);
    }

    RAEventSource::Events->UpgradeMessageProcessorAction(
        ra_.NodeInstanceIdStr, 
        source,
        tag, 
        incomingCancelContext == nullptr ? 0 : incomingCancelContext->InstanceId,
        incomingUpgradeTrace, 
        existingcancelContext == nullptr ? 0: existingcancelContext->InstanceId,
        existingUpgradeTrace,
        existingUpgrade != nullptr ? existingUpgrade->CurrentState : UpgradeStateName::None);
}

