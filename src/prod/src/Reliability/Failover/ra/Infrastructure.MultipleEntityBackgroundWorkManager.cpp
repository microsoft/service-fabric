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

#pragma warning(push)
#pragma warning(disable: 4355)
MultipleEntityBackgroundWorkManager::MultipleEntityBackgroundWorkManager(MultipleEntityBackgroundWorkManager::Parameters const & parameters)
: id_(parameters.Id),
  set_(EntitySet::Parameters(parameters.Id, parameters.SetIdentifier, parameters.PerformanceCounter)),
  bgmr_(
  parameters.Id, 
  parameters.Name,
  [this] (wstring const & activityId, ReconfigurationAgent&, BackgroundWorkManagerWithRetry&)
  {
      this->BGMCallback_Internal(activityId);  
  },
  *parameters.MinIntervalBetweenWork,        
  *parameters.RetryInterval,
  *parameters.RA),
  ra_(*parameters.RA),
  completeFunction_(parameters.CompleteFunction),
  factoryFunction_(parameters.FactoryFunction)
{
    ASSERT_IF(factoryFunction_ == nullptr, "A factory function must exist");
    ASSERT_IF(parameters.SetCollection == nullptr, "A set collection must be defined");
    parameters.SetCollection->Add(parameters.SetIdentifier, set_, &bgmr_.RetryTimerObj);
}
#pragma warning(pop)

MultipleEntityBackgroundWorkManager::~MultipleEntityBackgroundWorkManager()
{
}

void MultipleEntityBackgroundWorkManager::Close()
{
    bgmr_.Close();
}

void MultipleEntityBackgroundWorkManager::Request(StateMachineActionQueue & queue)
{
    queue.Enqueue(make_unique<ProcessRequestStateMachineAction>(*this));
}

void MultipleEntityBackgroundWorkManager::Request(wstring const & activityId)
{
    bgmr_.Request(activityId);
}

void MultipleEntityBackgroundWorkManager::BGMCallback_Internal(wstring const & activityId)
{
    auto entities = set_.GetEntities();
    
    MultipleEntityWorkSPtr multipleFTWork = make_shared<MultipleEntityWork>(
        activityId, 
        [this] (MultipleEntityWork & work, ReconfigurationAgent& ra)
        {
            OnCompleteCallbackHelper(work, ra);
        });

    ra_.JobQueueManager.CreateJobItemsAndStartMultipleFailoverUnitWork(multipleFTWork, entities, factoryFunction_);
}

void MultipleEntityBackgroundWorkManager::OnCompleteCallbackHelper(MultipleEntityWork & work, ReconfigurationAgent & ra)
{
    // execute completion callback if it exists
    if (completeFunction_ != nullptr)
    {
        completeFunction_(work, ra);
    }

    bool isRetryRequired = !set_.IsEmpty;
    bgmr_.OnWorkComplete(isRetryRequired);
}

MultipleEntityBackgroundWorkManager::ProcessRequestStateMachineAction::ProcessRequestStateMachineAction(MultipleEntityBackgroundWorkManager & owner)
: owner_(owner)
{
}

void MultipleEntityBackgroundWorkManager::ProcessRequestStateMachineAction::OnPerformAction(
    std::wstring const & activityId, 
    Infrastructure::EntityEntryBaseSPtr const &, 
    ReconfigurationAgent &)
{
    owner_.bgmr_.Request(activityId);
}

void MultipleEntityBackgroundWorkManager::ProcessRequestStateMachineAction::OnCancelAction(ReconfigurationAgent&) 
{
}
