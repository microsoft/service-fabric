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
using namespace ReplicaUp;
using namespace Infrastructure;

ReplicaUpMessageBuilder::ReplicaUpMessageBuilder(ReplicaUpSendFunctionPtr sendFunc)
: sendFunc_(sendFunc)
{
}

void ReplicaUpMessageBuilder::ProcessJobItem(ReplicaUpMessageRetryJobItem & jobItem)
{
    if (jobItem.Context.TryAddToMessage(upReplicas_, droppedReplicas_))
    {
        ftsInMessage_.insert(jobItem.Id);
    }
}

void ReplicaUpMessageBuilder::FinalizeAndSend(LastReplicaUpState const & lastReplicaUpState, ReconfigurationAgent & ra)
{
    // Get the value of the last replica up flag
    bool lastReplicaUp = lastReplicaUpState.ComputeLastReplicaUpFlagValue(ftsInMessage_);

    if (!ShouldSendMessage(lastReplicaUp))
    {
        return;
    }

    ASSERT_IF(upReplicas_.size() + droppedReplicas_.size() != ftsInMessage_.size(), "Size must match");
    
    ReplicaUpMessageBody body(move(upReplicas_), move(droppedReplicas_), lastReplicaUp);
    sendFunc_(ra.Federation, body);    
}

bool ReplicaUpMessageBuilder::ShouldSendMessage(bool lastReplicaUp) const
{
    if (lastReplicaUp)
    {
        return true;
    }

    return !ftsInMessage_.empty();
}
