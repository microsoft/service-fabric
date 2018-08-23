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
using namespace Node;

void NodeDeactivationStateProcessor::ProcessActivationStateChange(
    std::wstring const & activityId,
    NodeDeactivationState & state,
    NodeDeactivationInfo const & newInfo)
{
    if (!state.TryStartChange(activityId, newInfo))
    {
        return;
    }

    auto fts = ra_.LocalFailoverUnitMapObj.GetFailoverUnitEntries(state.FailoverManagerId);

    auto work = make_shared<MultipleEntityWork>(
        activityId,
        [this, newInfo, &state](MultipleEntityWork & innerWork, ReconfigurationAgent & )
        {
            OnFailoverUnitUpdateComplete(innerWork, state, newInfo);
        });

    auto handler = [this, newInfo, &state](HandlerParameters & handlerParameters, JobItemContextBase & )
    {
        return UpdateFailoverUnit(handlerParameters, state, newInfo);
    };

    ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(
        work,
        fts,
        [handler](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & innerWork)
        {
            NodeActivateDeactivateJobItem::Parameters parameters(
                entry,
                innerWork,
                handler,
                JobItemCheck::DefaultAndOpen,
                *JobItemDescription::NodeActivationDeactivation);

            return make_shared<NodeActivateDeactivateJobItem>(move(parameters));
        });
}

bool NodeDeactivationStateProcessor::UpdateFailoverUnit(
    HandlerParameters & handlerParameters,
    NodeDeactivationState  & state,
    NodeDeactivationInfo const & info)
{
    handlerParameters.AssertFTIsOpen();
    handlerParameters.AssertFTExists();

    if (!state.IsCurrent(info.SequenceNumber))
    {
        return false;
    }

    if (info.IsActivated)
    {
        ra_.ReopenDownReplica(handlerParameters);
    }
    else
    {
        ra_.CloseLocalReplica(handlerParameters, ReplicaCloseMode::DeactivateNode, ActivityDescription::Empty);
    }

    return true;
}

void NodeDeactivationStateProcessor::OnFailoverUnitUpdateComplete(
    Infrastructure::MultipleEntityWork & work,
    NodeDeactivationState & state,
    NodeDeactivationInfo const & info)
{
    auto fts = work.GetEntities();

    state.FinishChange(work.ActivityId, info, move(fts));
}
