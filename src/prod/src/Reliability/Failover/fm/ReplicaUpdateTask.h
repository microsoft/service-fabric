// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ReplicaUpdateTask : public DynamicStateMachineTask
        {
            DENY_COPY(ReplicaUpdateTask);

        public:
            ReplicaUpdateTask(
                ReplicasUpdateOperationSPtr const & operation,
                FailoverManager & fm,
                FailoverUnitInfo && failoverUnitInfo,
                bool isDropped,
                Federation::NodeInstance const & from);

            void CheckFailoverUnit(
                LockedFailoverUnitPtr & lockedFailoverUnit,
                std::vector<StateMachineActionUPtr> & actions);

            Common::ErrorCode ProcessMissingFailoverUnit();

        private:
            // This implementation makes use of the current state machine
            // behavior that the dynamic action will always be there before
            // we execute its action, otherwise it is not safe.
            class ReplicaUpdateAction : public StateMachineAction
            {
                DENY_COPY(ReplicaUpdateAction);

            public:
                ReplicaUpdateAction(
                    ReplicaUpdateTask & task_,
                    bool dropNeeded,
                    bool isFullyProcessed);

                int OnPerformAction(FailoverManager & fm);

                void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
                void WriteToEtw(uint16 contextSequenceId) const;

            private:
                ReplicaUpdateTask & task_;
                bool dropNeeded_;
                bool isFullyProcessed_;
            };

            void ProcessNormalReplica(LockedFailoverUnitPtr & failoverUnit, __out bool & isDropped, __out bool & isFullyProcessed);
            void ProcessDroppedReplica(LockedFailoverUnitPtr & failoverUnit);

            FailoverManager & fm_;
            ReplicasUpdateOperationSPtr operation_;
            FailoverUnitInfo failoverUnitInfo_;
            bool isDropped_;
            Federation::NodeInstance from_;
        };
    }
}
