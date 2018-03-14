// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Infrastructure;

MultipleEntityWorkManager::MultipleEntityWorkManager(ReconfigurationAgent & ra)
: ra_(ra)
{
}

void MultipleEntityWorkManager::Execute(MultipleEntityWorkSPtr && work)
{
    auto local = move(work);
    Execute(local);
}

void MultipleEntityWorkManager::Execute(MultipleEntityWorkSPtr const & work)
{
    RAEventSource::Events->MultipleFTWorkBegin(ra_.NodeInstanceIdStr, work->ActivityId, static_cast<uint64>(work->JobItems.size()));

    if (work->JobItems.empty())
    {
        EnqueueCompletionCallback(work);
    }
    else
    {
        work->OnExecutionStarted();

        for(auto const & it : work->JobItems)
        {            
            ra_.JobQueueManager.ScheduleJobItem(it);
        }
    }
}

void MultipleEntityWorkManager::OnFailoverUnitJobItemExecutionCompleted(MultipleEntityWorkSPtr const & work)
{
    ASSERT_IF(work == nullptr, "Parent must be supplied");

    if (work->OnFailoverUnitJobComplete())
    {
        EnqueueCompletionCallback(work);
    }
}

void MultipleEntityWorkManager::EnqueueCompletionCallback(
    MultipleEntityWorkSPtr const & work)
{
    if (!work->HasCompletionCallback)
    {
        RAEventSource::Events->MultipleFTWorkComplete(ra_.NodeInstanceIdStr, work->ActivityId);
        return;
    }

    auto root = ra_.Root.CreateComponentRoot();

    RAEventSource::Events->MultipleFTWorkCompletionCallbackEnqueued(ra_.NodeInstanceIdStr, work->ActivityId);

    // Think long and hard before moving this off the threadpool
    // Unfortunately there are dependencies in the upgrade code where we need to be 
    // on a separate thread -> this information will need to be flown back properly
    ra_.Threadpool.ExecuteOnThreadpool([this, root, work]
    {
        work->OnCompleteCallback(*work, ra_);
    });
}

void MultipleEntityWorkManager::Close()
{
}
