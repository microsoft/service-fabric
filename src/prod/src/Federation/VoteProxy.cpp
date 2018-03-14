// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Federation/VoteProxy.h"

using namespace std;
using namespace Common;
using namespace Federation;

StringLiteral const TraceAcquire("Acquire");
StringLiteral const TracePreempt("Preempt");
StringLiteral const TraceFailure("Failure");
StringLiteral const TraceUpdate("Update");
StringLiteral const TraceDispose("Dispose");

VoteProxy::VoteProxy(NodeId voteId, NodeId proxyId, bool isSharable)
    :   voteId_(voteId),
        proxyId_(proxyId),
        isSharable_(isSharable),
        isAcquired_(false),
        globalTicket_(0),
        superTicket_(0),
        storedGlobalTicket_(0)
{
}

VoteProxy::~VoteProxy()
{
    WriteInfo(TraceDispose, "{0} released vote {1}", proxyId_, voteId_);
}

ErrorCode VoteProxy::Acquire(SiteNode & siteNode, bool preempt)
{
    ASSERT_IF(proxyId_ != siteNode.Id,
        "{0} tries to acquire proxy {1} for {2}",
        siteNode.Id, proxyId_, voteId_);

    NodeConfig ownerConfig(siteNode.Id, siteNode.Address, siteNode.LeaseAgentAddress, siteNode.WorkingDir, siteNode.RingName);

    VoteOwnerState state;
    ErrorCode error = OnAcquire(ownerConfig, FederationConfig::GetConfig().VoteOwnershipLeaseInterval, preempt, state);
    if (error.IsSuccess())
    {
        isAcquired_ = true;
        owner_ = make_shared<PartnerNode>(ownerConfig, siteNode);

        globalTicket_ = StopwatchTime::FromDateTime(state.GlobalTicket);
        superTicket_ = StopwatchTime::FromDateTime(state.SuperTicket);

        WriteInfo(TraceAcquire,
            "{0} acquired vote {1} with state {2},{3}",
            siteNode.Id, voteId_, globalTicket_, superTicket_);
    }
    else if (error.ReadValue() == ErrorCodeValue::OwnerExists)
    {
        WriteInfo(TraceAcquire,
            "{0} unable to acquire vote {1}, existing owner {2}",
            siteNode.Instance, voteId_, ownerConfig);

        owner_ = make_shared<PartnerNode>(ownerConfig, siteNode);
        isAcquired_ = false;
    }
    else
    {
        WriteWarning(TraceAcquire,
            "{0} acquiring vote {1} failed: {2}",
            siteNode.Id, voteId_, error);
    }

    return error;
}

void VoteProxy::ProcessStoreError(ErrorCode error)
{
    if (error.ReadValue() == ErrorCodeValue::NotOwner)
    {
        isAcquired_ = false;

        WriteInfo(TracePreempt,
            "{0} ownership for {1} is preempted",
            proxyId_, voteId_);
    }
    else
    {
        WriteWarning(TraceFailure,
            "{0} failed to update state for vote {1}: {2}",
            proxyId_, voteId_, error);
    }
}

ErrorCode VoteProxy::SetGlobalTicket(StopwatchTime globalTicket)
{
    AcquireWriteLock grab(lock_);

    if (globalTicket < storedGlobalTicket_)
    {
        return ErrorCodeValue::StaleRequest;
    }

    DateTime globalTicketClock = StopwatchTime::ToDateTime(globalTicket);
    ErrorCode error = OnSetGlobalTicket(globalTicketClock);
    if (error.IsSuccess())
    {
        VoteManager::WriteInfo(TraceUpdate,
            "{0} updated vote {1} global ticket from {2} to {3}",
            proxyId_, voteId_, storedGlobalTicket_, globalTicket);

        storedGlobalTicket_ = globalTicket;
    }
    else
    {
        ProcessStoreError(error);
    }

    return error;
}

void VoteProxy::PostSetGlobalTicket(StopwatchTime globalTicket)
{
    globalTicket_ = globalTicket;
}

ErrorCode VoteProxy::SetSuperTicket(StopwatchTime superTicket)
{
    DateTime superTicketClock = StopwatchTime::ToDateTime(superTicket);
    ErrorCode error = OnSetSuperTicket(superTicketClock);
    if (error.IsSuccess())
    {
        TraceInfo(TraceTaskCodes::Bootstrap, "Issue",
            "{0} updated vote {1} super ticket from {2} to {3}",
            proxyId_, voteId_, superTicket_, superTicket);

        superTicket_ = superTicket;
    }
    else
    {
        ProcessStoreError(error);
    }

    return error;
}
