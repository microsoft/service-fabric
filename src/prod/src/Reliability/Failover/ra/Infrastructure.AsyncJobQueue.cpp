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

namespace
{
    wstring GetJobQueueName(ReconfigurationAgent & ra, wstring const & queueName)
    {
        return ra.NodeInstanceIdStr + queueName;
    }
}

AsyncJobQueue::AsyncJobQueue(wstring const & name, ReconfigurationAgent & ra, bool forceEnqueue, int count) :
    jobQueue_(GetJobQueueName(ra, name), ra, forceEnqueue, count, JobQueuePerfCounters::CreateInstance(GetJobQueueName(ra, name)))
{    
}

AsyncOperationSPtr AsyncJobQueue::BeginSchedule(AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    auto op = AsyncOperation::CreateAndStart<JobItem>(callback, parent);

    jobQueue_.Enqueue(static_pointer_cast<JobItem>(op));

    return op;
}

ErrorCode AsyncJobQueue::EndSchedule(AsyncOperationSPtr const & op)
{
    return AsyncOperation::End<AsyncOperation>(op)->Error;
}

void AsyncJobQueue::Close()
{
    jobQueue_.Close();
}
