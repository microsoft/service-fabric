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

NodeUpOperationSPtr NodeUpOperation::Create(
    IReliabilitySubsystem & reliabilitySubsystem,
    vector<ServicePackageInfo> && packages,
    bool isToFMM,
    bool anyReplicaFound)
{
    return shared_ptr<NodeUpOperation>(new NodeUpOperation(reliabilitySubsystem, move(packages), isToFMM, anyReplicaFound));
}

NodeUpOperation::NodeUpOperation(
    IReliabilitySubsystem & reliabilitySubsystem,
    vector<ServicePackageInfo> && packages,
    bool isToFMM,
    bool anyReplicaFound)
    : SendReceiveToFMOperation(
        reliabilitySubsystem, 
        RSMessage::GetNodeUpAck().Action, 
        isToFMM,
        DelayCalculator(FailoverConfig::GetConfig().NodeUpRetryInterval, FailoverConfig::GetConfig().MaxJitterForSendToFMRetry)),
      packages_(move(packages)),
      anyReplicaFound_(anyReplicaFound)
{
}

MessageUPtr NodeUpOperation::CreateRequestMessage() const
{
    NodeUpMessageBody body(
        packages_,
        reliabilitySubsystem_.GetNodeDescription(),
        anyReplicaFound_);

    return RSMessage::GetNodeUp().CreateMessage(body);
}

void NodeUpOperation::OnAckReceived()
{
    reliabilitySubsystem_.ProcessNodeUpAck(move(replyMessage_), IsToFMM);
}
