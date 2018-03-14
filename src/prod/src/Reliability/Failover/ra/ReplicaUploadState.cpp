// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

ReplicaUploadState::ReplicaUploadState(FMMessageState * fmMessageState, FailoverManagerId const & owner) :
fmMessageState_(fmMessageState),
replicaUploadPending_(EntitySetIdentifier(EntitySetName::ReplicaUploadPending, owner)),
uploadRequested_(false)
{
}

ReplicaUploadState::ReplicaUploadState(FMMessageState * fmMessageState, ReplicaUploadState const & other) :
fmMessageState_(fmMessageState),
replicaUploadPending_(other.replicaUploadPending_),
uploadRequested_(other.uploadRequested_)
{
}

ReplicaUploadState & ReplicaUploadState::operator=(ReplicaUploadState const & other)
{
    if (this != &other)
    {
        replicaUploadPending_ = other.replicaUploadPending_;
        uploadRequested_ = other.uploadRequested_;
    }

    return *this;
}

void ReplicaUploadState::MarkUploadPending(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    TESTASSERT_IF(IsUploadPending, "Cannot mark upload pending twice");

    stateUpdateContext.UpdateContextObj.EnableInMemoryUpdate();
    replicaUploadPending_.SetValue(true, stateUpdateContext.ActionQueue);
}

void ReplicaUploadState::UploadReplica(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    if (!IsUploadPending || uploadRequested_)
    {
        return;
    }

    stateUpdateContext.UpdateContextObj.EnableInMemoryUpdate();
    uploadRequested_ = true;
    fmMessageState_->OnLastReplicaUpPending(stateUpdateContext);
}

void ReplicaUploadState::OnUploaded(
    Infrastructure::StateUpdateContext & stateUpdateContext)
{
    if (!IsUploadPending)
    {
        return;
    }

    stateUpdateContext.UpdateContextObj.EnableInMemoryUpdate();
    replicaUploadPending_.SetValue(false, stateUpdateContext.ActionQueue);
    fmMessageState_->OnLastReplicaUpAcknowledged(stateUpdateContext);
}
