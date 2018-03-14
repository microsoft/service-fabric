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
            template<typename T>
            class ScheduleAndExecuteEntityJobItemAsyncOperation : public Common::AsyncOperation
            {
                DENY_COPY(ScheduleAndExecuteEntityJobItemAsyncOperation);

            public:
                typedef EntityScheduler<T> EntitySchedulerType;
                typedef EntityState<T> EntityStateType;
                typedef EntityMap<T> EntityMapType;
                typedef LockedEntityPtr<T> LockedEntityPtrType;
                typedef EntityJobItemListExecutorAsyncOperation<T> EntityJobItemListExecutorAsyncOperationType;
                typedef EntityEntry<T> EntityEntryType;
                typedef std::shared_ptr<EntityEntryType> EntityEntryTypeSPtr;

                ScheduleAndExecuteEntityJobItemAsyncOperation(
                    EntityEntryBaseSPtr const & entry,
                    EntityJobItemBaseSPtr const & jobItem,
                    ReconfigurationAgent & ra,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) :
                    Common::AsyncOperation(callback, parent),
                    entry_(std::static_pointer_cast<EntityEntry<T>>(entry)),
                    jobItem_(jobItem),
                    ra_(ra)
                {
                }

                void GetCompletedWork(std::vector<MultipleEntityWorkSPtr> & completedWork)
                {
                    completedWork = std::move(completedWork_);
                }

            protected:
                void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override
                {
                    /*
                        Higher layers require that the operation 
                        start on a separate thread. This makes handling of locks much simpler

                        Do not check the CompletedSynchronously in this code path (similar to the EntityScheduler)

                        CompletedSynchronously implies whether the operation is already complete or not
                        at the time the property is read. It does not indicate whether the operation
                        completed on the same thread or not

                        This code must always complete the operation on the callback thread so
                        that for the calling code the completion is always on a separate thead 
                        (which is guaranteed by the EntityScheduler)
                    */
                    auto op = entry_->Scheduler.BeginScheduleWorkItem(
                        std::move(jobItem_),
                        ra_.Threadpool,
                        ra_.Clock,
                        ra_.PerfCounters,
                        [this](Common::AsyncOperationSPtr const & scheduleOp)
                        {
                            FinishSchedule(scheduleOp);
                        },
                        thisSPtr);
                }

            private:
                void FinishSchedule(Common::AsyncOperationSPtr const & scheduleOp)
                {
                    EntityJobItemList result;
                    Diagnostics::ScheduleEntityPerformanceData perfData;
                    auto error = entry_->Scheduler.EndScheduleWorkItem(scheduleOp, result, perfData);
                    ASSERT_IF(!error.IsSuccess(), "Schedule must succeed");

                    if (result.empty())
                    {
                        TryComplete(scheduleOp->Parent);
                        return;
                    }

                    auto op = Common::AsyncOperation::CreateAndStart<EntityJobItemListExecutorAsyncOperationType>(
                        std::move(entry_),
                        std::move(result),
                        perfData,
                        ra_,
                        [this] (Common::AsyncOperationSPtr const & executionOp)
                        {
                            if (!executionOp->CompletedSynchronously)
                            {
                                FinishExecution(executionOp);
                            }
                        },
                        scheduleOp->Parent);

                    if (op->CompletedSynchronously)
                    {
                        FinishExecution(op);
                    }
                }

                void FinishExecution(Common::AsyncOperationSPtr const & executionOp)
                {
                    auto casted = Common::AsyncOperation::End<EntityJobItemListExecutorAsyncOperationType>(executionOp);
                    ASSERT_IF(!casted->Error.IsSuccess(), "execution op must succeed");

                    casted->GetCompletedWork(completedWork_);
                    TryComplete(executionOp->Parent);
                }

                EntityEntryTypeSPtr entry_;
                EntityJobItemBaseSPtr jobItem_;
                ReconfigurationAgent & ra_;
                std::vector<MultipleEntityWorkSPtr> completedWork_;
            };
        }
    }
}
