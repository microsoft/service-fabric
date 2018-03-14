// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            class AsyncJobQueue
            {
                DENY_COPY(AsyncJobQueue);
            public:
                AsyncJobQueue(
                    std::wstring const & name,
                    ReconfigurationAgent & ra,
                    bool forceEnqueue,
                    int count);

                virtual ~AsyncJobQueue() {}

                Common::AsyncOperationSPtr BeginSchedule(
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent);

                Common::ErrorCode EndSchedule(
                    Common::AsyncOperationSPtr const & op);

                void Close();
            private:
                class JobItem : public Common::AsyncOperation
                {
                    DENY_COPY(JobItem);
                public:
                    JobItem(
                        Common::AsyncCallback const & callback,
                        Common::AsyncOperationSPtr const & parent) :
                        Common::AsyncOperation(callback, parent)
                    {
                        stopwatch_.Start();
                    }

                    bool ProcessJob(ReconfigurationAgent & )
                    {
                        bool completed = TryComplete(shared_from_this());
                        ASSERT_IF(!completed, "Op must complete");
                        return true;
                    }

                    void UpdatePerfCounters(Common::JobQueuePerfCounters &perfCounter)
                    {
                        perfCounter.AverageTimeSpentInQueueBase.Increment();
                        perfCounter.AverageTimeSpentInQueue.IncrementBy(stopwatch_.ElapsedMilliseconds);
                    }

                protected:
                    void OnStart(Common::AsyncOperationSPtr const &) override {}

                    Common::Stopwatch stopwatch_;
                };

                typedef Common::JobQueue<std::shared_ptr<JobItem>, ReconfigurationAgent> JobQueueType;
                JobQueueType jobQueue_;
            };
        }
    }
}

