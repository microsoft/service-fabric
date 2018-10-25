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

MultipleReplicaCloseAsyncOperation::MultipleReplicaCloseAsyncOperation(
    Parameters && parameters,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent) :
    AsyncOperation(callback, parent, true)
{    
    parameters_.Clock = move(parameters.Clock);
    parameters_.RA = parameters.RA;
    parameters_.ActivityId = move(parameters.ActivityId);
    parameters_.FTsToClose = move(parameters.FTsToClose);
    parameters_.Check = static_cast<JobItemCheck::Enum>(parameters.JobItemCheck | JobItemCheck::FTIsNotNull);
    parameters_.CloseMode = parameters.CloseMode;
    parameters_.MonitoringIntervalEntry = parameters.MonitoringIntervalEntry;
    parameters_.MaxWaitBeforeTerminationEntry = parameters.MaxWaitBeforeTerminationEntry;
    parameters_.Callback = move(parameters.Callback);
    parameters_.TerminateReason = parameters.TerminateReason;
    parameters_.IsReplicaClosedFunction = move(parameters.IsReplicaClosedFunction);
}

void MultipleReplicaCloseAsyncOperation::OnStart(Common::AsyncOperationSPtr const & thisSPtr)
{
    if (parameters_.FTsToClose.empty())
    {
        TryComplete(thisSPtr);
        return;
    }

    auto work = make_shared<MultipleEntityWork>(
        parameters_.ActivityId,
        [this, thisSPtr](MultipleEntityWork const &, ReconfigurationAgent &)
        {
            OnCloseCompleted(thisSPtr);
        });

    auto handler = [this](HandlerParameters & handlerParameters, JobItemContextBase & context)
    {
        return Processor(handlerParameters, context);
    };

    parameters_.RA->JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(
        work,
        parameters_.FTsToClose,
        [handler, this](EntityEntryBaseSPtr const & entry, MultipleEntityWorkSPtr const & work)
        {
            FabricUpgradeSendCloseMessageJobItem::Parameters jobItemParameters(
                entry,
                work,
                handler,
                parameters_.Check,
                *JobItemDescription::CloseFailoverUnitJobItem,
                move(JobItemContextBase()));

            return make_shared<FabricUpgradeSendCloseMessageJobItem>(move(jobItemParameters));
        });
}

bool MultipleReplicaCloseAsyncOperation::Processor(
    Infrastructure::HandlerParameters & handlerParameters,
    JobItemContextBase &)
{
    handlerParameters.AssertFTExists();

    parameters_.RA->CloseLocalReplica(handlerParameters, parameters_.CloseMode, ActivityDescription::Empty);

    return true;
}

void MultipleReplicaCloseAsyncOperation::OnCloseCompleted(
    AsyncOperationSPtr const & thisSPtr)
{
    if (IsCancelRequested)
    {
        TryComplete(thisSPtr);
        return;
    }

    // if there is a racing cancel call then the child should see the cancel always
    auto op = AsyncOperation::CreateAndStart<MultipleReplicaCloseCompletionCheckAsyncOperation>(
        move(parameters_),
        [this](AsyncOperationSPtr const & inner)
        {
            auto err = MultipleReplicaCloseCompletionCheckAsyncOperation::End(inner);
            TryComplete(inner->Parent, err);
        },
        thisSPtr);

    completionCheckAsyncOp_ = dynamic_pointer_cast<MultipleReplicaCloseCompletionCheckAsyncOperation>(op);
}

void MultipleReplicaCloseAsyncOperation::OnCancel() 
{
    // Child is automatically notified
    // The only place where this op can do anything with cancel 
    // is after the initial close completes where it can skip 
}
