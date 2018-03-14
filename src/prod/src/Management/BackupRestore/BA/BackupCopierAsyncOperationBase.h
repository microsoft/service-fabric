// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        namespace BackupCopier
        {
            class BackupCopierAsyncOperationBase : public AsyncOperation
            {
            private:
                typedef function<void(void)> JobQueueItemCompletionCallback;

            public:
                BackupCopierAsyncOperationBase(
                    __in BackupCopierProxy & owner,
                    Common::ActivityId const & activityId,
                    TimeSpan const timeout,
                    AsyncCallback const & callback,
                    AsyncOperationSPtr const & parent);

                __declspec(property(get = get_BackupCopierProxy)) BackupCopierProxy & Owner;
                BackupCopierProxy & get_BackupCopierProxy() { return owner_; }

                __declspec(property(get = get_TraceId)) wstring const & TraceId;
                wstring const & get_TraceId() const { return traceId_; }

                __declspec(property(get = get_ActivityId)) Common::ActivityId const & ActivityId;
                Common::ActivityId const & get_ActivityId() const { return activityId_; }

                __declspec(property(get = get_Timeout)) TimeSpan const Timeout;
                TimeSpan get_Timeout() const { return timeoutHelper_.GetRemainingTime(); }

                void OnStart(AsyncOperationSPtr const &) override;

                virtual void OnProcessJob(AsyncOperationSPtr const &) = 0;

                void OnCompleted() override;

                void SetJobQueueItemCompletion(JobQueueItemCompletionCallback const & callback);

            private:

                BackupCopierProxy & owner_;
                wstring traceId_;
                Common::ActivityId activityId_;
                TimeoutHelper timeoutHelper_;

                JobQueueItemCompletionCallback jobCompletionCallback_;
            };
        }
    }
}
