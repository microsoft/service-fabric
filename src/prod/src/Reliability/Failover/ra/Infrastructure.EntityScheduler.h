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
            /*
                The entity scheduler is used to queue work items while waiting for an entity lock

                Updates to entities require an exclusive lock and possibly a disk io

                While this is in progress new work items may arrive that require the entity lock
                These items are put in a queue and when the entity is unlocked they are executed

                Further, the RA needs to ensure that too many entities are not executed at once (i.e. 
                the RA cannot just gobble up things from the threadpool). Hence, once a write lock is available
                the entity will be 'scheduled' in a queue being serviced by a dedicated number of threads
                and when a thread is available the entity will be processed
            */
            template<typename T>
            class EntityScheduler
            {
            public:
                EntityScheduler() : 
                    isLocked_(false)
                {}

                /*
                    Tries to schedule a work item
                    
                    If the work item is already scheduled then a Result with an Invalid lock is returned. This 
                    implies that a previous work item has been scheduled and this work item will be returned in the 
                    completion of the previous operation

                    If the work item was the first one to be scheduled then when the lock is acquired the async 
                    op will complete returning this work item (and any others that may get scheduled in the interval)
                */
                Common::AsyncOperationSPtr BeginScheduleWorkItem(
                    EntityJobItemBaseSPtr && jobItem,
                    IThreadpool & threadpool,
                    IClock & clock,
                    Diagnostics::RAPerformanceCounters & perfCounters,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent)
                {
                    perfCounters.JobItemsScheduledPerSecond.Increment();

                    auto op = std::static_pointer_cast<ScheduleAsyncOperation>(Common::AsyncOperation::CreateAndStart<ScheduleAsyncOperation>(
                        threadpool, 
                        clock, 
                        perfCounters,
                        *this, 
                        callback, 
                        parent));

                    bool shouldSchedule = false;
                    bool shouldCompleteAsQueued = false;

                    {
                        Common::AcquireWriteLock grab(lock_);
                        if (pendingWork_.empty())
                        {
                            AssertDoesNotHavePendingOp();

                            pendingOp_ = op;
                            if (!isLocked_)
                            {
                                UpdatePerfCountersOnSchedule(clock, perfCounters);
                                shouldSchedule = true;
                            }
                            else
                            {
                                schedulePerfData_.OnQueueStart(clock);
                                perfCounters.NumberOfQueuedEntities.Increment();
                            }
                        }
                        else
                        {
                            AssertHasPendingOp();
                            shouldCompleteAsQueued = true;
                        }

                        pendingWork_.push_back(jobItem);
                    }

                    ASSERT_IF(shouldSchedule && shouldCompleteAsQueued, "Cannot schedule and complete at the same time");

                    if (shouldSchedule)
                    {
                        op->Schedule(op);
                    }

                    if (shouldCompleteAsQueued)
                    {
                        op->Complete(op);
                    }

                    return op;
                }

                Common::ErrorCode EndScheduleWorkItem(
                    Common::AsyncOperationSPtr const & op,
                    __out EntityJobItemList & result,
                    __out Diagnostics::ScheduleEntityPerformanceData & perfData)
                {
                    auto casted = Common::AsyncOperation::End<ScheduleAsyncOperation>(op);

                    casted->GetResult(result);
                    perfData = casted->SchedulePerfData;

                    return casted->Error;
                }

                void ReleaseLock(IClock & clock, Diagnostics::RAPerformanceCounters & perfCounters)
                {
                    ScheduleAsyncOperationSPtr op;
                    bool shouldSchedule = false;

                    perfCounters.NumberOfLockedEntities.Decrement();

                    {
                        Common::AcquireWriteLock grab(lock_);

                        isLocked_ = false;

                        if (!pendingWork_.empty())
                        {
                            shouldSchedule = true;
                            op = pendingOp_;
                            perfCounters.NumberOfQueuedEntities.Decrement();
                            schedulePerfData_.OnQueueEnd(clock);
                            UpdatePerfCountersOnSchedule(clock, perfCounters);
                        }
                    }

                    if (shouldSchedule)
                    {
                        op->Schedule(op);
                    }
                }

            private:
                class ScheduleAsyncOperation : public Common::AsyncOperation
                {
                    DENY_COPY(ScheduleAsyncOperation);

                public:
                    ScheduleAsyncOperation(
                        IThreadpool & threadpool,
                        IClock & clock,
                        Diagnostics::RAPerformanceCounters & perfCounters,
                        EntityScheduler & scheduler,
                        Common::AsyncCallback const & callback,
                        Common::AsyncOperationSPtr const & parent) :
                        Common::AsyncOperation(callback, parent),
                        scheduler_(scheduler),
                        threadpool_(threadpool),
                        clock_(clock),
                        perfCounters_(perfCounters)
                    {
                    }

                    __declspec(property(get = get_SchedulePerfData)) Diagnostics::ScheduleEntityPerformanceData SchedulePerfData;
                    Diagnostics::ScheduleEntityPerformanceData get_SchedulePerfData() const { return perfData_; }

                    void GetResult(__out EntityJobItemList & result)
                    {
                        result = std::move(result_);
                    }

                    void Complete(Common::AsyncOperationSPtr const & thisSPtr)
                    {
                        Complete(EntityJobItemList(), Diagnostics::ScheduleEntityPerformanceData(), thisSPtr);
                    }

                    void Complete(
                        EntityJobItemList && pendingWork, 
                        Diagnostics::ScheduleEntityPerformanceData perfData, 
                        Common::AsyncOperationSPtr const & thisSPtr)
                    {
                        result_ = std::move(pendingWork);
                        perfData_ = perfData;
                        bool completed = TryComplete(thisSPtr);
                        ASSERT_IF(!completed, "Complete must succeed");
                    }

                    void Schedule(Common::AsyncOperationSPtr const & thisSPtr)
                    {
                        /*
                            In the case of the EntityScheduler, higher layers require that the operation 
                            start on a separate thread. This makes handling of locks much simpler

                            Do not check the CompletedSynchronously in this code path

                            CompletedSynchronously implies whether the operation is already complete or not
                            at the time the property is read. It does not indicate whether the operation
                            completed on the same thread or not

                            This code must always complete the operation on the callback thread so
                            that for the calling code the completion is always on a separate thead 
                            (which is guaranteed by the threadpool)
                        */
                        threadpool_.BeginScheduleEntity(
                            [this](Common::AsyncOperationSPtr const & innerOp)
                            {
                                FinishSchedule(innerOp);
                            },
                            thisSPtr);
                    }

                protected:
                    void OnStart(Common::AsyncOperationSPtr const & ) override
                    {
                    }

                private:
                    void FinishSchedule(Common::AsyncOperationSPtr const & scheduleOp)
                    {
                        auto error = threadpool_.EndScheduleEntity(scheduleOp);
                        ASSERT_IF(!error.IsSuccess(), "Schedule cannot fail");

                        scheduler_.OnScheduled(clock_, perfCounters_);
                    }

                    EntityJobItemList result_;
                    EntityScheduler & scheduler_;
                    IThreadpool & threadpool_;
                    IClock & clock_;
                    Diagnostics::RAPerformanceCounters & perfCounters_;
                    Diagnostics::ScheduleEntityPerformanceData perfData_;
                };

                typedef std::shared_ptr<ScheduleAsyncOperation> ScheduleAsyncOperationSPtr;

                void UpdatePerfCountersOnSchedule(
                    IClock & clock,
                    Diagnostics::RAPerformanceCounters & perfCounters)
                {
                    schedulePerfData_.OnScheduleStart(clock);
                    perfCounters.NumberOfScheduledEntities.Increment();
                }

                void OnScheduled(
                    IClock & clock,
                    Diagnostics::RAPerformanceCounters & perfCounters)
                {
                    ScheduleAsyncOperationSPtr op;
                    EntityJobItemList pendingWork;
                    Diagnostics::ScheduleEntityPerformanceData schedulePerfData;

                    perfCounters.NumberOfScheduledEntities.Decrement();
                    perfCounters.NumberOfLockedEntities.Increment();

                    {
                        Common::AcquireWriteLock grab(lock_);
                        ASSERT_IF(pendingWork_.empty(), "Must have work otherwise could not have been scheduled");
                        AssertHasPendingOp();
                        std::swap(op, pendingOp_);
                        std::swap(pendingWork, pendingWork_);
                        std::swap(schedulePerfData, schedulePerfData_);
                        isLocked_ = true;
                    }

                    schedulePerfData.OnScheduleEnd(clock);

                    op->Complete(std::move(pendingWork), schedulePerfData, op);
                }

                void AssertDoesNotHavePendingOp()
                {
                    ASSERT_IF(pendingOp_ != nullptr, "Pending op must be null");
                }

                void AssertHasPendingOp()
                {
                    ASSERT_IF(pendingOp_ == nullptr, "Pending op must not be null");
                }

                Common::RwLock lock_;
                EntityJobItemList pendingWork_;
                ScheduleAsyncOperationSPtr pendingOp_;
                bool isLocked_;
                Diagnostics::ScheduleEntityPerformanceData schedulePerfData_;
                
            };
        }
    }
}
