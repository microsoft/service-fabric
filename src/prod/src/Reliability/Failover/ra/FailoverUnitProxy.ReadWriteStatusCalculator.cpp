// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

FailoverUnitProxy::ReadWriteStatusCalculator::AccessStatusValue::AccessStatusValue()
    : value_(AccessStatus::NotPrimary),
    isDynamic_(false),
    isInvalid_(true),
    needsPC_(false)
{
}

FailoverUnitProxy::ReadWriteStatusCalculator::AccessStatusValue::AccessStatusValue(
    AccessStatus::Enum value,
    bool isDynamic,
    bool needsPC,
    bool isInvalid)
  : value_(value),
    isDynamic_(isDynamic),
    isInvalid_(isInvalid),
    needsPC_(needsPC)
{
}

AccessStatus::Enum FailoverUnitProxy::ReadWriteStatusCalculator::AccessStatusValue::Get(Common::AcquireExclusiveLock & fupLock, FailoverUnitProxy const & fup) const
{
    if (isInvalid_)
    {
        Assert::TestAssert("Invalid access status - unknown FUP state {0}", fup);
        return AccessStatus::NotPrimary;
    }

    if (isDynamic_)
    {
        return fup.HasMinReplicaSetAndWriteQuorum(fupLock, needsPC_) ? AccessStatus::Granted : AccessStatus::NoWriteQuorum;
    }

    return value_;
}

FailoverUnitProxy::ReadWriteStatusCalculator::ReadWriteStatusCalculator()
{
    AccessStatusValue const Invalid(AccessStatus::NotPrimary, false, false, true);
    AccessStatusValue const GrantedIfHasWriteQuorumInCC(AccessStatus::NotPrimary, true, false, false);
    AccessStatusValue const GrantedIfHasWriteQuorumInPCAndCC(AccessStatus::NotPrimary, true, true, false);
    AccessStatusValue const Granted(AccessStatus::Granted, false, false, false);
    AccessStatusValue const TryAgain(AccessStatus::TryAgain, false, false, false);
    AccessStatusValue const NotPrimary(AccessStatus::NotPrimary, false, false, false);

    // Set up read status
    readStatus_.LifeCycleState[LifeCycleState::OpeningPrimary] = TryAgain;
    readStatus_.LifeCycleState[LifeCycleState::ReadyPrimary] = Granted;
    readStatus_.LifeCycleState[LifeCycleState::Other] = NotPrimary;

    /*
        During S/P or U/P or I/P the order is role transition -> catchup -> deactivate -> activate
        Readstatus can be granted after catchup completes
    */
    readStatus_.PromoteToPrimaryReconfigurationAccessState[ReconfigurationState::PreWriteStatusCatchup] = Invalid;
    readStatus_.PromoteToPrimaryReconfigurationAccessState[ReconfigurationState::TransitioningRole] = TryAgain;
    readStatus_.PromoteToPrimaryReconfigurationAccessState[ReconfigurationState::CatchupInProgress] = TryAgain;
    readStatus_.PromoteToPrimaryReconfigurationAccessState[ReconfigurationState::CatchupCompleted] = Granted;
    readStatus_.PromoteToPrimaryReconfigurationAccessState[ReconfigurationState::Completed] = Granted;

    /*
        During swap on P/S the order is catchup -> role transition -> complete
        ReadStatus should be revoked as soon as catchup completes
    */
    readStatus_.DemoteToSecondaryReconfigurationAccessState[ReconfigurationState::PreWriteStatusCatchup] = Granted;
    readStatus_.DemoteToSecondaryReconfigurationAccessState[ReconfigurationState::CatchupInProgress] = Granted;
    readStatus_.DemoteToSecondaryReconfigurationAccessState[ReconfigurationState::CatchupCompleted] = TryAgain;
    readStatus_.DemoteToSecondaryReconfigurationAccessState[ReconfigurationState::TransitioningRole] = NotPrimary;
    readStatus_.DemoteToSecondaryReconfigurationAccessState[ReconfigurationState::Completed] = NotPrimary;

    /*
        During P/P with no primary change read does not need to be revoked
    */
    readStatus_.NoPrimaryChangeReconfigurationAccessState[ReconfigurationState::PreWriteStatusCatchup] = Invalid;
    readStatus_.NoPrimaryChangeReconfigurationAccessState[ReconfigurationState::TransitioningRole] = Invalid;
    readStatus_.NoPrimaryChangeReconfigurationAccessState[ReconfigurationState::CatchupInProgress] = Granted;
    readStatus_.NoPrimaryChangeReconfigurationAccessState[ReconfigurationState::CatchupCompleted] = Granted;
    readStatus_.NoPrimaryChangeReconfigurationAccessState[ReconfigurationState::Completed] = Granted;

    /*
        During I/S on idle never grant read
    */
    readStatus_.IdleToActiveReconfigurationAccessState[ReconfigurationState::PreWriteStatusCatchup] = Invalid;
    readStatus_.IdleToActiveReconfigurationAccessState[ReconfigurationState::TransitioningRole] = NotPrimary;
    readStatus_.IdleToActiveReconfigurationAccessState[ReconfigurationState::CatchupInProgress] = Invalid;
    readStatus_.IdleToActiveReconfigurationAccessState[ReconfigurationState::CatchupCompleted] = Invalid;
    readStatus_.IdleToActiveReconfigurationAccessState[ReconfigurationState::Completed] = NotPrimary;

    // Set up write status
    writeStatus_.LifeCycleState[LifeCycleState::OpeningPrimary] = TryAgain;
    writeStatus_.LifeCycleState[LifeCycleState::ReadyPrimary] = GrantedIfHasWriteQuorumInCC;
    writeStatus_.LifeCycleState[LifeCycleState::Other] = NotPrimary;

    /*
        During S/P or U/P or I/P the order is role transition -> catchup -> deactivate -> activate
        WriteStatus is granted only after activate completes
    */
    writeStatus_.PromoteToPrimaryReconfigurationAccessState[ReconfigurationState::PreWriteStatusCatchup] = Invalid;
    writeStatus_.PromoteToPrimaryReconfigurationAccessState[ReconfigurationState::TransitioningRole] = TryAgain;
    writeStatus_.PromoteToPrimaryReconfigurationAccessState[ReconfigurationState::CatchupInProgress] = TryAgain;
    writeStatus_.PromoteToPrimaryReconfigurationAccessState[ReconfigurationState::CatchupCompleted] = TryAgain;
    writeStatus_.PromoteToPrimaryReconfigurationAccessState[ReconfigurationState::Completed] = GrantedIfHasWriteQuorumInCC;

    /*
        During swap on P/S the order is catchup -> role transition -> complete
        WriteStatus is revoked prior to catchup itself
        Transition to NotPrimary once catchup completes as RA guarantees that demote will be completed
    */
    writeStatus_.DemoteToSecondaryReconfigurationAccessState[ReconfigurationState::PreWriteStatusCatchup] = GrantedIfHasWriteQuorumInPCAndCC;
    writeStatus_.DemoteToSecondaryReconfigurationAccessState[ReconfigurationState::CatchupInProgress] = TryAgain;
    writeStatus_.DemoteToSecondaryReconfigurationAccessState[ReconfigurationState::CatchupCompleted] = TryAgain;
    writeStatus_.DemoteToSecondaryReconfigurationAccessState[ReconfigurationState::TransitioningRole] = NotPrimary;
    writeStatus_.DemoteToSecondaryReconfigurationAccessState[ReconfigurationState::Completed] = NotPrimary;

    /*
        During P/P with no primary change write status is granted for both PC and CC
        Until the reconfig completes when only CC is needed
    */
    writeStatus_.NoPrimaryChangeReconfigurationAccessState[ReconfigurationState::PreWriteStatusCatchup] = Invalid;
    writeStatus_.NoPrimaryChangeReconfigurationAccessState[ReconfigurationState::TransitioningRole] = Invalid;
    writeStatus_.NoPrimaryChangeReconfigurationAccessState[ReconfigurationState::CatchupInProgress] = GrantedIfHasWriteQuorumInPCAndCC;
    writeStatus_.NoPrimaryChangeReconfigurationAccessState[ReconfigurationState::CatchupCompleted] = GrantedIfHasWriteQuorumInPCAndCC;
    writeStatus_.NoPrimaryChangeReconfigurationAccessState[ReconfigurationState::Completed] = GrantedIfHasWriteQuorumInCC;

    /*
        During I/S on remote never grant write
    */
    writeStatus_.IdleToActiveReconfigurationAccessState[ReconfigurationState::PreWriteStatusCatchup] = Invalid;
    writeStatus_.IdleToActiveReconfigurationAccessState[ReconfigurationState::TransitioningRole] = NotPrimary;
    writeStatus_.IdleToActiveReconfigurationAccessState[ReconfigurationState::CatchupInProgress] = Invalid;
    writeStatus_.IdleToActiveReconfigurationAccessState[ReconfigurationState::CatchupCompleted] = Invalid;
    writeStatus_.IdleToActiveReconfigurationAccessState[ReconfigurationState::Completed] = NotPrimary;
}

std::pair<AccessStatus::Enum, AccessStatus::Enum> FailoverUnitProxy::ReadWriteStatusCalculator::ComputeReadAndWriteStatus(Common::AcquireExclusiveLock & fupLock, FailoverUnitProxy const & fup) const
{
    return ComputeAccessStatus(readStatus_, writeStatus_, fupLock, fup);
}

std::pair<AccessStatus::Enum, AccessStatus::Enum> FailoverUnitProxy::ReadWriteStatusCalculator::ComputeAccessStatus(
    AccessStatusValueCollection const & readStateValue,
    AccessStatusValueCollection const & writeStateValue,
    Common::AcquireExclusiveLock & fupLock,
    FailoverUnitProxy const & fup) const
{
    LifeCycleState::Enum lifeCycleState;
    if (TryGetLifeCycleState(fupLock, fup, lifeCycleState))
    {
        return ComputeAccessStatusHelper(readStateValue.LifeCycleState[lifeCycleState], writeStateValue.LifeCycleState[lifeCycleState], fupLock, fup);
    }

    ReconfigurationState::Enum reconfigurationState;
    if (TryGetIdleToActiveReconfigurationState(fupLock, fup, reconfigurationState))
    {
        return ComputeAccessStatusHelper(readStateValue.IdleToActiveReconfigurationAccessState[reconfigurationState], writeStateValue.IdleToActiveReconfigurationAccessState[reconfigurationState], fupLock, fup);
    }

    if (TryGetPromoteToPrimaryReconfigurationState(fupLock, fup, reconfigurationState))
    {
        return ComputeAccessStatusHelper(readStateValue.PromoteToPrimaryReconfigurationAccessState[reconfigurationState], writeStateValue.PromoteToPrimaryReconfigurationAccessState[reconfigurationState], fupLock, fup);
    }

    if (TryGetDemoteToSecondaryReconfigurationState(fupLock, fup, reconfigurationState))
    {
        return ComputeAccessStatusHelper(readStateValue.DemoteToSecondaryReconfigurationAccessState[reconfigurationState], writeStateValue.DemoteToSecondaryReconfigurationAccessState[reconfigurationState], fupLock, fup);
    }

    if (TryGetNoPrimaryChangeReconfigurationState(fupLock, fup, reconfigurationState))
    {
        return ComputeAccessStatusHelper(readStateValue.NoPrimaryChangeReconfigurationAccessState[reconfigurationState], writeStateValue.NoPrimaryChangeReconfigurationAccessState[reconfigurationState], fupLock, fup);

    }

    Assert::TestAssert("Unknown FUP state for read/write {0}", fup);

    return make_pair(AccessStatus::NotPrimary, AccessStatus::NotPrimary);
}

std::pair<AccessStatus::Enum, AccessStatus::Enum> FailoverUnitProxy::ReadWriteStatusCalculator::ComputeAccessStatusHelper(
    AccessStatusValue const & readStateValue,
    AccessStatusValue const & writeStateValue,
    Common::AcquireExclusiveLock & fupLock,
    FailoverUnitProxy const & fup
    ) const
{
    auto readStatus = readStateValue.Get(fupLock, fup);
    auto writeStatus = writeStateValue.Get(fupLock, fup);
    return make_pair(readStatus, writeStatus);
}

bool FailoverUnitProxy::ReadWriteStatusCalculator::TryGetLifeCycleState(
    Common::AcquireExclusiveLock &,
    FailoverUnitProxy const & fup,
    __out LifeCycleState::Enum & lifeCycleState) const
{
    switch (fup.State)
    {
    case FailoverUnitProxyStates::Closed:
        lifeCycleState = LifeCycleState::Other;
        return true;

    case FailoverUnitProxyStates::Closing:
        lifeCycleState = LifeCycleState::Other;
        return true;

    case FailoverUnitProxyStates::Opening:
        if (fup.ReplicaDescription.CurrentConfigurationRole == ReplicaRole::Primary)
        {
            if (fup.AreServiceAndReplicatorRoleCurrent)
            {
                lifeCycleState = LifeCycleState::ReadyPrimary;
            }
            else
            {
                lifeCycleState = LifeCycleState::OpeningPrimary;
            }
        }
        else
        {
            lifeCycleState = LifeCycleState::Other;
        }

        return true;

    case FailoverUnitProxyStates::Opened:
        /*
            There are two main conditions to look at:

            - Replica that is S/I IB or P/I IB. In this case IsReconfiguring will return true because it checks that pc epoch is not empty
            - If it is not the above then check if it is indeed reconfiguring

            Thus, first check the CC Role. If this is Idle it cannot be reconfiguring.

            Then check the IsReconfiguring
        */
        if (fup.ReplicaDescription.CurrentConfigurationRole == ReplicaRole::Idle)
        {
            lifeCycleState = LifeCycleState::Other;
            return true;
        }

        if (!fup.IsReconfiguring)
        {
            lifeCycleState = fup.CurrentServiceRole == ReplicaRole::Primary ? LifeCycleState::ReadyPrimary : LifeCycleState::Other;
            return true;
        }

        return false; // Not handled here

    default:
        Assert::CodingError("unknown state {0}", fup);
    }
}

bool FailoverUnitProxy::ReadWriteStatusCalculator::TryGetIdleToActiveReconfigurationState(
    Common::AcquireExclusiveLock &,
    FailoverUnitProxy const & fup,
    __out ReconfigurationState::Enum & reconfigurationState) const
{
    ASSERT_IF(!fup.IsReconfiguring, "should be checked above {0}", fup);

    if (!fup.configurationReplicas_.empty() || fup.ReplicaDescription.CurrentConfigurationRole != ReplicaRole::Secondary)
    {
        return false;
    }

    if (!fup.AreServiceAndReplicatorRoleCurrent)
    {
        reconfigurationState = ReconfigurationState::TransitioningRole;
        return true;
    }

    reconfigurationState = ReconfigurationState::Completed;
    return true;
}

bool FailoverUnitProxy::ReadWriteStatusCalculator::TryGetPromoteToPrimaryReconfigurationState(
    Common::AcquireExclusiveLock & fupLock,
    FailoverUnitProxy const & fup,
    __out ReconfigurationState::Enum & reconfigurationState) const
{
    ASSERT_IF(!fup.IsReconfiguring, "Should be checked above {0}", fup);
    
    if (!fup.FailoverUnitDescription.IsPrimaryChangeBetweenPCAndCC || fup.ReplicaDescription.CurrentConfigurationRole != ReplicaRole::Primary)
    {
        return false;
    }

    // Role transition happens first
    if (!fup.AreServiceAndReplicatorRoleCurrent)
    {
        reconfigurationState = ReconfigurationState::TransitioningRole;
        return true;
    }

    reconfigurationState = GetReconfigurationStateForPrimary(fupLock, fup);
    return true;
}

bool FailoverUnitProxy::ReadWriteStatusCalculator::TryGetDemoteToSecondaryReconfigurationState(
    Common::AcquireExclusiveLock &,
    FailoverUnitProxy const & fup,
    __out ReconfigurationState::Enum & reconfigurationState) const
{
    ASSERT_IF(!fup.IsReconfiguring, "Should be checked above {0}", fup);

    if (!fup.FailoverUnitDescription.IsPrimaryChangeBetweenPCAndCC || fup.ReplicaDescription.CurrentConfigurationRole != ReplicaRole::Secondary)
    {
        return false;
    }

    if (fup.CatchupResult == CatchupResult::NotStarted)
    {
        if (fup.configurationStage_ == ProxyConfigurationStage::PreWriteStatusRevokeCatchup ||
            fup.configurationStage_ == ProxyConfigurationStage::PreWriteStatusRevokeCatchupPending)
        {
            reconfigurationState = ReconfigurationState::PreWriteStatusCatchup;
            return true;
        }

        reconfigurationState = ReconfigurationState::CatchupInProgress;
        return true;
    }

    if (fup.AreServiceAndReplicatorRoleCurrent)
    {
        reconfigurationState = ReconfigurationState::Completed;
        return true;
    }
    else
    {
        reconfigurationState = ReconfigurationState::TransitioningRole;
        return true;
    }
}

bool FailoverUnitProxy::ReadWriteStatusCalculator::TryGetNoPrimaryChangeReconfigurationState(
    Common::AcquireExclusiveLock & fupLock,
    FailoverUnitProxy const & fup,
    __out ReconfigurationState::Enum & reconfigurationState) const
{
    ASSERT_IF(!fup.IsReconfiguring, "Should be checked above {0}", fup);

    if (fup.FailoverUnitDescription.IsPrimaryChangeBetweenPCAndCC)
    {
        return false;
    }

    reconfigurationState = GetReconfigurationStateForPrimary(fupLock, fup);
    return true;
}

FailoverUnitProxy::ReadWriteStatusCalculator::ReconfigurationState::Enum 
FailoverUnitProxy::ReadWriteStatusCalculator::GetReconfigurationStateForPrimary(
    Common::AcquireExclusiveLock & ,
    FailoverUnitProxy const & fup) const
{
    if (fup.CatchupResult == CatchupResult::NotStarted)
    {
        return ReconfigurationState::CatchupInProgress;
    }
    else if (fup.ConfigurationStage == ProxyConfigurationStage::Catchup || fup.ConfigurationStage == ProxyConfigurationStage::CatchupPending)
    {
        return ReconfigurationState::CatchupCompleted;
    }
    else
    {
        return ReconfigurationState::Completed;
    }
}
