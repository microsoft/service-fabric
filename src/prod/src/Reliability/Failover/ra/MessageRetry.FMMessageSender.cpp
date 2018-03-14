// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace MessageRetry;
using ReconfigurationAgentComponent::Communication::FMMessageBuilder;
using ReconfigurationAgentComponent::Communication::FMTransport;
using ReconfigurationAgentComponent::Communication::FMMessageData;
using ReconfigurationAgentComponent::Communication::FMMessageDescription;
using ReconfigurationAgentComponent::Node::PendingReplicaUploadStateProcessor;

FMMessageSender::FMMessageSender(
    PendingReplicaUploadStateProcessor const & pendingReplicaUploadState,
    FMTransport & transport,
    FailoverManagerId const & target) :
    transport_(transport),
    target_(target),
    pendingReplicaUploadState_(pendingReplicaUploadState)
{
}

void FMMessageSender::Send(
    std::wstring const & activityId,
    __inout std::vector<FMMessageDescription> & messages,
    __inout Diagnostics::FMMessageRetryPerformanceData & perfData)
{
    perfData.OnMessageSendStart();

    FMMessageBuilder builder(transport_, target_);

    for (auto & it : messages)
    {
        builder.Send(activityId, it);
    }

    Infrastructure::EntityEntryBaseSet entitiesInReplicaUpMessage;
    builder.TakeEntriesInReplicaUpMessage(entitiesInReplicaUpMessage);

    auto isLastReplicaUp = pendingReplicaUploadState_.IsLastReplicaUpMessage(entitiesInReplicaUpMessage);
    builder.Finalize(activityId, isLastReplicaUp);

    perfData.OnMessageSendFinish();
}
