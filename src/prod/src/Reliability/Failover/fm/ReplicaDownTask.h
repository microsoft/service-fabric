// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ReplicaDownTask : public DynamicStateMachineTask
        {
            DENY_COPY(ReplicaDownTask);

        public:

            ReplicaDownTask(
                ReplicaDownOperationSPtr const& operation,
                FailoverManager & fm,
                FailoverUnitId const& failoverUnitId,
                ReplicaDescription && replicaDescription,
                Federation::NodeInstance const& from);

            void CheckFailoverUnit(
                LockedFailoverUnitPtr & lockedFailoverUnit,
                std::vector<StateMachineActionUPtr> & actions);

        private:

            // This implementation makes use of the current state machine
            // behavior that the dynamic action will always be there before
            // we execute its action, otherwise it is not safe.
            class ReplicaDownAction : public StateMachineAction
            {
                DENY_COPY(ReplicaDownAction);

            public:

                ReplicaDownAction(ReplicaDownTask & task_);

                int OnPerformAction(FailoverManager & fm);

                void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;
                void WriteToEtw(uint16 contextSequenceId) const;

            private:

                ReplicaDownTask & task_;
            };

            FailoverManager & fm_;
            ReplicaDownOperationSPtr operation_;
            FailoverUnitId failoverUnitId_;
            ReplicaDescription replicaDescription_;
            Federation::NodeInstance from_;
        };
    }
}
