// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ReconfigurationHealthState
        {
            DENY_COPY_ASSIGNMENT(ReconfigurationHealthState);

        public:

            ReconfigurationHealthState(const FailoverUnitId * failoverUnitId, Reliability::ServiceDescription * serviceDescription, int64 * localReplicaId, int64 * localReplicaInstanceId);

            ReconfigurationHealthState(const FailoverUnitId * failoverUnitId, Reliability::ServiceDescription * serviceDescription, int64 * localReplicaId, int64 * localReplicaInstanceId, ReconfigurationHealthState const  & other);

            __declspec(property(get = get_CurrentReport)) ReconfigurationStuckDescriptorSPtr CurrentReport;
            ReconfigurationStuckDescriptorSPtr get_CurrentReport() { return currentReport_; }
            
            //Clears any present health report on HM and resets the current descriptor pointer
            void OnPhaseChanged(Infrastructure::StateMachineActionQueue & queue, ReconfigurationStuckDescriptorSPtr clearReport);

            //Updates the current health report pointer and HM report if received Descriptor instance is different than the current one
            void OnReconfigurationPhaseHealthReport(Infrastructure::StateMachineActionQueue & queue, ReconfigurationStuckDescriptorSPtr newReport);
            
        private:
            ReconfigurationStuckDescriptorSPtr currentReport_;
            const FailoverUnitId * failoverUnitId_;
            const Reliability::ServiceDescription * serviceDesc_;
            const int64 * localReplicaId_;
            const int64 * localReplicaInstanceId_;
        };        
    }
}
