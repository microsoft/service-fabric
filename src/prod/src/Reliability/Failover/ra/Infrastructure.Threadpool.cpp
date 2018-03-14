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

    wstring GetMessageJobQueueName(ReconfigurationAgent & ra) { return GetJobQueueName(ra, L"_message"); }
}

IThreadpool::IThreadpool()
{
}

IThreadpool::~IThreadpool()
{
}

Infrastructure::Threadpool::Threadpool(ReconfigurationAgent & ra) :
    messageQueue_(GetMessageJobQueueName(ra), ra, false, ra.Config.MessageProcessingQueueThreadCount, JobQueuePerfCounters::CreateInstance(GetMessageJobQueueName(ra))),
    entityJobQueue_(L"_entity", ra, true, ra.Config.FailoverUnitProcessingQueueThreadCount),
    commitCallbackJobQueue_(L"_commitcallback", ra, true, ra.Config.FailoverUnitProcessingQueueThreadCount)
{
}

bool Infrastructure::Threadpool::EnqueueIntoMessageQueue(MessageProcessingJobItem<ReconfigurationAgent> & job)
{
    return messageQueue_.Enqueue(move(job));
}

void Infrastructure::Threadpool::ExecuteOnThreadpool(ThreadpoolCallback callback)
{
    Common::Threadpool::Post(callback);
}

void Infrastructure::Threadpool::Close()
{
    messageQueue_.Close();
    entityJobQueue_.Close();
    commitCallbackJobQueue_.Close();
}

AsyncOperationSPtr Infrastructure::Threadpool::BeginScheduleEntity(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) 
{
    return entityJobQueue_.BeginSchedule(callback, parent);
}

ErrorCode Infrastructure::Threadpool::EndScheduleEntity(AsyncOperationSPtr const & op) 
{
    return entityJobQueue_.EndSchedule(op);
}

AsyncOperationSPtr Infrastructure::Threadpool::BeginScheduleCommitCallback(
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return commitCallbackJobQueue_.BeginSchedule(callback, parent);
}

ErrorCode Infrastructure::Threadpool::EndScheduleCommitCallback(
    AsyncOperationSPtr const & op)
{
    return commitCallbackJobQueue_.EndSchedule(op);
}
