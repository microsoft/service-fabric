// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class TraceMovementAction : public StateMachineAction
        {
            DENY_COPY(TraceMovementAction);

        public:

            TraceMovementAction(LoadBalancingComponent::FailoverUnitMovement && movement, Common::Guid decisionId);

            int OnPerformAction(FailoverManager & fm);

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:

            LoadBalancingComponent::FailoverUnitMovement movement_;
            Common::Guid decisionId_;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class TraceDataLossAction : public StateMachineAction
        {
            DENY_COPY(TraceDataLossAction);

        public:

            TraceDataLossAction(FailoverUnitId const& failoverUnitId, Epoch dataLossEpoch);

            int OnPerformAction(FailoverManager & fm);

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:

            FailoverUnitId failoverUnitId_;
            Epoch dataLossEpoch_;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        class TraceAutoScaleAction : public StateMachineAction
        {
            DENY_COPY(TraceAutoScaleAction);

        public:

            TraceAutoScaleAction(int targetReplicaSetSize);

            int OnPerformAction(FailoverManager & fm);

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            int targetReplicaSetSize_;
        };
    }
}
