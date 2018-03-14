// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            // This class controls execution of any background work that needs to be done in RA
            // Multiple threads can request the background work to be processed
            // Whenever background work is requested it will immediately be enqueued onto the common queue
            // If a type of background work is being executed then once the current execution completes 
            // another instance of the background work will be enqueued onto the common queue
            // If we already have the background work executing and pending to be executed then
            // a request to execute the background work will be ignored
            // Once the work function has been called the BGM must be told that the work function has completed execution
            // The BGM does not care whether execution happened sync or async 
            // It is safe to call OnWorkComplete from the work function itself
            class BackgroundWorkManager
            {
                DENY_COPY(BackgroundWorkManager)
            public:
                typedef std::function<void(std::wstring const &, ReconfigurationAgent&, BackgroundWorkManager&)> WorkFunctionPointer;

                BackgroundWorkManager(
                    std::wstring const & id,
                    WorkFunctionPointer const & workFunction,
                    TimeSpanConfigEntry const & minIntervalBetweenWork,
                    ReconfigurationAgent & ra);

                ~BackgroundWorkManager();

                void Request(std::wstring const & activityId);

                void Request(std::wstring && activityId);

                void Close();

                void OnWorkComplete();

            private:
                void EnqueueWork();

                void Process(ReconfigurationAgent&);

                void ProcessQueuedExecution_UnderLock(std::wstring && activityId);

                void OnTimer();

                std::wstring const id_;

                // Indicates that the work item has been enqueued in the queue/execution is going on
                bool isExecuting_;
                bool isPending_;

                // Indicates that the process function was called and the BGM is now waiting for wait completion            
                bool isWaitingForCompletion_;

                bool isClosed_;

                Common::StopwatchTime lastExecutionStartTimeStamp_;

                Common::TimerSPtr timer_;
                std::wstring pendingActivityId_;
                std::wstring processingActivityId_;
                Common::ExclusiveLock lock_;
                WorkFunctionPointer workFunction_;
                ReconfigurationAgent & ra_;
                TimeSpanConfigEntry const & minIntervalBetweenWork_;
            };
        }
    }
}
