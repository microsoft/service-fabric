// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    StringLiteral const TraceComponent("TransactionReplicator");

    // *******************************
    // ComReplicationAsyncOperation
    // *******************************

    class ReplicatedStore::TransactionReplicator::ComReplicationAsyncOperation : public ComProxyAsyncOperation
    {
        DENY_COPY(ComReplicationAsyncOperation);

    public:
        ComReplicationAsyncOperation(
            ComPointer<ComReplicatedOperationData> && replicationOperation,
            ComPointer<IFabricStateReplicator> const & innerReplicator,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : ComProxyAsyncOperation(callback, parent)
            , replicationOperation_(std::move(replicationOperation))
            , innerReplicator_(innerReplicator)
            , operationLSN_(-1)
        {
            ReplicatedStoreEventSource::Trace->PrimaryComReplAsyncCtor(TraceThis);
        }

        ~ComReplicationAsyncOperation()
        {
            ReplicatedStoreEventSource::Trace->PrimaryComReplAsyncDtor(TraceThis);
        }

        __declspec(property(get=get_OperationLSN)) ::FABRIC_SEQUENCE_NUMBER InnerOperationLSN;
        ::FABRIC_SEQUENCE_NUMBER get_OperationLSN() const { return operationLSN_; }

        __declspec(property(get=get_OperationDataSize)) ULONG OperationDataSize;
        ULONG get_OperationDataSize() const { return replicationOperation_->OperationDataSize; }

        static ErrorCode End(AsyncOperationSPtr const & operation);

    protected:
        HRESULT BeginComAsyncOperation(__in IFabricAsyncOperationCallback *, __out IFabricAsyncOperationContext **);
        HRESULT EndComAsyncOperation(__in IFabricAsyncOperationContext *);

    private:
        HRESULT ConvertReplicatorErrorCode(HRESULT replicatorResult);

        ComPointer<ComReplicatedOperationData> replicationOperation_;
        ComPointer<IFabricStateReplicator> innerReplicator_;
        ::FABRIC_SEQUENCE_NUMBER operationLSN_;
    };

    // *******************************
    // ReplicationAsyncOperation
    // *******************************

    class ReplicatedStore::TransactionReplicator::ReplicationAsyncOperation
        : public TimedAsyncOperation
        , public ReplicaActivityTraceComponent<TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY(ReplicationAsyncOperation);

    public:
        ReplicationAsyncOperation(
            __in TransactionReplicator & owner,
            __in TimeSpan const timeout,
            __in Store::ReplicatedStore::Transaction & tx,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : TimedAsyncOperation(timeout, callback, parent)
            , ReplicaActivityTraceComponent(tx.ReplicaActivityId)
            , owner_(owner)
            , transactionSPtr_()
            , replicationOperations_(tx.MoveReplicationOperations())
            , txSettings_(tx.TransactionSettings)
            , pendingCommitWaiterCount_(0)
            , updatedTombstonesCount_(0)
            , overallStopwatch_()
            , innerStopwatch_()
            , replicationCompleted_(false)
            , pendingLocalAsyncOperations_(0)
            , createError_()
        {
            ReplicatedStoreEventSource::Trace->PrimaryReplAsyncCtor(this->ReplicaActivityId, TraceThis);

            createError_ = tx.TryGetInnerTransaction(transactionSPtr_);
        }

        ReplicationAsyncOperation(
            __in TransactionReplicator & owner,
            __in Store::ReplicatedStore::SimpleTransactionGroup & tx,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : TimedAsyncOperation(TimeSpan::MaxValue, callback, parent)
            , ReplicaActivityTraceComponent(tx.ReplicaActivityId)
            , owner_(owner)
            , transactionSPtr_()
            , operationLSN_(0)
            , replicationOperations_(tx.MoveReplicationOperations())
            , txSettings_()
            , pendingCommitWaiterCount_(0)
            , replicatorError_()
            , completionError_()
            , updatedTombstonesCount_(0)
            , overallStopwatch_()
            , innerStopwatch_()
            , replicationCompleted_(false)
            , pendingLocalAsyncOperations_(0)
            , createError_()
        {
            ReplicatedStoreEventSource::Trace->PrimaryReplAsyncCtor(this->ReplicaActivityId, TraceThis);

            createError_ = tx.TryGetInnerTransaction(transactionSPtr_);
        }

        ReplicationAsyncOperation(
            __in TransactionReplicator & owner,
            __in TimeSpan const timeout,
            __in Store::ReplicatedStore::SimpleTransaction & simpleTx,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : TimedAsyncOperation(timeout, callback, parent)
            , ReplicaActivityTraceComponent(simpleTx.ReplicaActivityId)
            , owner_(owner)
            , transactionSPtr_()
            , operationLSN_(0)
            , replicationOperations_()
            , txSettings_()
            , pendingCommitWaiterCount_(0)
            , replicatorError_()
            , completionError_()
            , updatedTombstonesCount_(0)
            , overallStopwatch_()
            , innerStopwatch_()
            , replicationCompleted_(false)
            , pendingLocalAsyncOperations_(0)
            , createError_()
        {
            ReplicatedStoreEventSource::Trace->PrimaryReplAsyncCtor(this->ReplicaActivityId, TraceThis);

            // transactionSPtr_ intentionally remains null
        }

        ReplicationAsyncOperation(
            __in TransactionReplicator & owner,
            __in ErrorCode const & createError,
            __in Store::ReplicaActivityId const & activityId,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : TimedAsyncOperation(TimeSpan::MaxValue, callback, parent)
            , ReplicaActivityTraceComponent(activityId)
            , owner_(owner)
            , transactionSPtr_()
            , operationLSN_(0)
            , replicationOperations_()
            , txSettings_()
            , pendingCommitWaiterCount_(0)
            , replicatorError_()
            , completionError_()
            , updatedTombstonesCount_(0)
            , overallStopwatch_()
            , innerStopwatch_()
            , replicationCompleted_(false)
            , pendingLocalAsyncOperations_(0)
            , createError_(createError)
        {
            ReplicatedStoreEventSource::Trace->PrimaryReplAsyncCtor(this->ReplicaActivityId, TraceThis);
        }

        ~ReplicationAsyncOperation()
        {
            ReplicatedStoreEventSource::Trace->PrimaryReplAsyncDtor(this->ReplicaActivityId, TraceThis);
        }

        __declspec(property(get=get_Transaction)) TransactionSPtr const & Transaction;
        __declspec(property(get=get_OperationLSN)) ::FABRIC_SEQUENCE_NUMBER OperationLSN;
        __declspec(property(get=get_ReplicationOperations)) std::vector<ReplicationOperation> & ReplicationOperations;
        __declspec(property(get=get_InnerCompletedSynchronously)) bool InnerCompletedSynchronously;
        __declspec(property(get=get_ReplicatorError)) ErrorCode const & ReplicatorError;
        __declspec(property(put=put_CompletionError)) ErrorCode CompletionError;
        __declspec(property(get=get_UpdatedTombstonesCount)) size_t UpdatedTombstonesCount;
        __declspec(property(get=get_TimeBeforeCommit, put=put_TimeBeforeCommit)) StopwatchTime TimeBeforeCommit;
        __declspec(property(get=get_ReplicationdCompleted, put=put_ReplicationCompleted)) bool const ReplicationCompleted;

        TransactionSPtr const & get_Transaction() const { return transactionSPtr_; }
        ::FABRIC_SEQUENCE_NUMBER get_OperationLSN() const { return operationLSN_; }
        std::vector<ReplicationOperation> & get_ReplicationOperations() { return replicationOperations_; }
        ErrorCode const & get_ReplicatorError() const { return replicatorError_; }
        void put_CompletionError(ErrorCode value) { completionError_ = value; }
        size_t get_UpdatedTombstonesCount() const { return updatedTombstonesCount_; }
        bool const get_ReplicationdCompleted() { return replicationCompleted_; }
        void put_ReplicationCompleted(bool value) { replicationCompleted_ = value; }

        void IncrementUpdatedTombstonesCount() { ++updatedTombstonesCount_; }

        void OnStart(AsyncOperationSPtr const &);
        void OnTimeout(AsyncOperationSPtr const &);
        void OnCompleted();
        static ErrorCode End(AsyncOperationSPtr const &, __out ::FABRIC_SEQUENCE_NUMBER &);
        void ResetInnerTransaction() { transactionSPtr_.reset(); }

        void InitializePendingCommitWaiterCount()
        {
            pendingCommitWaiterCount_.store(2);
        }

        LONG DecrementPendingCommitWaiterCount()
        {
            return --pendingCommitWaiterCount_;
        }

        void IncrementPendingLocalAsyncOperations()
        {
            ++pendingLocalAsyncOperations_;
        }

        void DecrementPendingLocalAsyncOperations(AsyncOperationSPtr const & thisSPtr)
        {
            if (0 == --pendingLocalAsyncOperations_)
            {
                overallStopwatch_.Stop();

                owner_.replicatedStore_.PerfCounters.AvgLatencyOfCommitBase.Increment();
                owner_.replicatedStore_.PerfCounters.AvgLatencyOfCommit.IncrementBy(overallStopwatch_.ElapsedMicroseconds);

                thisSPtr->TryComplete(thisSPtr, completionError_);
            }
        }

        void StartCommitStopwatch()
        {
            innerStopwatch_.Restart();
        }

        TimeSpan GetElapsedCommitTime()
        {
            return innerStopwatch_.Elapsed;
        }

    private:
        void InnerReplicationCallback(AsyncOperationSPtr const &);
        void OnInnerReplicationComplete(AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
        void FinishInnerReplication(AsyncOperationSPtr const &);

        ErrorCode TryAddPendingCommit_CallerHoldsLock(AsyncOperationSPtr const &);

        TransactionReplicator & owner_;
        TransactionSPtr transactionSPtr_;
        ::FABRIC_SEQUENCE_NUMBER operationLSN_;
        std::vector<ReplicationOperation> replicationOperations_;
        TransactionSettings txSettings_;

        Common::atomic_long pendingCommitWaiterCount_;
        ErrorCode replicatorError_;
        ErrorCode completionError_;

        size_t updatedTombstonesCount_;
        Stopwatch overallStopwatch_;
        Stopwatch innerStopwatch_;
        bool replicationCompleted_;
        Common::atomic_long pendingLocalAsyncOperations_;
        ErrorCode createError_;
    };

    // *******************************
    // PendingCommitsQueue
    // *******************************

    class ReplicatedStore::TransactionReplicator::PendingCommitsQueue
    {
    public:
        PendingCommitsQueue(
            __in ReplicatedStore & owner,
            FABRIC_SEQUENCE_NUMBER lastCommittedLSN)
            : owner_(owner)
            , isFlushing_(false)
            , quorumAckedLsn_(static_cast<uint64>(lastCommittedLSN))
            , pendingCommits_()
            , pendingCommitsLock_()
        {
        }

        __declspec(property(get=get_Lock)) ExclusiveLock & Lock;
        ExclusiveLock & get_Lock() { return pendingCommitsLock_; }

        __declspec(property(get=get_QuorumAckedLsn)) uint64 QuorumAckedLsn;
        uint64 get_QuorumAckedLsn() { return quorumAckedLsn_.load(); }

        void Enqueue_CallerHoldsLock(AsyncOperationSPtr const & operation)
        {
            auto casted = this->GetCasted(operation);

            pendingCommits_.push(move(casted));

            owner_.UpdatePendingTransactions(static_cast<int>(pendingCommits_.size()));
        }

        queue<ReplicationAsyncOperationSPtr> GetCommitsToComplete(
            AsyncOperationSPtr const & operation,
            __out ReplicationAsyncOperationSPtr & casted)
        {
            casted = this->GetCasted(operation);

            AcquireExclusiveLock lock(pendingCommitsLock_);

            casted->ReplicationCompleted = true;

            auto currentLsn = casted->OperationLSN;

            if (quorumAckedLsn_.load() < static_cast<uint64>(currentLsn))
            {
                quorumAckedLsn_.store(currentLsn);
            }

            // LSNs must be persisted (begin commit called) in LSN order
            // to report correct persisted progress. This means we cannot have
            // multiple flush threads.
            //
            if (isFlushing_)
            {
                return queue<ReplicationAsyncOperationSPtr>();
            }

            auto readyCommits = this->DequeueReadyCommits_CallerHoldsLock(currentLsn, casted->ReplicatorError);

            if (!readyCommits.empty())
            {
                isFlushing_ = true;
            }

            return move(readyCommits);
        }

        queue<ReplicationAsyncOperationSPtr> DrainReadyCommits()
        {
            AcquireExclusiveLock lock(pendingCommitsLock_);

            auto readyCommits = this->DequeueReadyCommits_CallerHoldsLock(0, ErrorCodeValue::Success);

            isFlushing_ = !readyCommits.empty();

            owner_.UpdatePendingTransactions(static_cast<int>(pendingCommits_.size()));

            return move(readyCommits);
        }

    private:

        ReplicationAsyncOperationSPtr GetCasted(AsyncOperationSPtr const & operation)
        {
            auto casted = dynamic_pointer_cast<ReplicationAsyncOperation>(operation);

            if (casted == nullptr)
            {
                auto msg = "Failed to cast ReplicationAsyncOperation";

                WriteError(TraceComponent, "{0}", msg);

                Assert::CodingError("{0}", msg);
            }

            return casted;
        }

        queue<ReplicationAsyncOperationSPtr> DequeueReadyCommits_CallerHoldsLock(
            ::FABRIC_SEQUENCE_NUMBER targetLsn,
            ErrorCode const & replicationError)
        {
            queue<ReplicationAsyncOperationSPtr> readyCommits;

            // Underlying replicator guarantees that when replication operation N completes successfully, 
            // all replication operations < N must have already completed as well even if we haven't
            // observed the async operation completion yet due to threadpool scheduling delays.
            //
            // Therefore, we can commit and complete all operations up to N and any contiguous operations
            // after N for which we've observed an explicit replication completion.
            //
            while (!pendingCommits_.empty())
            {
                auto & current = pendingCommits_.front();
                auto isTargetLsn = (current->OperationLSN == targetLsn);
                auto isImplicitCommit = (replicationError.IsSuccess() && current->OperationLSN < targetLsn);
                auto isExplicitCommit = current->ReplicationCompleted;

                if (isTargetLsn || isImplicitCommit || isExplicitCommit)
                {
                    readyCommits.push(move(current));

                    pendingCommits_.pop();
                }
                else
                {
                    break;
                }
            }

            return move(readyCommits);
        }

        ReplicatedStore & owner_;
        bool isFlushing_;
        atomic_uint64 quorumAckedLsn_;
        queue<ReplicationAsyncOperationSPtr> pendingCommits_;
        ExclusiveLock pendingCommitsLock_;
    };

    // *******************************
    // Throttle support
    // *******************************

    struct ThrottleCallbackJobEntry
    {
    public:
        ThrottleCallbackJobEntry()
        {
        }

       ThrottleCallbackJobEntry(
            bool shouldThrottle,
            ThrottleCallback const & callbackToInvoke)
            : shouldThrottle_(shouldThrottle)
            , callBack_(callbackToInvoke)
        {
        }

        bool ProcessJob(ComponentRoot const &)
        {
            if (!callBack_) return true;
            callBack_(shouldThrottle_);
            return true;
        }

    private:
        bool shouldThrottle_;
        ThrottleCallback callBack_;
    };

    class ReplicatedStore::TransactionReplicator::InvokeThrottleCallbackJobQueue
        : public JobQueue<ThrottleCallbackJobEntry, ComponentRoot>
        , public PartitionedReplicaTraceComponent<TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY(InvokeThrottleCallbackJobQueue);

    public:
        static InvokeThrottleCallbackJobQueueSPtr Create(std::wstring const & name, ReplicatedStore & replicatedStore)
        {
            return std::shared_ptr<InvokeThrottleCallbackJobQueue>(new InvokeThrottleCallbackJobQueue(name, const_cast<ComponentRoot &>(replicatedStore.Root), replicatedStore));
        }

    protected:
        virtual void OnFinishItems()
        {
            WriteInfo("TransactionReplicator", "{0}: Throttle Callback Job Queue OnFinishedItems", this->TraceId);

            // incremented in InitializeThrottleCallerHoldsLock()
            //
            replicatedStore_.FinishTransaction();
        }

    private:
        InvokeThrottleCallbackJobQueue(std::wstring const & name, ComponentRoot & root, ReplicatedStore & replicatedStore)
            : PartitionedReplicaTraceComponent(replicatedStore.PartitionedReplicaId)
            , JobQueue(name, root, false, 1/*This MUST be 1 to ensure only 1 item is processed at a time*/)
            , replicatedStore_(replicatedStore)
        {
        }

        ReplicatedStore & replicatedStore_;
    };

    // *******************************
    // TransactionReplicator
    // *******************************

    ReplicatedStore::TransactionReplicator::TransactionReplicator(
        __in ReplicatedStore & replicatedStore,
        __in ComPointer<IFabricStateReplicator> const& innerReplicator,
        __in ComPointer<IFabricInternalStateReplicator> const& innerInternalReplicator)
        : PartitionedReplicaTraceComponent(replicatedStore.PartitionedReplicaId)
        , replicatedStore_(replicatedStore)
        , innerReplicator_(innerReplicator)
        , innerInternalReplicator_(innerInternalReplicator)
        , pendingCommitsQueueUPtr_()
        , localFlushLock_()
        , shouldLocalFlush_(true)
        , thLastUpdatedReplicationQueueOperationCount_(0)
        , thLastUpdatedReplicationQueueSizeBytes_(0)
        , thPendingReplicationQueueOperationCount_(0)
        , thPendingReplicationQueueSizeBytes_(0)
        , currentReplicationCompletedOperationCount_(0)
        , thLock_()
        , thStateMachineUPtr_(nullptr)
        , thShouldThrottle_(false)
        , thCallbackJobQueueSPtr_(nullptr)
        , thCallBack_()
        , thCountersUpdateTimerSPtr_(nullptr)
        , thActivelyRunning_(false)
    {
        thStateMachineUPtr_ = ThrottleStateMachine::Create(replicatedStore);
    }

    ReplicatedStore::TransactionReplicator::~TransactionReplicator()
    {
        ASSERT_IF(thCountersUpdateTimerSPtr_ || thCallbackJobQueueSPtr_, "{0}: Throttle timer or callback jobQueue is still active", this->TraceId);
    }

    void ReplicatedStore::TransactionReplicator::Initialize(::FABRIC_SEQUENCE_NUMBER lastCommittedLSN)
    {
        if (pendingCommitsQueueUPtr_.get() != nullptr) { return; }

        pendingCommitsQueueUPtr_ = make_unique<PendingCommitsQueue>(replicatedStore_, lastCommittedLSN);

        AcquireWriteLock lock(thLock_);
        ASSERT_IF(thCallbackJobQueueSPtr_ || thCountersUpdateTimerSPtr_,"{0}: Throttle callback job queue or counter update timer must be empty on Initialize", this->TraceId);
        thStateMachineUPtr_->TransitionToInitialized();
    }

    void ReplicatedStore::TransactionReplicator::TryUnInitialize()
    {
        InvokeThrottleCallbackJobQueueSPtr jobQueueCopy;
        TimerSPtr timerCopy;

        {
            AcquireWriteLock lock(thLock_);

            if (!thStateMachineUPtr_->IsStarted)
            {
                // Nothing to cleanup as throttle is not started
                thStateMachineUPtr_->TransitionToStopped();
                return;
            }

            thStateMachineUPtr_->TransitionToStopped();
            ASSERT_IFNOT(thCallbackJobQueueSPtr_ || thCountersUpdateTimerSPtr_,"{0}: Throttle callback job queue or counter update timer must not be empty on TryUnInitialize", this->TraceId);

            WriteInfo(TraceComponent, "{0}: Posting Throttle Callback Job Queue Close", this->TraceId);

            swap(thCallbackJobQueueSPtr_, jobQueueCopy);
            swap(thCountersUpdateTimerSPtr_, timerCopy);
        } // Release Lock

        // ***********************************************************************
        // NOTE - Do not access any member variables after the thread post below
        // as it could result in the destruction of the object when calling close on the job queue
        // ***********************************************************************
        Threadpool::Post([jobQueueCopy, timerCopy]()
        {
            timerCopy->Cancel();
            jobQueueCopy->Close();
        });
    }

    AsyncOperationSPtr ReplicatedStore::TransactionReplicator::BeginReplicate(
        __in ReplicatedStore::Transaction & transaction,
        __in TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        replicatedStore_.PerfCounters.RateOfReplication.Increment();

        return AsyncOperation::CreateAndStart<ReplicationAsyncOperation>(*this, timeout, transaction, callback, parent);
    }

    AsyncOperationSPtr ReplicatedStore::TransactionReplicator::BeginReplicate(
        __in ReplicatedStore::SimpleTransactionGroup & transaction,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<ReplicationAsyncOperation>(*this, transaction, callback, parent);
    }

    AsyncOperationSPtr ReplicatedStore::TransactionReplicator::BeginReplicate(
        __in ReplicatedStore::SimpleTransaction & transaction,
        __in TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<ReplicationAsyncOperation>(*this, timeout, transaction, callback, parent);
    }

    AsyncOperationSPtr ReplicatedStore::TransactionReplicator::BeginReplicate(
        __in ErrorCode const & createError,
        __in Store::ReplicaActivityId const & activityId,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<ReplicationAsyncOperation>(*this, createError, activityId, callback, parent);
}

    ErrorCode ReplicatedStore::TransactionReplicator::EndReplicate(
        AsyncOperationSPtr const & asyncOperation,
        __out ::FABRIC_SEQUENCE_NUMBER & operationLSN)
    {
        return ReplicationAsyncOperation::End(asyncOperation, operationLSN);
    }

    void ReplicatedStore::TransactionReplicator::OrderedCommitAndComplete(AsyncOperationSPtr const & asyncOperation)
    {
        ReplicationAsyncOperationSPtr casted;
        auto readyCommits = pendingCommitsQueueUPtr_->GetCommitsToComplete(asyncOperation, casted);
        auto currentLsn = casted->OperationLSN;

        if (readyCommits.empty())
        {
            ReplicatedStoreEventSource::Trace->PrimarySkipFlush(this->PartitionedReplicaId, currentLsn);

            return;
        }


        casted->IncrementPendingLocalAsyncOperations();

        auto error = replicatedStore_.FatalError;

        this->ApplyReadyCommits(readyCommits, currentLsn, error);

        // Must drain completed operations that may have skipped flush.
        //
        auto selfRoot = this->CreateComponentRoot();
        Threadpool::Post([this, selfRoot, currentLsn, error] { this->DrainReadyCommits(currentLsn, error); });

        // Do this as the last step to keep the ReplicatedStore and TransactionReplicator alive
        // as long as there's on outstanding commit.
        //
        casted->DecrementPendingLocalAsyncOperations(asyncOperation);
    }

    void ReplicatedStore::TransactionReplicator::DrainReadyCommits(FABRIC_SEQUENCE_NUMBER currentLsn, ErrorCode const & inputError)
    {
        ErrorCode error = inputError;

        auto readyCommits = pendingCommitsQueueUPtr_->DrainReadyCommits();

        while (!readyCommits.empty())
        {
            this->ApplyReadyCommits(readyCommits, currentLsn, error);

            readyCommits = pendingCommitsQueueUPtr_->DrainReadyCommits();
        }

        AcquireReadLock local(localFlushLock_);

        // ReplicatedStore may be destructed once Cleanup() is called
        // (triggered by completion of last drained ready commit).
        // Only access TransactionReplicator member variables.
        //
        if (shouldLocalFlush_ && replicatedStore_.Settings.EnableFlushOnDrain)
        {
            this->FlushCommits();
        }
    }

    void ReplicatedStore::TransactionReplicator::FlushCommits()
    {
        TransactionSPtr txSPtr;
        if (replicatedStore_.LocalStore->CreateTransaction(txSPtr).IsSuccess())
        {
            auto error = txSPtr->Commit();

            if (error.IsError(ErrorCodeValue::StoreFatalError))
            {
                WriteError(
                    TraceComponent,
                    "{0} failed to flush commits: error={1}",
                    this->TraceId,
                    error);

                this->replicatedStore_.ReportFault(error, FABRIC_FAULT_TYPE_TRANSIENT);
            }
        }
    }

    void ReplicatedStore::TransactionReplicator::Cleanup()
    {
        AcquireWriteLock lock(localFlushLock_);

        shouldLocalFlush_ = false;
    }

    void ReplicatedStore::TransactionReplicator::ApplyReadyCommits(
        __in queue<ReplicationAsyncOperationSPtr> & readyCommits,
        FABRIC_SEQUENCE_NUMBER currentLsn,
        __in ErrorCode & error)
    {
        if (replicatedStore_.Test_GetLocalApplyDelay() > TimeSpan::Zero)
        {
            auto delay = replicatedStore_.Test_GetLocalApplyDelay();

            WriteWarning(
                TraceComponent,
                "{0} delaying local apply for test: delay={1}",
                this->TraceId,
                delay);

            Sleep(static_cast<DWORD>(delay.TotalMilliseconds()));
        }

        ReplicatedStoreEventSource::Trace->PrimaryFlushCommits(
            this->PartitionedReplicaId, 
            readyCommits.front()->OperationLSN,
            readyCommits.back()->OperationLSN,
            currentLsn);

        while (!readyCommits.empty())
        {
            auto readyCommit = readyCommits.front();

            readyCommits.pop();

            if (error.IsSuccess())
            {
                error = readyCommit->ReplicatorError;
            }

            if (error.IsSuccess())
            {
                ReplicatedStoreEventSource::Trace->OperationComplete(
                    readyCommit->ReplicaActivityId,
                    readyCommit->OperationLSN,
                    currentLsn);

                error = this->StartLocalCommit(readyCommit);

                // Fail pending transactions gracefully on ObjectClosed
                //
                if (error.IsError(ErrorCodeValue::ObjectClosed))
                {
                    error = ErrorCodeValue::NotPrimary;
                }
                else if (!error.IsSuccess())
                {
                    WriteError(
                        TraceComponent,
                        "{0} primary failed to update LSN {1}: error={2}",
                        readyCommit->TraceId,
                        readyCommit->OperationLSN,
                        error);

                    // Operations must be persisted in LSN order so we cannot
                    // continue to persist operations at this point (even if
                    // they had replicated succesfully).
                    //
                    replicatedStore_.ReportFault(error, FABRIC_FAULT_TYPE_TRANSIENT);
                }
            }

            if (!error.IsSuccess())
            {
                if (readyCommit->OperationLSN == currentLsn)
                {
                    // Do not complete here to avoid destruction of ReplicatedStore
                    // while processing remaining operations.
                    //
                    readyCommit->CompletionError = error;
                }
                else
                {
                    readyCommit->TryComplete(readyCommit, error);
                }
            }
        } // for readyCommits
    }

    ErrorCode ReplicatedStore::TransactionReplicator::StartLocalCommit(ReplicationAsyncOperationSPtr const & casted)
    {
        ErrorCode error(ErrorCodeValue::Success);

        auto operationLsn = casted->OperationLSN;

        ReplicatedStoreEventSource::Trace->PrimaryUpdateLsn(
            this->PartitionedReplicaId,
            operationLsn);

        size_t tombstoneIndex = 0;

        for (auto const & repl : casted->ReplicationOperations)
        {
            switch (repl.Operation)
            {
            case ReplicationOperationType::Insert:
            case ReplicationOperationType::Update:
                error = replicatedStore_.LocalStore->UpdateOperationLSN(
                    casted->Transaction,
                    repl.Type,
                    repl.Key,
                    operationLsn);
                break;

            case ReplicationOperationType::Delete:
            {
                bool shouldCountTombstone = false;
                error = replicatedStore_.FinalizeTombstone(
                    casted->Transaction,
                    repl.Type,
                    repl.Key,
                    operationLsn,
                    tombstoneIndex++,
                    shouldCountTombstone);

                if (error.IsSuccess() && shouldCountTombstone)
                {
                    casted->IncrementUpdatedTombstonesCount();
                }

                break;
             }

            default:
                TRACE_ERROR_AND_TESTASSERT(
                    TraceComponent,
                    "{0} Unrecognized operation on primary: [{1}] type = '{2}' key = '{3}'",
                    casted->TraceId,
                    repl.Operation,
                    repl.Type,
                    repl.Key);

                error = ErrorCodeValue::InvalidOperation;

                break;

            } // switch operation type

            if (!error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent,
                    "{0} failed to update sequence number for data item: [{1}] type = '{2}' key = '{3}' operationLSN = {4} error = {5}",
                    casted->TraceId,
                    repl.Operation,
                    repl.Type,
                    repl.Key,
                    operationLsn,
                    error);
                return error;
            }

        } // for operations

        if (error.IsSuccess())
        {
            casted->IncrementPendingLocalAsyncOperations();

            casted->StartCommitStopwatch();

            casted->Transaction->BeginCommit(
                casted->RemainingTime,
                [this](AsyncOperationSPtr const & operation){ this->OnCommitComplete(operation); },
                casted);
        }

        return error;
    }

    void ReplicatedStore::TransactionReplicator::OnCommitComplete(AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::Get<ReplicationAsyncOperation>(asyncOperation->Parent);

        ::FABRIC_SEQUENCE_NUMBER operationLSN = casted->OperationLSN;

        auto commitTimeMillis = casted->GetElapsedCommitTime().TotalMilliseconds();
        auto slowCommitTraceThreshold = StoreConfig::GetConfig().SlowCommitTraceThreshold;

        if (commitTimeMillis < static_cast<int64>(slowCommitTraceThreshold.TotalPositiveMilliseconds())
            && !replicatedStore_.Test_GetSlowCommitTestEnabled())
        {
            ReplicatedStoreEventSource::Trace->PrimaryCommitFast(
                this->PartitionedReplicaId,
                operationLSN,
                commitTimeMillis);
        }
        else
        {
            ReplicatedStoreEventSource::Trace->PrimaryCommitSlow(
                this->PartitionedReplicaId,
                operationLSN,
                commitTimeMillis);

            replicatedStore_.OnSlowCommit();
        }

        ::FABRIC_SEQUENCE_NUMBER unused;
        auto error =  casted->Transaction->EndCommit(asyncOperation, unused);

        // Do this after commit so that any new tombstones show up to the prune process
        //
        if (error.IsSuccess() && casted->UpdatedTombstonesCount > 0)
        {
            replicatedStore_.ScheduleTombstoneCleanupIfNeeded(casted->UpdatedTombstonesCount);
        }

        casted->ResetInnerTransaction();
        casted->CompletionError = error;
        casted->DecrementPendingLocalAsyncOperations(asyncOperation->Parent);
    }

    ErrorCode ReplicatedStore::TransactionReplicator::InitializeThrottleCallerHoldsLock()
    {
        ASSERT_IF(thCallbackJobQueueSPtr_ || thCountersUpdateTimerSPtr_, "{0}: ThrottleCallback JobQueue or counters update timer already exists", this->TraceId);

        ReplicatedStoreState::Enum state;
        ErrorCode error = replicatedStore_.stateMachineUPtr_->StartTransactionEvent(state);

        if (!error.IsSuccess())
        {
            WriteInfo(TraceComponent, "{0}: Failed to initialize throttle. Error = {1}", this->TraceId, error);
            return error;
        }

        thCallbackJobQueueSPtr_ = InvokeThrottleCallbackJobQueue::Create(this->TraceId, replicatedStore_);

        ComponentRoot root = this->replicatedStore_.Root;
        thCountersUpdateTimerSPtr_ = Timer::Create(
            TimerTagDefault,
            [this, root](TimerSPtr const &)
            {
                UpdateThrottleCounters();
            },
            false);

        // Set the flag to wait for all callbacks to complete on cancel.
        thCountersUpdateTimerSPtr_->SetCancelWait();

        return ErrorCodeValue::Success;
    }

    void ReplicatedStore::TransactionReplicator::UpdateThrottleCallback(ThrottleCallback const & callBack)
    {
        AcquireWriteLock grab(thLock_);
        WriteInfo(TraceComponent, "{0}: Throttle Callback changed", this->TraceId);
        thCallBack_ = callBack;
    }

    bool ReplicatedStore::TransactionReplicator::IsThrottleNeeded()
    {
        if (replicatedStore_.IsThrottleEnabled)
        {
            AcquireReadLock grab(thLock_);
            return IsThrottleNeededInternalCallerHoldsLock();
        }
        else
        {
            return false;
        }
    }

    bool ReplicatedStore::TransactionReplicator::IsThrottleNeededInternalCallerHoldsLock()
    {
        ULONGLONG operationCount = thLastUpdatedReplicationQueueOperationCount_ + thPendingReplicationQueueOperationCount_;
        ULONGLONG queueSizeBytes = thLastUpdatedReplicationQueueSizeBytes_ + thPendingReplicationQueueSizeBytes_;

        auto const & settings = replicatedStore_.Settings;

        bool isThrottleNeeded =
            // if a counter is non-0, it means we must throttle by this metric
            // Return TRUE if either of the counters have exceeded the limit that is set
            (
                (settings.ThrottleReplicationQueueOperationCount != 0
                &&
                operationCount >= settings.ThrottleReplicationQueueOperationCount)

                ||

                (settings.ThrottleReplicationQueueSizeBytes != 0
                &&
                queueSizeBytes >= settings.ThrottleReplicationQueueSizeBytes)
            );

        return isThrottleNeeded;
    }

    bool ReplicatedStore::TransactionReplicator::ShouldScheduleThrottleNotification(__out bool & enableThrottle)
    {
        // Schedule a notification only if there is a change in the state of what the user already knows and throttle was not stopped due to an uninitialize call
        AcquireReadLock grab(thLock_);
        return ShouldScheduleThrottleNotificationCallerHoldsLock(enableThrottle);
    }

    bool ReplicatedStore::TransactionReplicator::ShouldScheduleThrottleNotificationCallerHoldsLock(__out bool & enableThrottle)
    {
        // Schedule a notification only if there is a change in the state of what the user already knows and throttle was not stopped due to an uninitialize call
        enableThrottle = IsThrottleNeededInternalCallerHoldsLock();
        return enableThrottle != thShouldThrottle_ && !thStateMachineUPtr_->IsStopped;
    }

    void ReplicatedStore::TransactionReplicator::UpdateThrottleCounters()
    {
        // Optimization: If there are a burst of EndReplicate calls, we will quickly return if one of the threads
        // is already updating the throttle counters, rather than waiting on the write lock below
        shared_ptr<ScopedActiveFlag> scopedActiveFlag;
        if (!ScopedActiveFlag::TryAcquire(thActivelyRunning_, scopedActiveFlag))
        {
            return;
        }

        {
            AcquireWriteLock lock(thLock_);
            FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS counters;

            HRESULT hr = replicatedStore_.InnerInternalReplicator->GetReplicationQueueCounters(&counters);

            if (SUCCEEDED(hr))
            {
                // If the above call succeeds, reset our in memory book keeping variables
                thPendingReplicationQueueOperationCount_ = 0;
                thPendingReplicationQueueSizeBytes_ = 0;
                currentReplicationCompletedOperationCount_.store(0);

                thLastUpdatedReplicationQueueOperationCount_ = counters.operationCount;
                thLastUpdatedReplicationQueueSizeBytes_ = counters.queueSizeBytes;
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    "{0} Get Replication Queue counters for Throttle Failed with EC = {1}: \r\n"
                    "OperationCount = {2} \r\n"
                    "QueueSizeBytes = {3}",
                    TraceId,
                    ErrorCode::FromHResult(hr),
                    thLastUpdatedReplicationQueueOperationCount_,
                    thLastUpdatedReplicationQueueSizeBytes_);
            }

            bool enableThrottle;
            if (ShouldScheduleThrottleNotificationCallerHoldsLock(enableThrottle))
            {
                ScheduleThrottleNotificationCallerHoldsLock(enableThrottle);
            }
        }
    }

    void ReplicatedStore::TransactionReplicator::ScheduleThrottleNotificationCallerHoldsLock(__in bool enableThrottle)
    {
        if (!thStateMachineUPtr_->IsStarted)
        {
            if(!InitializeThrottleCallerHoldsLock().IsSuccess())
            {
                // Failed to initialize. It could be because the replicated store closed or
                // it changed role to non-primary
                return;
            }
            thStateMachineUPtr_->TransitionToStarted();
        }

        WriteInfo(
            TraceComponent,
            "{0} Enqueuing Throttle callback in job queue with shouldThrottle = {1}: \r\n"
            "OperationCount = {2} \r\n"
            "QueueSizeBytes = {3} \r\n",
            this->TraceId,
            enableThrottle,
            thLastUpdatedReplicationQueueOperationCount_ + thPendingReplicationQueueOperationCount_,
            thLastUpdatedReplicationQueueSizeBytes_ + thPendingReplicationQueueSizeBytes_);

        // Invert the flag as this method is called only when the state changes
        thShouldThrottle_ = enableThrottle;
        thCallbackJobQueueSPtr_->Enqueue(ThrottleCallbackJobEntry(enableThrottle, thCallBack_));

        if (enableThrottle)
        {
            // If throttle got enabled start the timer
            thCountersUpdateTimerSPtr_->Change(StoreConfig::GetConfig().ThrottleCountersRefreshInterval, StoreConfig::GetConfig().ThrottleCountersRefreshInterval);
            WriteInfo(TraceComponent, "{0}: Throttle Counters Update timer enabled. Due time = {1}", this->TraceId, StoreConfig::GetConfig().ThrottleCountersRefreshInterval);
        }
        else
        {
            // If throttle got disabled stop the timer
            thCountersUpdateTimerSPtr_->Change(TimeSpan::MaxValue);
            WriteInfo(TraceComponent, "{0}: Throttle Counters Update timer disabled", this->TraceId);
        }
    }

    // *************************
    // ReplicationAsyncOperation
    // *************************

    void ReplicatedStore::TransactionReplicator::ReplicationAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!createError_.IsSuccess())
        {
            this->TryComplete(thisSPtr, createError_);
            return;
        }

        auto error = owner_.replicatedStore_.FatalError;
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        if (!transactionSPtr_)
        {
            // Do not complete simple transaction here. Transaction group 
            // will commit and complete all associated (AddToParent) 
            // simple transactions.
            //
            return;
        }

        // Optimize for read-only transactions
        //
        auto replicationOperationsCount = this->ReplicationOperations.size();

        if (replicationOperationsCount == 0)
        {
            ReplicatedStoreEventSource::Trace->PrimaryCommitReadOnly(this->ReplicaActivityId);

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
            return;
        }

        // Do not measure commit time for read-only transactions
        //
        overallStopwatch_.Start();

        owner_.replicatedStore_.PerfCounters.AvgNumberOfKeysBase.Increment();
        owner_.replicatedStore_.PerfCounters.AvgNumberOfKeys.IncrementBy(replicationOperationsCount);

        AtomicOperation atomicOperation(
            this->ActivityId.Guid,
            move(this->ReplicationOperations),
            static_cast<::FABRIC_SEQUENCE_NUMBER>(owner_.pendingCommitsQueueUPtr_->QuorumAckedLsn));

        Stopwatch stopwatch;
        stopwatch.Start();

        Serialization::IFabricSerializableStreamUPtr serializedStreamUPtr;
        FabricSerializer::Serialize(&atomicOperation, serializedStreamUPtr, txSettings_.SerializationBlockSize);

        stopwatch.Stop();

        // After serialization, restore operations for use when applying LSN updates
        // after replication completion
        //
        error = atomicOperation.TakeReplicationOperations(replicationOperations_);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
            return;
        }

        TimedAsyncOperation::OnStart(thisSPtr);

        ComPointer<ComReplicatedOperationData> comReplicatedOperation = make_com<ComReplicatedOperationData>(Ktl::Move(serializedStreamUPtr));

        ReplicatedStoreEventSource::Trace->PrimaryReplicating(
            this->ReplicaActivityId,
            replicationOperationsCount,
            comReplicatedOperation->BufferCount,
            txSettings_.SerializationBlockSize,
            comReplicatedOperation->OperationDataSize,
            atomicOperation.ActivityId,
            stopwatch.ElapsedMilliseconds);

        this->InitializePendingCommitWaiterCount();

        AsyncOperationSPtr asyncOperation;
        {
            AcquireExclusiveLock lock(owner_.pendingCommitsQueueUPtr_->Lock);

            innerStopwatch_.Start();

            asyncOperation = AsyncOperation::CreateAndStart<ComReplicationAsyncOperation>(
                std::move(comReplicatedOperation),
                owner_.InnerReplicator,
                [this](AsyncOperationSPtr const & operation) { this->InnerReplicationCallback(operation); },
                thisSPtr);

            this->OnInnerReplicationComplete(asyncOperation, true);

            // The queue is guaranteed to be in the correct commit order since 
            // the returned sequence number is guaranteed to be in increasing order.
            //
            error = this->TryAddPendingCommit_CallerHoldsLock(asyncOperation);
        }

        if (error.IsSuccess())
        {
            this->FinishInnerReplication(asyncOperation);
        }
        else
        {
            this->WriteInfo(
                TraceComponent,
                "{0} replication failed: error={1}",
                this->TraceId,
                error);

            this->ResetInnerTransaction();

            this->TryComplete(thisSPtr, error);
        }
    }

    void ReplicatedStore::TransactionReplicator::ReplicationAsyncOperation::InnerReplicationCallback(
        AsyncOperationSPtr const & asyncOperation)
    {
        this->OnInnerReplicationComplete(asyncOperation, false);

        this->FinishInnerReplication(asyncOperation);
    }

    void ReplicatedStore::TransactionReplicator::ReplicationAsyncOperation::OnInnerReplicationComplete(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        innerStopwatch_.Stop();

        owner_.replicatedStore_.PerfCounters.AvgLatencyOfReplicationBase.Increment();
        owner_.replicatedStore_.PerfCounters.AvgLatencyOfReplication.IncrementBy(innerStopwatch_.ElapsedMicroseconds);

        replicatorError_ = ComReplicationAsyncOperation::End(asyncOperation);

        ReplicatedStoreEventSource::Trace->PrimaryReplicationComplete(
            this->ReplicaActivityId,
            replicatorError_.ReadValue(),
            asyncOperation->CompletedSynchronously);
    }

    void ReplicatedStore::TransactionReplicator::ReplicationAsyncOperation::FinishInnerReplication(
        AsyncOperationSPtr const & asyncOperation)
    {
        // Do not call OrderedCommitAndComplete until this operation
        // has been added to the pending commits queue (in OnStart)
        //
        if (this->DecrementPendingCommitWaiterCount() == 0)
        {
            owner_.OrderedCommitAndComplete(asyncOperation->Parent);
        }
    }

    ErrorCode ReplicatedStore::TransactionReplicator::ReplicationAsyncOperation::TryAddPendingCommit_CallerHoldsLock(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto const & thisSPtr = asyncOperation->Parent;

        auto error = (asyncOperation->CompletedSynchronously ? replicatorError_ : ErrorCodeValue::Success);

        if (error.IsSuccess())
        {
            auto casted = AsyncOperation::Get<ComReplicationAsyncOperation>(asyncOperation);

            operationLSN_ = casted->InnerOperationLSN;

            if (operationLSN_ > 0)
            {
                owner_.pendingCommitsQueueUPtr_->Enqueue_CallerHoldsLock(thisSPtr);

                if (owner_.replicatedStore_.IsThrottleEnabled)
                {
                    // Book keeping for throttle support
                    owner_.thPendingReplicationQueueOperationCount_ += 1;
                    owner_.thPendingReplicationQueueSizeBytes_ += casted->OperationDataSize;

                    bool enableThrottle;
                    if (owner_.ShouldScheduleThrottleNotification(enableThrottle))
                    {
                        AcquireWriteLock lock(owner_.thLock_);
                        owner_.ScheduleThrottleNotificationCallerHoldsLock(enableThrottle);
                    }
                }
            }
            else
            {
                this->WriteWarning(
                    TraceComponent,
                    "{0} replicator returned invalid LSN = {1}",
                    this->TraceId,
                    operationLSN_);

                error = ErrorCodeValue::OperationFailed;
            }
        }

        return error;
    }

    void ReplicatedStore::TransactionReplicator::ReplicationAsyncOperation::OnTimeout(AsyncOperationSPtr const & operation)
    {
        TimedAsyncOperation::OnTimeout(operation);

        ReplicatedStoreEventSource::Trace->PrimaryReplicationTimeout(
            this->ReplicaActivityId,
            operationLSN_);
    }

    void ReplicatedStore::TransactionReplicator::ReplicationAsyncOperation::OnCompleted()
    {
        TimedAsyncOperation::OnCompleted();
    }

    ErrorCode ReplicatedStore::TransactionReplicator::ReplicationAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation,
        ::FABRIC_SEQUENCE_NUMBER & operationLSN)
    {
        auto casted = AsyncOperation::End<ReplicationAsyncOperation>(asyncOperation);

        if (casted->Error.IsSuccess())
        {
            operationLSN = casted->OperationLSN;

            if (casted->owner_.replicatedStore_.IsThrottleEnabled &&
                ++casted->owner_.currentReplicationCompletedOperationCount_ > static_cast<uint64>(StoreConfig::GetConfig().ThrottleCountersRefreshIntervalInOperationCount))
            {
                casted->owner_.UpdateThrottleCounters();
            }
        }

       return casted->Error;
    }

    // ****************************
    // ComReplicationAsyncOperation
    // ****************************

    HRESULT ReplicatedStore::TransactionReplicator::ComReplicationAsyncOperation::BeginComAsyncOperation(
        __in IFabricAsyncOperationCallback * callback,
        __out IFabricAsyncOperationContext ** operationContext)
    {
        if (callback == NULL || operationContext == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

        HRESULT hr = innerReplicator_->BeginReplicate(
            replicationOperation_.GetRawPointer(),
            callback,
            &operationLSN_,
            operationContext);

        return ComUtility::OnPublicApiReturn(ConvertReplicatorErrorCode(hr));
    }

    HRESULT ReplicatedStore::TransactionReplicator::ComReplicationAsyncOperation::EndComAsyncOperation(
        __in IFabricAsyncOperationContext * operationContext)
    {
        if (operationContext == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

        HRESULT hr = innerReplicator_->EndReplicate(operationContext, nullptr);

        return ComUtility::OnPublicApiReturn(ConvertReplicatorErrorCode(hr));
    }

    ErrorCode ReplicatedStore::TransactionReplicator::ComReplicationAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
    {
        return AsyncOperation::End<ComReplicationAsyncOperation>(asyncOperation)->Error;
    }

    HRESULT ReplicatedStore::TransactionReplicator::ComReplicationAsyncOperation::ConvertReplicatorErrorCode(HRESULT const replicatorResult)
    {
        if (FABRIC_E_REPLICATION_OPERATION_TOO_LARGE == replicatorResult)
        {
            // Convert the replication operation too large error code to transaction too large as we try to replicate the transaction data as part of 1 replication operation
            return FABRIC_E_TRANSACTION_TOO_LARGE;
        }

        return replicatorResult;
    }
}
