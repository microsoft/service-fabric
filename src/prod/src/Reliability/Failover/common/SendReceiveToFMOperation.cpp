// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace Federation;
using namespace std;

StringLiteral const TraceFMRequest("FMRequest");
StringLiteral const TraceFMMRequest("FMMRequest");

SendReceiveToFMOperation::SendReceiveToFMOperation(
    IReliabilitySubsystem & reliabilitySubsystem,
    std::wstring const & ackAction,
    bool isToFMM,
    DelayCalculator delayCalculator)
    : reliabilitySubsystem_(reliabilitySubsystem),
      ackAction_(ackAction),
      isToFMM_(isToFMM),
      delayCalculator_(delayCalculator),
      isCancelled_(false)
{
}

SendReceiveToFMOperation::~SendReceiveToFMOperation()
{
}

void SendReceiveToFMOperation::Send()
{
    if (IsCancelled)
    {    
        return;
    }

    if (reliabilitySubsystem_.FederationWrapperBase.IsShutdown)
    {
        return;
    }

    auto requestMessage = CreateRequestMessage();
    replyMessage_ = nullptr;

    auto sendReceiveToFMOperation = shared_from_this();
    if (isToFMM_)
    {
        reliabilitySubsystem_.FederationWrapperBase.BeginRequestToFMM(
            std::move(requestMessage),
            TimeSpan::MaxValue,
            FailoverConfig::GetConfig().SendToFMTimeout,
            [sendReceiveToFMOperation] (AsyncOperationSPtr const& routeRequestOperation)
            {
                sendReceiveToFMOperation->RouteRequestCallback(sendReceiveToFMOperation, routeRequestOperation);
            },
            reliabilitySubsystem_.Root.CreateAsyncOperationRoot());
    }
    else
    {
        reliabilitySubsystem_.FederationWrapperBase.BeginRequestToFM(
            std::move(requestMessage),
            TimeSpan::MaxValue,
            FailoverConfig::GetConfig().SendToFMTimeout,
            [sendReceiveToFMOperation] (AsyncOperationSPtr const& routeRequestOperation)
            {
                sendReceiveToFMOperation->RouteRequestCallback(sendReceiveToFMOperation, routeRequestOperation);
            },
            reliabilitySubsystem_.Root.CreateAsyncOperationRoot());
    }
}

void SendReceiveToFMOperation::RouteRequestCallback(
    shared_ptr<SendReceiveToFMOperation> const& sendReceiveToFMOperation,
    AsyncOperationSPtr const& routeRequestOperation)
{
    ErrorCode error;
    if (isToFMM_)
    {
        error = reliabilitySubsystem_.FederationWrapperBase.EndRequestToFMM(routeRequestOperation, replyMessage_);
    }
    else
    {
        error = reliabilitySubsystem_.FederationWrapperBase.EndRequestToFM(routeRequestOperation, replyMessage_);
    }
    
    if (error.IsSuccess())
    {
        WriteInfo(
            (isToFMM_ ? TraceFMMRequest : TraceFMRequest),
            "{0} received reply message {1}.",
            reliabilitySubsystem_.FederationWrapperBase.Instance, replyMessage_->Action);

        if (replyMessage_->Action == ackAction_)
        {
            OnAckReceived();
            return;
        }
    }
    else
    {
        WriteInfo(
            (isToFMM_ ? TraceFMMRequest : TraceFMRequest),
            "{0} SendReceiveToFMOperation failed with error {1}. AckAction='{2}'.",
            reliabilitySubsystem_.FederationWrapperBase.Instance, error, ackAction_);
    }

    // The last send was not successful, we need to decide when/whether to retry.
    TimeSpan delay = GetDelay();

    // Do we need to retry?
    if (delay != TimeSpan::MaxValue)
    {
        if (delay == TimeSpan::Zero)
        {
            // Retry now.
            Send();
        }
        else
        {
            // Retry after the delay.
            auto root = reliabilitySubsystem_.Root.CreateComponentRoot();
            Threadpool::Post(
                [sendReceiveToFMOperation, root] () { sendReceiveToFMOperation->RetryTimerCallback(); }, delay);
        }
    }
}

void SendReceiveToFMOperation::RetryTimerCallback()
{
    Send();
}

void SendReceiveToFMOperation::OnAckReceived()
{
}

TimeSpan SendReceiveToFMOperation::GetDelay() const
{
    return delayCalculator_.GetDelay();
}

SendReceiveToFMOperation::DelayCalculator::DelayCalculator(TimeSpan baseInterval, double maxJitter)
: baseInterval_(baseInterval),
  maxJitterInterval_(TimeSpan::FromSeconds(maxJitter * baseInterval.TotalSeconds()))
{
    ASSERT_IF(maxJitterInterval_.TotalMilliseconds() >= baseInterval_.TotalMilliseconds(), "MaxJitterInterval must be less than base interval");
    ASSERT_IF(maxJitterInterval_.TotalMilliseconds() < 0, "MaxJitterInterval must be positive");
}

TimeSpan SendReceiveToFMOperation::DelayCalculator::GetDelay() const
{
    return baseInterval_ + TimeSpan::FromSeconds(random_.NextDouble() * maxJitterInterval_.TotalSeconds());
}
