// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ReplicasUpdateOperation;
        typedef std::shared_ptr<ReplicasUpdateOperation> ReplicasUpdateOperationSPtr;

        class ReplicasUpdateOperation : public std::enable_shared_from_this<ReplicasUpdateOperation>
        {
            DENY_COPY(ReplicasUpdateOperation);

        public:
            struct OperationType
            {
                enum Enum
                {
                    ReplicaUp = 0,
                };
            };

            ReplicasUpdateOperation(
                FailoverManager & failoverManager,
                OperationType::Enum type,
                Federation::NodeInstance const & from);

            ~ReplicasUpdateOperation();

            void AddResult(
                FailoverManager & failoverManager,
                FailoverUnitInfo && failoverUnitInfo,
                bool dropNeeded);

            void AddRetry(
                FailoverManager & failoverManager,
                FailoverUnitInfo && failoverUnitInfo);

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        protected:
            void Start(
                ReplicasUpdateOperationSPtr const & thisSPtr,
                FailoverManager & failoverManager,
                ReplicaUpMessageBody & replicaUpMessageBody);

            void SetError(Common::ErrorCode error, FailoverManager & failoverManager);
            void SetError(Common::ErrorCode error, FailoverManager & failoverManager, FailoverUnitId const & failoverUnitId);

            virtual void OnCompleted(FailoverManager & failoverManager) = 0;

            Federation::NodeInstance from_;
            std::vector<FailoverUnitInfo> processed_;
            std::vector<FailoverUnitInfo> dropped_;

            Common::ErrorCode error_;
            
        private:
            void ProcessReplica(
                ReplicasUpdateOperationSPtr const & thisSPtr,
                FailoverManager & failoverManager,
                FailoverUnitInfo && failoverUnitInfo,
                bool isDropped);

            void AddReplicaUpdateTask(
                ReplicasUpdateOperationSPtr const & thisSPtr,
                FailoverManager & failoverManager,
                FailoverUnitInfo && failoverUnitInfo,
                bool isDropped);

            void Complete(FailoverManager & failoverManager);
            bool RemovePending(FailoverUnitId const & failoverUnitId);

            void OnTimer();

            OperationType::Enum type_;
            std::set<FailoverUnitId> pending_;

            std::map<FailoverUnitId, DynamicStateMachineTaskUPtr> retryList_;

            Common::TimeoutHelper timeoutHelper_;

            Common::RootedObjectWeakHolder<FailoverManager> root_;
            Common::TimerSPtr timer_;
            bool completed_;

            RWLOCK(FM.ReplicasUpdateOperation, lock_);
        };

        class ReplicaUpProcessingOperation : public ReplicasUpdateOperation
        {
        public:
            ReplicaUpProcessingOperation(
                FailoverManager & failoverManager,
                Federation::NodeInstance const & from);

            void Process(
                ReplicasUpdateOperationSPtr const & thisSPtr,
                FailoverManager & failoverManager,
                ReplicaUpMessageBody & body);

        protected:
            void OnCompleted(FailoverManager & failoverManager);

        private:
            void SendReplicaUpReply(
                bool isLastReplicaUpFromRAProcessedSuccessfully,
                FailoverManager & failoverManage);

            bool isLastReplicaUpMessageFromRA_;
        };

        void WriteToTextWriter(Common::TextWriter & w, ReplicasUpdateOperation::OperationType::Enum const & val);
    }
}
