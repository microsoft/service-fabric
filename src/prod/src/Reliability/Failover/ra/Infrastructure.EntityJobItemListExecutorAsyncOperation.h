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
            class EntityJobItemListExecutorAsyncOperation : public Common::AsyncOperation
            {
                DENY_COPY(EntityJobItemListExecutorAsyncOperation);

            public:
                typedef EntityScheduler<T> EntitySchedulerType;
                typedef EntityMap<T> EntityMapType;
                typedef EntityState<T> EntityStateType;
                typedef LockedEntityPtr<T> LockedEntityPtrType;
                typedef EntityEntry<T> EntityEntryType;
                typedef std::shared_ptr<EntityEntryType> EntityEntryTypeSPtr;
                typedef EntityJobItemListTracer<T> EntityJobItemListTracerType;
                typedef EntityJobItemBaseT<T> EntityJobItemBaseTType;

                EntityJobItemListExecutorAsyncOperation(
                    EntityEntryTypeSPtr && entry,
                    EntityJobItemList && jobItems,
                    Diagnostics::ScheduleEntityPerformanceData const & schedulePerfData,
                    ReconfigurationAgent & ra,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) :
                    Common::AsyncOperation(callback, parent),
                    entry_(std::move(entry)),
                    jobItems_(std::move(jobItems)),
                    schedulePerfData_(schedulePerfData),
                    ra_(ra)
                {
                }

                void GetCompletedWork(__out std::vector<MultipleEntityWorkSPtr> & completedWork)
                {
                    completedWork = std::move(completedWork_);
                }

            protected:
                void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override
                {
                    /*
                        It is expected that the entity is lockable (the scheduler has a lock) at this time
                    */
                    executionPerfData_.OnExecutionStart(jobItems_.size(), ra_.Clock);

                    AcquireLock();

                    for (auto & it : jobItems_)
                    {
                        CastToDerived(it).Process(lockedEntity_, ra_);
                    }

                    executionPerfData_.OnProcessed(ra_.Clock);

                    if (!lockedEntity_.IsUpdating)
                    {
                        FinishProcess(thisSPtr, Common::ErrorCode::Success());
                        return;
                    }

                    StartCommit(thisSPtr);
                }

            private:
                void StartCommit(Common::AsyncOperationSPtr const & thisSPtr)
                {
                    auto commitDescription = lockedEntity_.CreateCommitDescription();

                    ra_.PerfCounters.NumberOfCommittingEntities.Increment();

                    auto commitOp = entry_->GetEntityMap(ra_).BeginCommit(
                        std::static_pointer_cast<EntityEntryBase>(entry_),
                        commitDescription,
                        entry_->State,
                        entry_->Id,
                        ra_.Config.MaxEseCommitWaitDuration,
                        [this](Common::AsyncOperationSPtr const & inner)
                        {
                            if (!inner->CompletedSynchronously)
                            {
                                FinishCommit(inner);
                            }
                        },
                        thisSPtr);


                    if (commitOp->CompletedSynchronously)
                    {
                        FinishCommit(commitOp);
                    }
                }

                void FinishCommit(Common::AsyncOperationSPtr const & storeCommitOp)
                {
                    auto commitResult = entry_->GetEntityMap(ra_).EndCommit(storeCommitOp, commitPerfData_);

                    ra_.PerfCounters.NumberOfCommittingEntities.Decrement();

                    FinishProcess(storeCommitOp->Parent, commitResult);
                }

                void FinishProcess(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & commitResult)
                {
                    executionPerfData_.OnPostProcessStart(ra_.Clock);

                    for (auto & it : jobItems_)
                    {
                        auto & casted = CastToDerived(it);
                        casted.FinishProcess(lockedEntity_, entry_, commitResult, ra_);

                        /*
                            The entity may be part of a piece of work that required notification when all its associated
                            child job items were completed. Example, something may be checking if multiple replicas are closed
                            In this case both the work and the job item contain a shared ptr to each other

                            It is important to release the work from the job item here to break that cycle otherwise there is a leak

                            TODO: When the move to async is complete this will not be a problem
                        */
                        auto jobItemWork = casted.ReleaseWork();
                        if (jobItemWork != nullptr)
                        {
                            completedWork_.push_back(std::move(jobItemWork));
                        }
                    }

                    executionPerfData_.OnExecutionEnd(ra_.Clock);

                    EntityJobItemListTracerType tracer;
                    tracer.Trace(*entry_, entry_->Id, lockedEntity_, jobItems_, commitResult, schedulePerfData_, executionPerfData_, commitPerfData_, ra_);
                    
                    ReleaseLock();

                    executionPerfData_.ReportPerformanceData(ra_.PerfCounters);
                    schedulePerfData_.ReportPerformanceData(ra_.PerfCounters);
                    commitPerfData_.ReportPerformanceData(ra_.PerfCounters);

                    ra_.PerfCounters.JobItemsCompletedPerSecond.IncrementBy(static_cast<int>(jobItems_.size()));
					ra_.PerfCounters.NumberOfCompletedJobItems.IncrementBy(static_cast<int>(jobItems_.size()));

                    TryComplete(thisSPtr, Common::ErrorCode::Success());
                }

                void AcquireLock()
                {
                    lockedEntity_ = LockedEntityPtrType(entry_->State);
                }

                void ReleaseLock()
                {
                    {
                        LockedEntityPtrType releaser;
                        releaser = std::move(lockedEntity_);
                    }

                    entry_->Scheduler.ReleaseLock(ra_.Clock, ra_.PerfCounters);
                }

                static EntityJobItemBaseTType & CastToDerived(EntityJobItemBaseSPtr const & ji)
                {
                    /*
                        Always guaranteed to be correct as it is guaranteed by the scheduler and the jqm
                    */
                    return static_cast<EntityJobItemBaseTType&>(*ji);
                }

                EntityEntryTypeSPtr entry_;
                EntityJobItemList jobItems_;
                ReconfigurationAgent & ra_;
                LockedEntityPtrType lockedEntity_;
                std::vector<MultipleEntityWorkSPtr> completedWork_;
                Diagnostics::ScheduleEntityPerformanceData schedulePerfData_;
                Diagnostics::ExecuteEntityJobItemListPerformanceData executionPerfData_;
                Diagnostics::CommitEntityPerformanceData commitPerfData_;
            };
        }
    }
}
