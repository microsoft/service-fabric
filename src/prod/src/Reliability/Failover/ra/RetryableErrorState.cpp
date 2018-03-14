// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

RetryableErrorState::RetryableErrorState()
: currentState_(RetryableErrorStateName::None),
  currentFailureCount_(0),
  thresholds_(RetryableErrorStateThresholdCollection::CreateDefault())
{
}

void RetryableErrorState::EnterState(RetryableErrorStateName::Enum state)
{
    currentState_ = state;
    currentFailureCount_ = 0;
}

RetryableErrorAction::Enum RetryableErrorState::OnFailure(FailoverConfig const & config)
{
    currentFailureCount_++;

    auto const & currentThresholds = GetCurrentThresholds(config);
    if (currentFailureCount_ == currentThresholds.WarningThreshold)
    {
        return RetryableErrorAction::ReportHealthWarning;
    }
    else if (currentFailureCount_ == currentThresholds.DropThreshold)
    {
        return RetryableErrorAction::Drop;
    }
    else if (currentFailureCount_ == currentThresholds.RestartThreshold)
    {
        return RetryableErrorAction::Restart;
    }
    else if (currentFailureCount_ == currentThresholds.ErrorThreshold)
    {
        return RetryableErrorAction::ReportHealthError;
    }
    else
    {
        return RetryableErrorAction::None;
    }
}

RetryableErrorAction::Enum RetryableErrorState::OnSuccessAndTransitionTo(
    RetryableErrorStateName::Enum state,
    FailoverConfig const & config)
{
    RetryableErrorAction::Enum rv = RetryableErrorAction::None;

    auto const & currentThresholds = GetCurrentThresholds(config);
    if (currentFailureCount_ >= currentThresholds.WarningThreshold || currentFailureCount_ >= currentThresholds.ErrorThreshold)
    {
        rv = RetryableErrorAction::ClearHealthReport;
    }

    EnterState(state);

    return rv;
}

bool RetryableErrorState::IsLastRetryBeforeDrop(FailoverConfig const & config) const
{
    auto const & currentThresholds = GetCurrentThresholds(config);
    
    return currentFailureCount_ == currentThresholds.DropThreshold - 1;
}

void RetryableErrorState::Reset()
{
    EnterState(RetryableErrorStateName::None);
}

RetryableErrorStateThresholdCollection::Threshold RetryableErrorState::GetCurrentThresholds(FailoverConfig const & config) const
{
    TESTASSERT_IF(currentState_ == RetryableErrorStateName::None, "Invalid current state");
    return thresholds_.GetThreshold(currentState_, config);
}

void RetryableErrorState::Test_Set(RetryableErrorStateName::Enum state, int currentCount)
{
    currentState_ = state;
    currentFailureCount_ = currentCount;
}

void RetryableErrorState::Test_SetThresholds(RetryableErrorStateName::Enum state, int warningThreshold, int dropThreshold, int restartThreshold, int errorThreshold)
{
    auto & collection = const_cast<RetryableErrorStateThresholdCollection&>(thresholds_);
    collection.AddThreshold(state, warningThreshold, dropThreshold, restartThreshold, errorThreshold);
}

std::string RetryableErrorState::GetTraceState(FailoverConfig const & config) const
{
    if (currentState_ != RetryableErrorStateName::None)
    {
        return formatString("[{0}/{1}]", currentFailureCount_, GetCurrentThresholds(config).DropThreshold);
    }

    return "";
}
