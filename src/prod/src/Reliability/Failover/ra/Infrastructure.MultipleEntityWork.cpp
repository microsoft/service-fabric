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

namespace
{
    StringLiteral const CtorTag("ctor");
    StringLiteral const DtorTag("dtor");
}

MultipleEntityWork::MultipleEntityWork(wstring const & activityId, MultipleEntityWork::CompleteFunctionPtr completeFunction)
    : activityId_(activityId),
      completeFunction_(completeFunction),
      completed_(0),
      hasStarted_(false)
{
    RAEventSource::Events->MultipleFTWorkLifecycle(activityId_, CtorTag);
}

MultipleEntityWork::MultipleEntityWork(wstring const & activityId)    
    : activityId_(activityId),      
      completed_(0),
      hasStarted_(false)
{
    RAEventSource::Events->MultipleFTWorkLifecycle(activityId_, CtorTag);
}

MultipleEntityWork::~MultipleEntityWork()
{
    RAEventSource::Events->MultipleFTWorkLifecycle(activityId_, DtorTag);
}

void MultipleEntityWork::AddFailoverUnitJob(EntityJobItemBaseSPtr const & job)
{
    ASSERT_IF(hasStarted_, "Cannot add ft job items to multiple ft work if execution has started");
    jobItems_.push_back(job);
}

void MultipleEntityWork::AddFailoverUnitJob(EntityJobItemBaseSPtr && job)
{
    ASSERT_IF(hasStarted_, "Cannot add ft job items to multiple ft work if execution has started");
    jobItems_.push_back(move(job));
}

void MultipleEntityWork::OnExecutionStarted()
{
    ASSERT_IF(hasStarted_, "Cannot start execution twice");
    ASSERT_IF(jobItems_.empty(), "cannot execute empty ft work");
    hasStarted_ = true;
    completed_.store(static_cast<uint64>(jobItems_.size()));
}

bool MultipleEntityWork::OnFailoverUnitJobComplete()
{
    ASSERT_IF(!hasStarted_, "Work cannot be complete if execution did not start");
    ASSERT_IF(completed_.load() == 0, "cannot complete more job items than started");
    return --completed_ == 0;
}
