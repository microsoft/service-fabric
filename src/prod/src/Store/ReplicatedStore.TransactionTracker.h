// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::TransactionTracker : public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
    public:
        TransactionTracker(__in ReplicatedStore &);

        bool TryAdd(TransactionBaseSPtr const &);
        void Remove(TransactionBase const &);
        void StartDrainTimer();
        void CancelDrainTimer();
        void AbortOutstandingTransactions();

    private:
        typedef std::unordered_map<uint64, TransactionBaseWPtr> TransactionMap;

        void ScheduleAbortOutstanding(bool enableDrainTimer);
        void AbortOutstanding(Common::ComponentRootSPtr const &, std::shared_ptr<TransactionMap> const & toRelease, bool enableDrainTimer);
        void DrainTimerCallback();

        ReplicatedStore & replicatedStore_;
        TransactionMap outstandingTxs_;

        bool isDraining_;
        Common::TimerSPtr drainTimer_;
        Common::TimeSpan drainTimeout_;
        Common::ExclusiveLock lock_;
    };
}
