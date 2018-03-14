// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"

using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace Federation;
using namespace std;

namespace
{
    StringLiteral const TraceFMRequest("FMRequest");
    StringLiteral const TraceFMMRequest("FMMRequest");
}

ChangeNotificationOperationSPtr ChangeNotificationOperation::Create(IReliabilitySubsystem & reliabilitySubsystem, bool isToFMM)
{
    return shared_ptr<ChangeNotificationOperation>(new ChangeNotificationOperation(reliabilitySubsystem, isToFMM));
}

ChangeNotificationOperation::ChangeNotificationOperation(IReliabilitySubsystem & reliabilitySubsystem, bool isToFMM)
    : SendReceiveToFMOperation(
        reliabilitySubsystem, 
        RSMessage::GetChangeNotificationAck().Action, 
        isToFMM,
        DelayCalculator(FailoverConfig::GetConfig().ChangeNotificationRetryInterval, FailoverConfig::GetConfig().MaxJitterForSendToFMRetry))
{
}

MessageUPtr ChangeNotificationOperation::CreateRequestMessage() const
{
    vector<NodeInstance> shutdownNodes;
    RoutingToken token = reliabilitySubsystem_.Federation.GetRoutingTokenAndShutdownNodes(shutdownNodes);

    if (IsChallenged())
    {
        ChangeNotificationChallenge replyBody;
        if (replyMessage_->GetBody(replyBody))
        {
            for (NodeInstance const& node : replyBody.Instances)
            {
                if (token.Contains(node.Id))
                {
                    shutdownNodes.push_back(node);
                }
            }
        }
        else
        {
            SendReceiveToFMOperation::WriteWarning(
                (IsToFMM ? TraceFMMRequest : TraceFMRequest),
                "Ignored ChangeNotificationChallenge message because deserialization of message failed with {0}", replyMessage_->Status);
        }
    }
    ChangeNotificationMessageBody body(token, move(shutdownNodes));

    return RSMessage::GetChangeNotification().CreateMessage(body);
}

TimeSpan ChangeNotificationOperation::GetDelay() const
{
    if (IsChallenged())
    {
        return TimeSpan::Zero;
    }

    return SendReceiveToFMOperation::GetDelay();
}

bool ChangeNotificationOperation::IsChallenged() const
{
    return (replyMessage_ != nullptr && replyMessage_->Action == RSMessage::GetChangeNotificationChallenge().Action);
}
