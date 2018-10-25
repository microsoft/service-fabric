// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProcessRolloutContextAsyncOperationBase : 
            public Common::AsyncOperation,
            public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::ClusterManager>
        {
        public:
            ProcessRolloutContextAsyncOperationBase(
                __in RolloutManager &,
                Store::ReplicaActivityId const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            virtual ~ProcessRolloutContextAsyncOperationBase() { }

            static Common::ErrorCode End(Common::AsyncOperationSPtr const &);

            Store::StoreTransaction CreateTransaction();

            __declspec(property(get=get_RolloutManager)) RolloutManager & Rollout;
            RolloutManager & get_RolloutManager() const { return rolloutManager_; }

            __declspec(property(get=get_Replica)) ClusterManagerReplica const & Replica;
            ClusterManagerReplica const & get_Replica() const { return rolloutManager_.Replica; }

            __declspec(property(get=get_ImageBuilder)) IImageBuilder & ImageBuilder;
            IImageBuilder & get_ImageBuilder() const { return rolloutManager_.Replica.ImageBuilder; }

            __declspec(property(get=get_Client)) FabricClientProxy const & Client;
            FabricClientProxy const & get_Client() const { return rolloutManager_.Replica.Client; }

            __declspec(property(get=get_Federation)) Reliability::FederationWrapper & Router;
            Reliability::FederationWrapper & get_Federation() const { return rolloutManager_.Replica.Router; }

            __declspec(property(get=get_RemainingTimeout)) Common::TimeSpan const RemainingTimeout;
            Common::TimeSpan const get_RemainingTimeout() const { return timeoutHelper_.GetRemainingTime(); }

            __declspec(property(get=get_PerfCounters)) ClusterManagerPerformanceCounters const & PerfCounters;
            ClusterManagerPerformanceCounters const & get_PerfCounters() const { return rolloutManager_.PerfCounters; }

            bool IsRolloutManagerActive() const { return rolloutManager_.IsActive; }

            bool IsReconfiguration(Common::ErrorCode const & error) const { return rolloutManager_.IsReconfiguration(error); }
            bool IsImageBuilderSuccessOnCleanup(Common::ErrorCode const & error) const;

        protected:
            void ResetTimeout(Common::TimeSpan const);

            Common::ErrorCode TraceAndGetErrorDetails(
                Common::StringLiteral const & traceComponent,
                Common::ErrorCodeValue::Enum error, 
                std::wstring && msg) const
            {
                return rolloutManager_.Replica.TraceAndGetErrorDetails(traceComponent, error, move(msg));
            }

            Common::ErrorCode ReportApplicationPolicy(Common::StringLiteral const & traceComponent, Common::AsyncOperationSPtr const &, __in ApplicationContext &);
            virtual void OnReportApplicationPolicyComplete(__in ApplicationContext &, Common::AsyncOperationSPtr const &, Common::ErrorCode const &) { /* intentional no-op */ }

        protected:
            using RetryCallback = std::function<void(Common::AsyncOperationSPtr const &)>;

            // Callback called by the operation to let the parallel operation executor how it completed
            using ParallelOperationsCompletedCallback = std::function<void(Common::AsyncOperationSPtr const & parent, Common::ErrorCode const & error)>;
            
            // Callback called by Parallel operation executor to start the inner operation at specified index
            using ParallelOperationCallback = std::function<Common::ErrorCode(
                Common::AsyncOperationSPtr const & parent,
                size_t operationIndex,
                ParallelOperationsCompletedCallback const & completedCallback)>;
                        
            class ParallelRetryableAsyncOperation : public Common::AsyncOperation
            {
                DENY_COPY(ParallelRetryableAsyncOperation);
            public:
                ParallelRetryableAsyncOperation(
                    __in ProcessRolloutContextAsyncOperationBase & owner,
                    __in RolloutContext & context,
                    long count,
                    ParallelOperationCallback const & operationCallback,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent);
                ParallelRetryableAsyncOperation(
                    __in ProcessRolloutContextAsyncOperationBase & owner,
                    __in RolloutContext & context,
                    long count,
                    bool waitForAllOperationsOnError,
                    ParallelOperationCallback const & operationCallback,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent);
                virtual ~ParallelRetryableAsyncOperation() { }

                static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation);

            protected:
                virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;
                virtual bool IsInnerErrorRetryable(Common::ErrorCode const & error) const;
                virtual void ScheduleRetryOnInnerOperationFailed(
                    Common::AsyncOperationSPtr const & thisSPtr,
                    RetryCallback const & callback,
                    Common::ErrorCode && firstError);

                RolloutContext & context_;

            private:
                void InitializeCtor();
                void Run(Common::AsyncOperationSPtr const & thisSPtr, long count);
                void OnOperationComplete(
                    Common::AsyncOperationSPtr const & thisSPtr,
                    size_t index,
                    Common::ErrorCode const & error);

            private:
                // Kept alive by AsyncOperation parent SPtr
                ProcessRolloutContextAsyncOperationBase & owner_;

                long count_;
                bool waitForAllOperationsOnError_;
                Common::atomic_long pendingOperations_;
                std::vector<Common::ErrorCode> operationsCompleted_;
                ParallelOperationCallback operationCallback_;
            };

        public:
            Common::AsyncOperationSPtr BeginSubmitDeleteDnsServicePropertyBatch(
                __in Common::AsyncOperationSPtr const & thisSPtr,                
                __in std::wstring const & serviceDnsName,
                __in Common::ByteBuffer && buffer,
                __in Common::ActivityId const & activityId,
                __in Common::AsyncCallback const & jobQueueCallback);
            Common::ErrorCode EndSubmitDeleteDnsServicePropertyBatch(
                __in Common::AsyncOperationSPtr const & operation,
                __in Common::StringLiteral const & traceComponent,
                __in std::wstring const & serviceDnsName,
                __in std::wstring const & serviceName);

            Common::AsyncOperationSPtr BeginCreateServiceDnsName(
                std::wstring const & serviceName,
                std::wstring const & serviceDnsName,
                Common::ActivityId const & activityId,
                Common::TimeSpan const &timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const &parent);
            Common::ErrorCode EndCreateServiceDnsName(Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr BeginDeleteServiceDnsName(
                std::wstring const & serviceName,
                std::wstring const & serviceDnsName,
                Common::ActivityId const & activityId,
                Common::TimeSpan const &timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const &parent);
            Common::ErrorCode EndDeleteServiceDnsName(Common::AsyncOperationSPtr const &);

        private:
            void OnReportApplicationPolicyComplete(__in ApplicationContext &, Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

            RolloutManager & rolloutManager_;
            Common::TimeoutHelper timeoutHelper_;

            class CreateServiceDnsNameAsyncOperation;
            class DeleteServiceDnsNameAsyncOperation;
        };
    }
}
