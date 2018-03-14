// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace std;

LFUMUploadOperation::LFUMUploadOperation(
    Reliability::IReliabilitySubsystem & reliabilitySubsystem,
    Reliability::GenerationNumber const & generation,
    Reliability::NodeDescription && node,
    std::vector<FailoverUnitInfo> && failoverUnitInfos,
    bool isToFMM,
    bool anyReplicaFound)
    : SendReceiveToFMOperation(
        reliabilitySubsystem, 
        RSMessage::GetLFUMUploadReply().Action, 
        isToFMM,
        DelayCalculator(FailoverConfig::GetConfig().LfumUploadRetryInterval, FailoverConfig::GetConfig().MaxJitterForSendToFMRetry)),
      body_(generation, move(node), move(failoverUnitInfos), anyReplicaFound)
{
}

MessageUPtr LFUMUploadOperation::CreateRequestMessage() const
{
    return RSMessage::GetLFUMUpload().CreateMessage(body_);
}

void LFUMUploadOperation::OnAckReceived()
{
    GenerationHeader generationHeader = GenerationHeader::ReadFromMessage(*replyMessage_);

    reliabilitySubsystem_.ProcessLFUMUploadReply(generationHeader);
}
