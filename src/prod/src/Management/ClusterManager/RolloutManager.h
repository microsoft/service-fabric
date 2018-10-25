// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ClusterManagerReplica;

        class RolloutManager : public Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ClusterManager>
        {
            DENY_COPY(RolloutManager);

        public:
            RolloutManager(ClusterManagerReplica const &);

            virtual ~RolloutManager();

            __declspec(property(get=get_Replica)) ClusterManagerReplica const & Replica;
            ClusterManagerReplica const & get_Replica() const { return replica_; }

            __declspec(property(get=get_IsActive)) bool IsActive;
            bool get_IsActive() const { Common::AcquireReadLock lock(stateLock_); return (state_ == RolloutManagerState::Active); }

            __declspec(property(get=get_PerfCounters)) ClusterManagerPerformanceCounters const & PerfCounters;
            ClusterManagerPerformanceCounters const & get_PerfCounters() const { return *perfCounters_; }

            Common::ErrorCode Start();
            void Stop();
            void Close();

            Common::ErrorCode Enqueue(std::shared_ptr<RolloutContext> &&);

            bool IsReconfiguration(Common::ErrorCode const &);
            static bool IsRetryable(Common::ErrorCode const &);
            static Common::TimeSpan GetOperationTimeout(RolloutContext const &);
            static Common::TimeSpan GetOperationRetry(RolloutContext const &, Common::ErrorCode const &);

            Common::ErrorCode Refresh(__in RolloutContext &) const;

            // Forcefully failing upgrade. It is used by application and compose now.
            void ExternallyFailUpgrade(RolloutContext const &);

        private:
            void SchedulePendingContextCheck(Common::TimeSpan const delay);
            void PendingContextCheckCallback();
            void ScheduleRecovery(Common::ErrorCode const &);
            void RecoveryCallback();
            void OnMigrateDataComplete(Common::ActivityId const &, Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void OnBaselineComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            void RecoveryComplete();
            
            template <class T>
            Common::ErrorCode RecoverRolloutContexts(
                Store::StoreTransaction const &, 
                std::wstring const & type, 
                __inout Common::ActivityId &,
                __out size_t & recoveryCount);

            template <class T>
            Common::ErrorCode RecoverRolloutContexts(
                Store::StoreTransaction const &, 
                std::wstring const & type, 
                __inout Common::ActivityId &,
                __out size_t & recoveryCount,
                __out std::vector<T> & allContexts);

            void ScheduleRolloutContextProcessing(__in RolloutContext &, Common::TimeSpan const retryDelay);
            void PostRolloutContextProcessing(__in RolloutContext &);
            void ProcessRolloutContext(__in RolloutContext &);
            void FinishProcessingRolloutContext(__in RolloutContext &, Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void ProcessPendingApplicationType(__in ApplicationTypeContext &);
            void ProcessPendingApplication(__in ApplicationContext &);
            void ProcessPendingApplicationUpdate(__in ApplicationUpdateContext &, bool);

            void ProcessPendingComposeDeployment(__in ComposeDeploymentContext &);
            void ProcessPendingComposeDeploymentUpgrade(__in ComposeDeploymentUpgradeContext &);

            void ProcessPendingSingleInstanceDeployment(__in SingleInstanceDeploymentContext &);
            void ProcessPendingSingleInstanceDeploymentUpgrade(__in SingleInstanceDeploymentUpgradeContext &);

            void ProcessPendingService(__in ServiceContext &);
            void ProcessPendingApplicationUpgrade(__in ApplicationUpgradeContext &);
            void ProcessPendingFabricProvision(__in FabricProvisionContext &);
            void ProcessPendingFabricUpgrade(__in FabricUpgradeContext &);
            void ProcessPendingInfrastructureTask(__in InfrastructureTaskContext &);

            void DeletePendingApplicationType(__in ApplicationTypeContext &);
            void DeletePendingApplication(__in ApplicationContext &);
            void DeletePendingService(__in ServiceContext &);
            void DeletePendingComposeDeployment(__in ComposeDeploymentContext &);
            void DeletePendingSingleInstanceDeployment(__in SingleInstanceDeploymentContext &);
            void ClearPendingApplicationUpgrade(__in ApplicationUpgradeContext &);
            void ClearPendingComposeDeploymentUpgrade(__in ComposeDeploymentUpgradeContext &);
            void ClearPendingSingleInstanceDeploymentUpgrade(__in SingleInstanceDeploymentUpgradeContext &);

            void ProcessFailedComposeDeployment(__in ComposeDeploymentContext &);
            void OnProcessFailedComposeDeploymentComplete(__in RolloutContext &, Common::AsyncOperationSPtr const &, bool);

            void ProcessFailedSingleInstanceDeployment(__in SingleInstanceDeploymentContext & context);
            void OnProcessFailedSingleInstanceDeploymentComplete(__in RolloutContext &, Common::AsyncOperationSPtr const &, bool);
            
            void ProcessReplacingSingleInstanceDeployment(__in SingleInstanceDeploymentContext &);

            void ProcessUnknown(__in RolloutContext &);
            void ProcessCompleted(__in RolloutContext &);
            void ProcessFailed(__in RolloutContext &, RolloutStatus::Enum);
            void OnProcessFailedCommitComplete(__in RolloutContext &, RolloutStatus::Enum, Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            void OnProcessingError(__in RolloutContext &, Common::ErrorCode const &);
            void OnRetryTimeoutCommitComplete(__in RolloutContext &, Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
            
            void OnUnknownContextType(__in RolloutContext &);

        private:
            bool TryAddActiveContextCallerHoldsLock(std::shared_ptr<RolloutContext> const &, __out std::shared_ptr<RolloutContext> & activeContext);
            void RemoveActiveContext(__in RolloutContext &);

            Common::TimerSPtr recoveryTimer_;
            Common::ExclusiveLock timerLock_;
            Common::ComponentRootSPtr replicaSPtr_;
            ClusterManagerReplica const & replica_;
            ClusterManagerPerformanceCountersSPtr perfCounters_;

            // The following is a state transition diagram for the RolloutManager.
            //
            // The Stop(), Start(), and Close() events correspond to calls on the 
            // public member functions on RolloutManager of the same name.
            //
            // [done] corresponds to completion of recovery, which occurs in the
            // private member function RecoveryCallback().
            //
            // Client requests are only accepted in the Active state.
            // 
            //             
            // ==============+===========+==========+========+========+
            // Current \ New | NotActive | Recovery | Active | Closed |
            // ==============+===========+==========+========+========+
            // NotActive     | Stop()    |  Start() |        | Close()|
            // --------------+-----------+----------+--------+--------+
            // Recovery      | Stop()    |  Start() | [done] | Close()|
            // --------------+-----------+----------+--------+--------+
            // Active        | Stop()    |  Start() |        | Close()|
            // --------------+-----------+----------+--------+--------+
            // Closed        |           |          |        | Close()|
            // --------------+-----------+----------+--------+--------+
            //
            RolloutManagerState::Enum state_;
            mutable Common::RwLock stateLock_;

            // Active contexts will keep the ClusterManagerReplica alive
            std::map<std::wstring, std::shared_ptr<RolloutContext>> activeContexts_;
            Common::ExclusiveLock contextsLock_;
        };
    }
}
