// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    // This class manages replicating local transactions performed on the primary
    // to the secondaries. Transactions on the primary are only committed in
    // monotonically increasing LSN order after quorum ack so that the copy process 
    // (building a new replica), does not have to deal with LSN "holes".
    //
    class ReplicatedStore::TransactionReplicator 
        : public ComponentRoot
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::TraceId;
        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::get_TraceId;

        DENY_COPY(TransactionReplicator);

    public:
        TransactionReplicator(
            __in ReplicatedStore & replicatedStore,
            __in Common::ComPointer<IFabricStateReplicator> const& innerReplicator,
            __in Common::ComPointer<IFabricInternalStateReplicator> const& innerInternalReplicator);

        virtual ~TransactionReplicator();

        __declspec(property(get=get_InnerReplicator)) Common::ComPointer<IFabricStateReplicator> const & InnerReplicator;
        Common::ComPointer<IFabricStateReplicator> const & get_InnerReplicator() const { return innerReplicator_; }

        __declspec(property(get=get_InnerInternalReplicator)) Common::ComPointer<IFabricInternalStateReplicator> const & InnerInternalReplicator;
        Common::ComPointer<IFabricInternalStateReplicator> const & get_InnerInternalReplicator() const { return innerInternalReplicator_; }

        void Initialize(::FABRIC_SEQUENCE_NUMBER lastCommittedLSN);
        void TryUnInitialize();

        void UpdateThrottleCallback(ThrottleCallback const & callBack);
        bool IsThrottleNeeded();

        Common::AsyncOperationSPtr BeginReplicate(
            __in Store::ReplicatedStore::Transaction &,
            __in Common::TimeSpan const timeout,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::AsyncOperationSPtr BeginReplicate(
            __in Store::ReplicatedStore::SimpleTransactionGroup &,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::AsyncOperationSPtr BeginReplicate(
            __in Store::ReplicatedStore::SimpleTransaction &,
            __in Common::TimeSpan const timeout,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::AsyncOperationSPtr BeginReplicate(
            __in Common::ErrorCode const & creationError,
            __in Store::ReplicaActivityId const & activityId,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndReplicate(Common::AsyncOperationSPtr const &, __out ::FABRIC_SEQUENCE_NUMBER &);

        void Cleanup();

    private:
        class ComReplicationAsyncOperation;
        class ReplicationAsyncOperation;
        class PendingCommitsQueue;
        class ThrottleStateMachine;
        class InvokeThrottleCallbackJobQueue;

        typedef std::shared_ptr<ReplicationAsyncOperation> ReplicationAsyncOperationSPtr;
        typedef std::shared_ptr<InvokeThrottleCallbackJobQueue> InvokeThrottleCallbackJobQueueSPtr;
        typedef std::unique_ptr<ThrottleStateMachine> ThrottleStateMachineUPtr;
       
        void OrderedCommitAndComplete(Common::AsyncOperationSPtr const &);
        void DrainReadyCommits(::FABRIC_SEQUENCE_NUMBER currentLsn, Common::ErrorCode const &);
        void FlushCommits();
        void ApplyReadyCommits(
            __in std::queue<ReplicationAsyncOperationSPtr> &, 
            ::FABRIC_SEQUENCE_NUMBER currentLsn,
            __in Common::ErrorCode &);
        Common::ErrorCode StartLocalCommit(ReplicationAsyncOperationSPtr const &);
        void OnCommitComplete(Common::AsyncOperationSPtr const &);
        
        //**********************************************************
        //  Throttle specific methods
        //**********************************************************
        void UpdateThrottleCounters();
        bool IsThrottleNeededInternalCallerHoldsLock();
        void ScheduleThrottleNotificationCallerHoldsLock(bool enableThrottle);

        bool ShouldScheduleThrottleNotification(__out bool & enableThrottle);
        bool ShouldScheduleThrottleNotificationCallerHoldsLock(__out bool & enableThrottle);

        Common::ErrorCode InitializeThrottleCallerHoldsLock();

        Store::ReplicatedStore & replicatedStore_;
        Common::ComPointer<IFabricStateReplicator> innerReplicator_;
        Common::ComPointer<IFabricInternalStateReplicator> innerInternalReplicator_;

        std::unique_ptr<PendingCommitsQueue> pendingCommitsQueueUPtr_;
        Common::RwLock localFlushLock_;
        bool shouldLocalFlush_;
        
        //**********************************************************
        //  Throttle Specific: All members start with a prefix 'th'
        //**********************************************************

        // Number of completed Replicate Calls, counting from the time when the throttle counters were refreshed
        Common::atomic_uint64 currentReplicationCompletedOperationCount_;

        // This lock protects all the data structures below
        RWLOCK(StoreTransactionReplicatorThrottle, thLock_);

        ThrottleStateMachineUPtr thStateMachineUPtr_;

        // Counters used for calculating Throttle 
        ULONGLONG thLastUpdatedReplicationQueueOperationCount_;
        ULONGLONG thLastUpdatedReplicationQueueSizeBytes_;
        ULONGLONG thPendingReplicationQueueOperationCount_;
        ULONGLONG thPendingReplicationQueueSizeBytes_;

        // Last notified state of throttle to the user
        bool thShouldThrottle_;
        
        // To ensure in order delivery of throttle notifications callback, we use a reader queue with a thread count = 1
        InvokeThrottleCallbackJobQueueSPtr thCallbackJobQueueSPtr_;
        
        // When throttle is enabled, this timer periodically checks the replication queue counters to see if throttle can be disabled
        // Without this timer, it is possible to get into a situation where we enable throttle on the last EndReplicate call
        // and after that, we never disable it as there are no longer any operations from the user
        Common::TimerSPtr thCountersUpdateTimerSPtr_;
        
        // Indicates if the throttle logic is currently running
        // Optimization to avoid having multiple threads waiting on a write lock
        Common::atomic_bool thActivelyRunning_;

        // User callback
        ThrottleCallback thCallBack_;
    };
}
