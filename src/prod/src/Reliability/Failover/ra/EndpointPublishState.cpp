// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

EndpointPublishState::EndpointPublishState(
    TimeSpanConfigEntry const * publishTimeout,
    ReconfigurationState const * reconfigState,
    FMMessageState * fmMessageState) :
    state_(State::None),
    publishTimeout_(publishTimeout),
    reconfigState_(reconfigState),
    fmMessageState_(fmMessageState)
{
}

EndpointPublishState::EndpointPublishState(
    ReconfigurationState const * reconfigState,
    FMMessageState * fmMessageState,
    EndpointPublishState const & other) :
    state_(other.state_),
    publishTimeout_(other.publishTimeout_),
    reconfigState_(reconfigState),
    fmMessageState_(fmMessageState)
{
}

EndpointPublishState & EndpointPublishState::operator=(
    EndpointPublishState const & other)
{
    if (this != &other)
    {
        state_ = other.state_;
        publishTimeout_ = other.publishTimeout_;
    }

    return *this;
}

void EndpointPublishState::StartReconfiguration(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    if (state_ != State::None)
    {
        stateUpdateContext.UpdateContextObj.EnableInMemoryUpdate();
        state_ = State::None;
    }
}

bool EndpointPublishState::OnEndpointUpdated(
    Infrastructure::IClock const & clock,
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    if (state_ == State::Completed)
    {
        return false;
    }

    if (state_ == State::None)
    {
        stateUpdateContext.UpdateContextObj.EnableInMemoryUpdate();
        state_ = State::PublishPending;
    }

    if (HasTimeoutExpired(clock))
    {
        return UpdateFMMessageState(stateUpdateContext);
    }
    else
    {
        return false;
    }
}

void EndpointPublishState::OnReconfigurationTimer(
    Infrastructure::IClock const & clock,
    Infrastructure::StateUpdateContext & stateUpdateContext,
    __out bool & sendMessage,
    __out bool & isRetryRequired)
{
    isRetryRequired = false;
    sendMessage = false;

    if (state_ != State::PublishPending)
    {
        return;
    }

    if (!HasTimeoutExpired(clock))
    {
        // If the timeout has not expired continue to wait for notification so that 
        // when the timeout expires the message can be sent
        isRetryRequired = true;
        return;
    }
    else
    {
        sendMessage = UpdateFMMessageState(stateUpdateContext);
    }
}

void EndpointPublishState::OnFmReply(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    if (state_ == State::PublishPending)
    {
        stateUpdateContext.UpdateContextObj.EnableInMemoryUpdate();
        state_ = State::Completed;
        fmMessageState_->Reset(stateUpdateContext);
    }
}

void EndpointPublishState::Clear(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    if (state_ == State::PublishPending)
    {
        stateUpdateContext.UpdateContextObj.EnableInMemoryUpdate();
        state_ = State::None;
        fmMessageState_->Reset(stateUpdateContext);
    }
}

bool EndpointPublishState::HasTimeoutExpired(Infrastructure::IClock const & clock) const
{
    return reconfigState_->GetCurrentReconfigurationDuration(clock) >= publishTimeout_->GetValue();
}

bool EndpointPublishState::UpdateFMMessageState(Infrastructure::StateUpdateContext & context)
{
    if (fmMessageState_->MessageStage != FMMessageStage::EndpointAvailable)
    {
        fmMessageState_->MarkReplicaEndpointUpdatePending(context);
        return true;
    }

    return false;
}

void EndpointPublishState::AssertInvariants(
    FailoverUnit const & owner) const
{
    TESTASSERT_IF(!owner.IsReconfiguring && IsEndpointPublishPending, "Endpoint publish pending when reconfiguring {0}", owner);
    TESTASSERT_IF(owner.LocalReplicaClosePending.IsSet && IsEndpointPublishPending, "Publish pending when closing {0}", owner);

    TESTASSERT_IF(fmMessageState_->MessageStage == FMMessageStage::EndpointAvailable && !IsEndpointPublishPending, "Incorrect fm message stage with publish pending {0}", owner);
}
