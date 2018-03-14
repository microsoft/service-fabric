// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Health
        {
            class ReportReplicaHealthStateMachineAction : public Infrastructure::StateMachineAction
            {
                DENY_COPY(ReportReplicaHealthStateMachineAction);
            public:
                ReportReplicaHealthStateMachineAction(
                    ReplicaHealthEvent::Enum type,
                    FailoverUnitId ftId,
                    bool isStateful,
                    uint64 replicaId,
                    uint64 instanceId,
                    IHealthDescriptorSPtr const & reportDescriptor);

                void OnPerformAction(
                    std::wstring const & activityId, 
                    Infrastructure::EntityEntryBaseSPtr const & entity, 
                    ReconfigurationAgent & reconfigurationAgent);

                void OnCancelAction(ReconfigurationAgent&);

            private:
                ReplicaHealthEvent::Enum const type_;
                bool const isStateful_;
                uint64 const replicaId_;
                uint64 const instanceId_;
                FailoverUnitId const ftId_;
                IHealthDescriptorSPtr const reportDescriptor_;
            };
        }
    }
}


