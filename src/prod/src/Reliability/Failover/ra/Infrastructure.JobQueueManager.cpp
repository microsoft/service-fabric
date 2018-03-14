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
using namespace Infrastructure;

RAJobQueueManager::RAJobQueueManager(ReconfigurationAgent & root)
    : multipleEntityWorkManager_(root),
      ra_(root)
{
}

RAJobQueueManager::~RAJobQueueManager()
{
}

void RAJobQueueManager::ScheduleJobItem(EntityJobItemBaseSPtr const & job)
{
    ASSERT_IF(job == nullptr, "job cannot be null");
    ASSERT_IF(job->Work != nullptr && !job->Work->HasStarted, "Job should be scheduled using ExecuteMultipleFailoverUnitWork");
    auto entry = job->EntrySPtr;

    entry->BeginExecuteJobItem(
        entry,
        job,
        ra_,
        [this, entry](AsyncOperationSPtr const & inner)
        {
            vector<MultipleEntityWorkSPtr> completedWork;
            auto error = entry->EndExecuteJobItem(inner, completedWork);
            ASSERT_IF(!error.IsSuccess(), "Op must complete with success");

            for (auto const & innerWork : completedWork)
            {
                multipleEntityWorkManager_.OnFailoverUnitJobItemExecutionCompleted(innerWork);
            }
        },
        ra_.Root.CreateAsyncOperationRoot());
}

void RAJobQueueManager::ScheduleJobItem(EntityJobItemBaseSPtr && job)
{
    auto innerJob = move(job);
    ScheduleJobItem(innerJob);
}

void RAJobQueueManager::CreateJobItemsAndStartMultipleFailoverUnitWork(
    MultipleEntityWorkSPtr const & work,
    EntityEntryBaseList const & entries,
    MultipleEntityWork::FactoryFunctionPtr factory)
{
    for (auto const & entry : entries)
    {
        auto ji = factory(entry, work);
        work->AddFailoverUnitJob(ji);
    }

    multipleEntityWorkManager_.Execute(work);
}

void RAJobQueueManager::AddJobItemsAndStartMultipleFailoverUnitWork(
    MultipleEntityWorkSPtr const & work,
    std::vector<std::tuple<EntityEntryBaseSPtr, EntityJobItemBaseSPtr>> const & jobItems)
{
    for (auto const & item : jobItems)
    {
        work->AddFailoverUnitJob(get<1>(item));
    }

    multipleEntityWorkManager_.Execute(work);
}

void RAJobQueueManager::ExecuteMultipleFailoverUnitWork(MultipleEntityWorkSPtr const & work)
{
    multipleEntityWorkManager_.Execute(work);
}

void RAJobQueueManager::ExecuteMultipleFailoverUnitWork(MultipleEntityWorkSPtr && work)
{
    multipleEntityWorkManager_.Execute(move(work));
}

void RAJobQueueManager::Close()
{
}
